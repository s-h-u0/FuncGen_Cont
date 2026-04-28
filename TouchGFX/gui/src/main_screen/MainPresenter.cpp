/**************************************************************
 * @file    MainPresenter.cpp
 * @brief   Main 画面の Presenter（View と Model の仲介）
 * @details
 *  - 役割: View からの要求を子機へのコマンド送信へ橋渡しし、
 *          子機から返ってきた状態を View / Model に反映する
 *  - 設計:
 *      * 子機が正本
 *      * Model は子機から最後に確認した状態の親機キャッシュ
 *      * setDesiredValue()/runCurrent()/stopCurrent() は送信要求を出すだけ
 *      * 確定反映は AppRemote_QueryState() 後の onRemoteLine() で行う
 *      * RUN/STOP も送信成功時点では確定しない。AppRemote_Run()/AppRemote_Stop() は要求送信のみ行い、
 *        AppRemote_QueryState() 後に受信した RUN/STOP イベントを onRemoteLine() で Model::running[id] へ確定反映する
 *      * synced は、親機キャッシュがその時点で子機確認済みかどうかを表し、
 *        要求送信後や画面表示直後の再確認前は false、子機応答で確定反映できたら true になる
 **************************************************************/


#include <gui/main_screen/MainPresenter.hpp>
#include <gui/main_screen/MainView.hpp>
#include <gui/model/Model.hpp>

#include <cstdint>

extern "C" {
#include "app_remote.h"
#include "rs485_bridge.h"
#include <cstdio>
}


namespace {

/** Model の保持値を、現在選択中IDの画面表示へ反映する。 */
inline void updateBothValuesFromModel(Model* m, MainView& v)
{
    if (!m) return;

    const uint8_t id = AppRemote_GetID();

    const uint32_t vVolt = m->getDesiredValue(SettingType::Voltage, id);
    const uint32_t vCurr = m->getDesiredValue(SettingType::TripCurrent, id);
    const uint32_t vPhas = m->getDesiredValue(SettingType::Phase, id);

    v.updateBothValues(vVolt, vCurr, vPhas, id);
    v.setDipHex(static_cast<uint8_t>(id & 0x0F));
}

/** 子機から確認済みの状態を受けたことを Model に記録する。 */
inline void markConfirmed(Model* m, uint8_t id)
{
    if (!m) return;

    m->noteAlive(id, HAL_GetTick());
    m->setSynced(id, true);
}

/** コマンド送信直後の「未確認」状態に戻す。 */
inline void markUnsynced(Model* m, uint8_t id)
{
    if (m) {
        m->setSynced(id, false);
    }
}

/** コマンド送信後、状態問い合わせを発行して確定反映を待つ。 */
inline void requestStateRefresh(Model* m, uint8_t id)
{
    markUnsynced(m, id);
    AppRemote_QueryState();
}

/** 位相変更後は全chを再同期対象として扱う。 */
inline void markAllChannelsSyncNeeded(Model* m, MainView& v)
{
    if (m) {
        for (uint8_t i = 0; i < Model::kMaxId; ++i) {
            m->setSyncNeeded(i, true);
        }
    }

    v.updateSyncButtonUI(true);
}

}  // namespace

/** @brief コンストラクタ：対応する View を束縛 */
MainPresenter::MainPresenter(MainView& v) : view(v) {}

/** @brief 画面表示時に Model キャッシュを仮表示し、子機との再同期を開始する
 *  @details
 *   - まず Model に保持している現在IDのキャッシュ値を View に反映する
 *   - 問い合わせ完了を待たずに直前キャッシュを先に見せることで、画面表示の応答性を保つ
 *   - この時点ではまだ子機へ最新確認していないため、synced は false にする
 *   - その後 AppRemote_QueryState() を発行し、子機から返る状態を onRemoteLine() で確定反映する
 */
void MainPresenter::activate()
{
    const uint8_t id = AppRemote_GetID();

    markUnsynced(model, id);

    updateBothValuesFromModel(model, view);

    const bool running = model ? model->isRunning(id) : false;
    view.updateRunStopUI(running);

    AppRemote_QueryState();
}

/** @brief 画面終了時のライフサイクルフック（現状処理なし） */
void MainPresenter::deactivate() {}


/** @brief Modelから設定値(PVではなく設定値)を取得
 *  @param t 取得対象の種別（電圧/位相など）
 *  @return 設定値（存在しなければ0）
 */
uint32_t MainPresenter::getDesiredValue(SettingType t) const
{
    const uint8_t id = AppRemote_GetID();
    return model ? model->getDesiredValue(t, id) : 0;
}


/** @brief 現在の設定対象（どの項目を編集するか）を切り替え */
void MainPresenter::setCurrentSetting(SettingType s)
{
    if (model) model->setCurrentSetting(s);
}


/** @brief 現在の設定対象を取得（未設定時は Voltage を既定に） */
SettingType MainPresenter::getCurrentSetting() const
{
    return model ? model->getCurrentSetting() : SettingType::Voltage;
}

bool MainPresenter::isCurrentIdSynced() const
{
    const uint8_t id = AppRemote_GetID();
    return model ? model->isSynced(id) : false;
}


/** @brief 現在選択中IDの子機から計測値を同期取得し、Viewへ反映する
 *  @details
 *   - 1回の呼び出しで取得するのは Voltage / Current のどちらか片方のみ
 *   - 種別は ID ごとの nextQueryIsVolt[] により交互に切り替える
 *   - 通信成功時は最新値を lastVolt_mV[] / lastCurr_mA[] に保存し、Viewへ反映する
 *   - 通信失敗時は直前の成功値があればそれを表示し続ける
 *   - 連続失敗がしきい値に達した場合のみ、その種別の表示を "--" 相当に落とす
 *   - failVolt[] / failCurr[] と backoff により、失敗時の再試行間隔を伸ばして通信負荷を抑える
 *   - lastDoneTick[] は ID ごとの問い合わせ完了時刻を表し、問い合わせ周期の下限管理に使う
 */
void MainPresenter::updateMeasuredValues()
{
    if (RS485_IsBusy()) return;

    constexpr uint32_t kStatePollPeriodMs  = 2000u;
    constexpr uint32_t kAliveWindowMs      = 1500u;
    constexpr uint32_t kMinPeriodAliveMs   = 150u;
    constexpr uint32_t kMeasTimeoutMs      = 150u;
    constexpr uint32_t kBackoffBaseMs      = 80u;
    constexpr uint8_t  kBackoffShiftMax    = 4u;
    constexpr uint8_t  kFailHideThreshold  = 3u;
    constexpr uint8_t  kFailStreakMax      = 8u;
    constexpr uint32_t kMeasStaleSyncLostMs = 3000u;

    static uint32_t lastStatePollTick[Model::kMaxId] = {0};

    static uint32_t lastDoneTick[Model::kMaxId]    = {0};
    static uint8_t  failVolt[Model::kMaxId]        = {0};
    static uint8_t  failCurr[Model::kMaxId]        = {0};
    static uint32_t lastMeasOkTick[Model::kMaxId]  = {0};

    static int32_t  lastVolt_mV[Model::kMaxId]     = {0};
    static int32_t  lastCurr_mA[Model::kMaxId]     = {0};
    static uint8_t  hasVolt[Model::kMaxId]         = {0};
    static uint8_t  hasCurr[Model::kMaxId]         = {0};
    static uint8_t  nextQueryIsVolt[Model::kMaxId] = {1};

    const uint8_t id = AppRemote_GetID();
    const uint32_t startTick = HAL_GetTick();

    /*
     * 取りこぼし補正用の低頻度 STATE?。
     * RUN/STOP/STAT通知を取りこぼしても、次回STATEでModelキャッシュを修復する。
     * 測定問い合わせとは同じタイミングで重ねない。
     */
    if ((int32_t)(startTick - lastStatePollTick[id]) >= (int32_t)kStatePollPeriodMs) {
        lastStatePollTick[id] = startTick;
        lastDoneTick[id] = startTick;

        AppRemote_QueryState();
        return;
    }

    const bool likelyAlive = model ? model->isLikelyAlive(id, startTick, kAliveWindowMs) : false;

    if (likelyAlive) {
        if ((int32_t)(startTick - lastDoneTick[id]) < (int32_t)kMinPeriodAliveMs) return;
    }

    const uint32_t to_ms = kMeasTimeoutMs;
    const bool doVoltQuery = (nextQueryIsVolt[id] != 0);

    uint32_t backoff = 0;
    const uint8_t streak = doVoltQuery ? failVolt[id] : failCurr[id];

    if (!likelyAlive || streak > 0) {
        uint8_t s = streak;
        if (s > kBackoffShiftMax) s = kBackoffShiftMax;
        backoff = kBackoffBaseMs << s;
        if ((int32_t)(startTick - lastDoneTick[id]) < (int32_t)backoff) return;
    }

    int32_t volt_phys_mv = 0;
    int32_t curr_ma      = 0;

    bool okVolt = false;
    bool okCurr = false;

    if (doVoltQuery) {
        okVolt = AppRemote_MeasVolt(&volt_phys_mv, to_ms);
    } else {
        okCurr = AppRemote_MeasCurr(&curr_ma, to_ms);
    }

    const uint32_t doneTick = HAL_GetTick();

    lastDoneTick[id] = doneTick;

    if (doVoltQuery) {
        if (okVolt) {
        	lastMeasOkTick[id] = doneTick;
            lastVolt_mV[id] = volt_phys_mv;
            hasVolt[id] = 1;
            view.setMeasuredVolt_Phys_mV(volt_phys_mv);
        } else if (hasVolt[id]) {
            view.setMeasuredVolt_Phys_mV(lastVolt_mV[id]);
        } else {
            view.setMeasuredVolt_Phys_mV(INT32_MIN);
        }
    } else {
        if (okCurr) {
        	lastMeasOkTick[id] = doneTick;
            lastCurr_mA[id] = curr_ma;
            hasCurr[id] = 1;
            view.setMeasuredCurr_mA(curr_ma);
        } else if (hasCurr[id]) {
            view.setMeasuredCurr_mA(lastCurr_mA[id]);
        } else {
            view.setMeasuredCurr_mA(INT32_MIN);
        }
    }

    if (doVoltQuery) {
        if (okVolt) {
            if (model) model->noteAlive(id, doneTick);
            failVolt[id] = 0;
        } else {
        	if (failVolt[id] < kFailStreakMax) ++failVolt[id];
        	if (failVolt[id] >= kFailHideThreshold) {
                hasVolt[id] = 0;
                view.setMeasuredVolt_Phys_mV(INT32_MIN);
            }
        }
    } else {
        if (okCurr) {
            if (model) model->noteAlive(id, doneTick);
            failCurr[id] = 0;
        } else {
            if (failCurr[id] < kFailStreakMax) ++failCurr[id];
            if (failCurr[id] >= kFailHideThreshold) {
                hasCurr[id] = 0;
                view.setMeasuredCurr_mA(INT32_MIN);
            }
        }
    }

    if (lastMeasOkTick[id] != 0U &&
        (int32_t)(doneTick - lastMeasOkTick[id]) >= (int32_t)kMeasStaleSyncLostMs) {

        if (model) {
            model->setSyncNeeded(id, true);
        }

        if (id == AppRemote_GetID()) {
            view.updateSyncButtonUI(true);
        }
    }

    nextQueryIsVolt[id] ^= 1u;
}


// ==========================================================
// RS-485受信イベント：UIに反映
// ==========================================================

/** @brief 子機から受信した状態行を親機キャッシュへ確定反映する
 *  @details
 *   - 本プロジェクトでは子機が正本であり、送信成功だけでは状態確定しない
 *   - RUN/STOP の親機内キャッシュは Model::running[id] のみを正とする
 *   - 子機からの状態イベントを受けたこの関数で、必要な Model 更新と
 *     Model::setSynced(id, true) を行って確定反映する
 *   - current ID と一致する場合のみ View の見た目も更新する
 */
void MainPresenter::onRemoteLine(const char* line)
{
    if (!line) return;

    AppRemote_Event ev;
    if (!AppRemote_ParseLine(line, &ev)) {
        view.showRemoteLine(line);
        return;
    }

    if (ev.id == 0xFF) {
        view.showRemoteLine(line);
        return;
    }

    const uint8_t id = ev.id;
    const uint8_t current = AppRemote_GetID();


    /* STAT_* は子機から返った確認済み設定値。
     * ここで Model の親機キャッシュへ確定反映する。
     */
    switch (ev.type) {
    case APPREMOTE_EVT_RUN:
        if (model) {
            model->setRunning(id, true);
            markConfirmed(model, id);
        }
        if (id == current) {
            view.updateRunStopUI(true);
        }
        break;

    case APPREMOTE_EVT_STOP:
        if (model) {
            model->setRunning(id, false);
            markConfirmed(model, id);
        }
        if (id == current) {
            view.updateRunStopUI(false);
        }
        break;

    case APPREMOTE_EVT_STAT_VOLT:
        if (model) {
            model->setDesiredValue(SettingType::Voltage, ev.value, id);
            markConfirmed(model, id);
        }
        break;

    case APPREMOTE_EVT_STAT_TRIP_CURR:
        if (model) {
            model->setDesiredValue(SettingType::TripCurrent, ev.value, id);
            markConfirmed(model, id);
        }
        break;

    case APPREMOTE_EVT_STAT_PHAS:
        if (model) {
            model->setDesiredValue(SettingType::Phase, ev.value, id);
            markConfirmed(model, id);
        }
        break;

    case APPREMOTE_EVT_STAT_FREQ:
        if (model) {
            model->setDesiredFreq(id, ev.value);
            markConfirmed(model, id);
        }
        break;

    case APPREMOTE_EVT_STAT_READY:
        if (model) {
            model->setDdsArmed(id, ev.value != 0);
            markConfirmed(model, id);
        }
        break;

    case APPREMOTE_EVT_STAT_MCLK:
        if (model) {
            model->setMclkEnabled(id, ev.value != 0);
            markConfirmed(model, id);
        }
        break;

    default:
        break;
    }

    /* 現在表示中のIDに対する確定反映なら、設定値表示を Model から更新し、
     * あわせて同期状態などの補助UIも再評価して画面へ反映する。
     */
    if (id == current) {
        updateBothValuesFromModel(model, view);
        view.triggerRefreshFromPresenter();
    }
}


/** @brief 設定変更コマンドを子機へ送信する
 *  @param t 設定項目（電圧/電流/位相）
 *  @param v 設定値
 *  @details
 *   - この関数では親機 Model の確定値は更新しない
 *   - 送信成功時は、その時点ではまだ子機確認前なので synced を false にする
 *   - 送信成功時に AppRemote_QueryState() を発行し、
 *     子機から返る状態を onRemoteLine() で確定反映する
 */
void MainPresenter::setDesiredValue(SettingType t, uint32_t v)
{
    bool ok = false;
    const uint8_t id = AppRemote_GetID();

    switch (t) {
    case SettingType::Voltage:
        ok = AppRemote_SetVolt(v);
        break;

    case SettingType::TripCurrent:
        ok = AppRemote_SetTripCurr(v);
        break;

    case SettingType::Phase:
        ok = AppRemote_SetPhas((uint16_t)v);
        break;

    default:
        break;
    }

    if (ok) {
        if (t == SettingType::Phase) {
            markAllChannelsSyncNeeded(model, view);
        }

        requestStateRefresh(model, id);
    }
}


/** @brief 現在選択中IDの子機へ RUN 要求を送る
 *  @details
 *   - この関数は RUN コマンド送信だけを行い、親機の RUN 状態はここでは確定しない
 *   - 送信成功時は、その時点ではまだ子機確認前なので synced を false にする
 *   - その後 AppRemote_QueryState() を発行し、RUN の確定反映は
 *     onRemoteLine() で受けた APPREMOTE_EVT_RUN に委ねる
 */
void MainPresenter::runCurrent()
{
    const uint8_t id = AppRemote_GetID();

    if (AppRemote_Run()) {
        requestStateRefresh(model, id);
    }
}


/** @brief 現在選択中IDの子機へ STOP 要求を送る
 *  @details
 *   - この関数は STOP コマンド送信だけを行い、親機の STOP 状態はここでは確定しない
 *   - 送信成功時は、その時点ではまだ子機確認前なので synced を false にする
 *   - その後 AppRemote_QueryState() を発行し、STOP の確定反映は
 *     onRemoteLine() で受けた APPREMOTE_EVT_STOP に委ねる
 */
void MainPresenter::stopCurrent()
{
    const uint8_t id = AppRemote_GetID();

    if (AppRemote_Stop()) {
        requestStateRefresh(model, id);
    }
}


void MainPresenter::syncStart()
{
    const bool all_ok = AppRemote_SyncStart();


    if (model) {
        for (uint8_t id = 0; id < 8; ++id) {
            const bool ok = AppRemote_GetLastSyncOk(id);
            model->setSyncNeeded(id, !ok);
        }
    }

    const uint8_t current = AppRemote_GetID();
    view.updateSyncButtonUI(model ? model->isSyncNeeded(current) : true);

    if (!all_ok) {
        view.showRemoteLine("SYNC failed");
    }
}

bool MainPresenter::isSyncNeeded(uint8_t id) const
{
    return model ? model->isSyncNeeded(id) : true;
}
