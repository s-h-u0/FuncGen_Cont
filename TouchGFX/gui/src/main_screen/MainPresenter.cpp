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



extern "C" {
#include "app_remote.h"
#include <cstdio>   // printf()
#include <cstring>  // strlen(), strcmp(), strncmp()
#include <cstdlib>  // atoi()
#include <cctype>   // tolower()
}


/** @brief Model の保持値（電圧・位相）を View 表示へ一括同期する内部ユーティリティ
 *  @note  activate()/setDesiredValue() から呼び出され、表示の一貫性を保つ。
 */
namespace {
inline void updateBothValuesFromModel(Model* m, MainView& v) {
    if (!m) return;

    const uint8_t id = AppRemote_GetID();

    const uint32_t vVolt = m->getDesiredValue(SettingType::Voltage, id);
    const uint32_t vCurr = m->getDesiredValue(SettingType::Current, id);
    const uint32_t vPhas = m->getDesiredValue(SettingType::Phase,   id);

    v.updateBothValues(vVolt, vCurr, vPhas, id);
    v.setDipHex(static_cast<uint8_t>(id & 0x0F));
}
}

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

    if (model) {
        model->setSynced(id, false);
    }

    updateBothValuesFromModel(model, view);

    const bool running = model ? model->isRunning(id) : false;
    view.notifyRunStopFromCLI(running);

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




#include "rs485_bridge.h"  // RS485_PcHasPending() を使う

/** @brief リモート子機の最新計測値を View に反映
 *  @details
 *   - 電圧: 子機の MEAS:VOLT? 応答を実電圧[mV]として取得し、
 *           View::setMeasuredVolt_Phys_mV() へ渡す
 *   - 電流: 子機の MEAS:CURR? 応答を実電流[mA]として取得し、
 *           View::setMeasuredCurr_mA() へ渡す
 *   - 通信失敗時は各表示を "--" にする
 */
void MainPresenter::updateMeasuredValues()
{
    if (RS485_IsBusy()) return;

    static uint32_t lastProbeTick[16] = {0};
    static uint8_t  failStreak[16]    = {0};

    static int32_t  lastVolt_mV[16]   = {0};
    static int32_t  lastCurr_mA[16]   = {0};
    static uint8_t  hasVolt[16]       = {0};
    static uint8_t  hasCurr[16]       = {0};

    const uint8_t id = AppRemote_GetID();
    const uint32_t now = HAL_GetTick();

    const bool likelyAlive = model ? model->isLikelyAlive(id, now, 1500) : false;

    const uint32_t minPeriodAlive = 150u;
    if (likelyAlive) {
        if ((int32_t)(now - lastProbeTick[id]) < (int32_t)minPeriodAlive) return;
    }

    uint32_t backoff = 0;
    if (!likelyAlive || failStreak[id] > 0) {
        uint8_t s = failStreak[id];
        if (s > 4) s = 4;
        backoff = 80u << s;
        if ((int32_t)(now - lastProbeTick[id]) < (int32_t)backoff) return;
    }

    const uint32_t to_ms = 400u;

    int32_t volt_phys_mv = 0;
    int32_t curr_ma      = 0;

    const bool okVolt = AppRemote_MeasVolt(&volt_phys_mv, to_ms);
    const bool okCurr = AppRemote_MeasCurr(&curr_ma, to_ms);

    lastProbeTick[id] = now;

    if (okVolt) {
        lastVolt_mV[id] = volt_phys_mv;
        hasVolt[id] = 1;
        view.setMeasuredVolt_Phys_mV(volt_phys_mv);
    } else if (hasVolt[id]) {
        view.setMeasuredVolt_Phys_mV(lastVolt_mV[id]);
    } else {
        view.setMeasuredVolt_Phys_mV(INT32_MIN);
    }

    if (okCurr) {
        lastCurr_mA[id] = curr_ma;
        hasCurr[id] = 1;
        view.setMeasuredCurr_mA(curr_ma);
    } else if (hasCurr[id]) {
        view.setMeasuredCurr_mA(lastCurr_mA[id]);
    } else {
        view.setMeasuredCurr_mA(INT32_MIN);
    }

    if (okVolt || okCurr) {
        if (model) model->noteAlive(id, now);
        failStreak[id] = 0;
    } else {
        if (failStreak[id] < 8) ++failStreak[id];

        /* たとえば3回以上連続で両方失敗したら無効表示 */
        if (failStreak[id] >= 3) {
            hasVolt[id] = 0;
            hasCurr[id] = 0;
            view.setMeasuredVolt_Phys_mV(INT32_MIN);
            view.setMeasuredCurr_mA(INT32_MIN);
        }
    }
}




// ==========================================================
// RS-485受信イベント：UIに反映
// ==========================================================
#include <cstring>
#include <cstdio>
#include <cctype>


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


    /* STAT_* は子機から返った確認済み設定値を表す。
     * ここで Model::desired... を親機キャッシュとして確定更新する。
     */
    switch (ev.type) {
    case APPREMOTE_EVT_RUN: {
        const uint32_t now = HAL_GetTick();

        if (model) {
            model->setRunning(id, true);
            model->noteAlive(id, now);
            model->setSynced(id, true);
        }

        if (id == current) {
            view.notifyRunStopFromCLI(true);
        }
        break;
    }

    case APPREMOTE_EVT_STOP: {
        const uint32_t now = HAL_GetTick();

        if (model) {
            model->setRunning(id, false);
            model->noteAlive(id, now);
            model->setSynced(id, true);
        }

        if (id == current) {
            view.notifyRunStopFromCLI(false);
        }
        break;
    }

    case APPREMOTE_EVT_STAT_VOLT: {
        const uint32_t now = HAL_GetTick();
        if (model) {
            model->setDesiredValue(SettingType::Voltage, ev.value, id);
            model->noteAlive(id, now);
            model->setSynced(id, true);
        }
        break;
    }

    case APPREMOTE_EVT_STAT_CURR: {
        const uint32_t now = HAL_GetTick();
        if (model) {
            model->setDesiredValue(SettingType::Current, ev.value, id);
            model->noteAlive(id, now);
            model->setSynced(id, true);
        }
        break;
    }

    case APPREMOTE_EVT_STAT_PHAS: {
        const uint32_t now = HAL_GetTick();
        if (model) {
            model->setDesiredValue(SettingType::Phase, ev.value, id);
            model->noteAlive(id, now);
            model->setSynced(id, true);
        }
        break;
    }

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

    case SettingType::Current:
        ok = AppRemote_SetCurr(v);
        break;

    case SettingType::Phase:
        ok = AppRemote_SetPhas((uint16_t)v);
        break;

    default:
        break;
    }

    if (ok) {
        if (model) model->setSynced(id, false);
        AppRemote_QueryState();
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
        if (model) model->setSynced(id, false);
        AppRemote_QueryState();
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
        if (model) model->setSynced(id, false);
        AppRemote_QueryState();
    }
}

