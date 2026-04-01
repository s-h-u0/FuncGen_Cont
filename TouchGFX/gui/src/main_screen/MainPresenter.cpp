/**************************************************************
 * @file    MainPresenter.cpp
 * @brief   Main 画面の Presenter（View と Model の仲介）
 * @details
 *  - 役割: View からの要求を Model/ドライバへ橋渡し、結果を View に反映
 *  - 測定フロー:
 *      * updateMeasuredValues():
 *          - MCP3428 を Ch4 / 16bit / OneShot / Gain1x で設定
 *          - mV を取得し View::setMeasuredVolt_mV() に渡す
 *  - 設定フロー:
 *      * setDesiredValue() で Model に保存 → View 表示を updateBothValues() で同期
 **************************************************************/


#include <gui/main_screen/MainPresenter.hpp>
#include <gui/main_screen/MainView.hpp>
#include <gui/model/Model.hpp>



extern "C" {
#include "dipsw_221AMA16R.h"
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
    const uint32_t vPhas = m->getDesiredValue(SettingType::Phase,   id);

    v.updateBothValues(vVolt, vPhas, id);
    v.setDipHex(static_cast<uint8_t>(id & 0x0F));
}

}


/** @brief コンストラクタ：対応する View を束縛 */
MainPresenter::MainPresenter(MainView& v) : view(v) {}


/** @brief 画面表示の初期同期（Model → View） */
void MainPresenter::activate()
{
    updateBothValuesFromModel(model, view);
    AppRemote_QueryState();
}


/** @brief 画面終了時のライフサイクルフック（現状処理なし） */
void MainPresenter::deactivate() {}


/** @brief 設定値を Model に反映し、View 表示も直ちに同期
 *  @param t 設定項目（電圧/位相など）
 *  @param v 設定値
 */
void MainPresenter::setDesiredValue(SettingType t, uint32_t v)
{
    const uint8_t id = AppRemote_GetID();
    bool ok = false;

    switch (t) {
    case SettingType::Voltage:
        ok = AppRemote_SetVolt(v);
        break;
    case SettingType::Phase:
        ok = AppRemote_SetPhas((uint16_t)v);
        break;
    default:
        break;
    }

    if (ok && model) {
        model->setDesiredValue(t, v, id);
    }

    updateBothValuesFromModel(model, view);
}

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




#include "rs485_bridge.h"  // RS485_PcHasPending() を使う

void MainPresenter::updateMeasuredValues()
{

    // ② 他のUI往復中ならやめる（既存）
    if (RS485_IsBusy()) return;

    static uint32_t lastProbeTick[16] = {0};
    static uint8_t  failStreak  [16] = {0};

    const uint8_t id = AppRemote_GetID();
    const uint32_t now = HAL_GetTick();

    const bool likelyAlive = model ? model->isLikelyAlive(id, now, 1500) : false;

    // ③ 生きてる時でも叩きすぎ防止（最小周期 150ms）
    const uint32_t minPeriodAlive = 150u;
    if (likelyAlive) {
        if ((int32_t)(now - lastProbeTick[id]) < (int32_t)minPeriodAlive) return;
    }

    // ④ 失敗が続く or 生存不明は指数バックオフ（80,160,320,640,1280ms）
    uint32_t backoff = 0;
    if (!likelyAlive || failStreak[id] > 0) {
        uint8_t s = failStreak[id];
        if (s > 4) s = 4;
        backoff = 80u << s;
        if ((int32_t)(now - lastProbeTick[id]) < (int32_t)backoff) return;
    }

    // ⑤ 叩く。死んでそうなら短TOで軽く、元気なら少し長め
    //const uint32_t to_ms = likelyAlive ? 120u : 80u;
    const uint32_t to_ms = 400u;

    int32_t mv = 0;
    const bool ok = AppRemote_MeasVolt(&mv, to_ms);

    lastProbeTick[id] = now;

    if (ok) {
        if (mv >  32767) mv =  32767;
        if (mv < -32768) mv = -32768;
        view.setMeasuredVolt_mV((int16_t)mv);
        if (model) model->noteAlive(id, now);
        failStreak[id] = 0;
    } else {
        view.setMeasuredVolt_mV(INT16_MIN);     // "--"
        if (failStreak[id] < 8) ++failStreak[id];
    }
}


void MainPresenter::updateDipValue()
{
    // 221-AMA16R（4bit）の状態を 0x0〜0xF で取得
    uint8_t v = DIP221_Read() & 0x0F;
    view.setDipHex(v);
}




// ==========================================================
// RS-485受信イベント：UIに反映
// ==========================================================
#include <cstring>
#include <cstdio>
#include <cctype>


void MainPresenter::onRemoteLine(const char* line)
{
    if (!line) return;

    AppRemote_Event ev;
    if (!AppRemote_ParseLine(line, &ev)) {
        view.showRemoteLine(line);
        return;
    }

    switch (ev.type) {
    case APPREMOTE_EVT_RUN: {
        const uint8_t id = AppRemote_GetID();
        const uint32_t now = HAL_GetTick();

        AppRemote_SetRunning(true);

        if (model) {
            model->setRunning(id, true);
            model->noteAlive(id, now);
        }

        view.notifyRunStopFromCLI(true);
        break;
    }

    case APPREMOTE_EVT_STOP: {
        const uint8_t id = AppRemote_GetID();
        const uint32_t now = HAL_GetTick();

        AppRemote_SetRunning(false);

        if (model) {
            model->setRunning(id, false);
            model->noteAlive(id, now);
        }

        view.notifyRunStopFromCLI(false);
        break;
    }

    case APPREMOTE_EVT_STAT_VOLT: {
        const uint32_t now = HAL_GetTick();

        if (ev.id != 0xFF && model) {
            model->setLastInputValue(SettingType::Voltage, ev.value, ev.id);
            model->noteAlive(ev.id, now);
        }
        break;
    }

    case APPREMOTE_EVT_STAT_PHAS: {
        const uint32_t now = HAL_GetTick();

        if (ev.id != 0xFF && model) {
            model->setLastInputValue(SettingType::Phase, ev.value, ev.id);
            model->noteAlive(ev.id, now);
        }
        break;
    }

    default:
        break;
    }

    const uint8_t current = AppRemote_GetID();
    if (ev.id != 0xFF && ev.id == current) {
        updateBothValuesFromModel(model, view);
        view.triggerRefreshFromPresenter();
    }
}


void MainPresenter::setDesiredValueByID(uint8_t id, SettingType t, uint32_t v)
{
    if (model)
        model->setDesiredValue(t, v, id);

    const uint8_t current = AppRemote_GetID();

    if (id != 0xFF && id == current) {
        updateBothValuesFromModel(model, view);
        view.triggerRefreshFromPresenter();
    }
}




