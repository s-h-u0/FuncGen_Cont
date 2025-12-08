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
#include "adc_MCP3428.h"
#include "dipsw_221AMA16R.h"
#include "remote_client.h"
#include <cstdio>   // printf()
#include <cstring>  // strlen(), strcmp(), strncmp()
#include <cstdlib>  // atoi()
#include <cctype>   // tolower()
#include "rs485_bridge.h"
}

extern uint8_t g_currentID;

/** @brief Model の保持値（電圧・位相）を View 表示へ一括同期する内部ユーティリティ
 *  @note  activate()/setDesiredValue() から呼び出され、表示の一貫性を保つ。
 */
namespace {
inline void updateBothValuesFromModel(Model* m, MainView& v) {
    if (!m) return;

    const uint32_t vVolt = m->getDesiredValue(SettingType::Voltage, g_currentID);
    const uint32_t vPhas = m->getDesiredValue(SettingType::Phase,   g_currentID);

    v.updateBothValues(vVolt, vPhas, g_currentID);  // ← vIDの代わりにg_currentIDを渡す
    v.setDipHex(static_cast<uint8_t>(g_currentID & 0x0F));
}

}


/** @brief コンストラクタ：対応する View を束縛 */
MainPresenter::MainPresenter(MainView& v) : view(v) {}


/** @brief 画面表示の初期同期（Model → View） */
void MainPresenter::activate()
{
    updateBothValuesFromModel(model, view);
    remote_query_state(); // 応答は既存 onRemoteLine() 経由でModel/表示に反映


}


/** @brief 画面終了時のライフサイクルフック（現状処理なし） */
void MainPresenter::deactivate() {}


/** @brief 設定値を Model に反映し、View 表示も直ちに同期
 *  @param t 設定項目（電圧/位相など）
 *  @param v 設定値
 */
void MainPresenter::setDesiredValue(SettingType t, uint32_t v)
{
	if (model) model->setDesiredValue(t, v, g_currentID);
    updateBothValuesFromModel(model, view);
}


/** @brief Modelから設定値(PVではなく設定値)を取得
 *  @param t 取得対象の種別（電圧/位相など）
 *  @return 設定値（存在しなければ0）
 */
uint32_t MainPresenter::getDesiredValue(SettingType t) const
{
	return model ? model->getDesiredValue(t, g_currentID) : 0;

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


/** @brief ADCデバイスのハンドルを Presenter に登録（以後の測定で使用） */
void MainPresenter::setADCHandle(MCP3428_HandleTypeDef* h)
{
    adcHandle = h;   // ← ここを hadc3428 ではなく adcHandle に
}


/** @brief MCP3428 を 1回測定設定にして mV を取得 → View に反映
 *  @details
 *   - 設定: CHANNEL_4 / 16BIT / ONESHOT / GAIN_1X
 *   - 表示: View::setMeasuredVolt_mV(mV) で "X.YY" に整形して表示
 */
#include "rs485_bridge.h"  // RS485_PcHasPending() を使う

void MainPresenter::updateMeasuredValues()
{

    // ② 他のUI往復中ならやめる（既存）
    if (RS485_IsBusy()) return;

    static uint32_t lastProbeTick[16] = {0};
    static uint8_t  failStreak  [16] = {0};

    const uint8_t  id  = g_currentID & 0x0F;
    const uint32_t now = HAL_GetTick();

    const bool likelyAlive = model ? model->isLikelyAlive(id, 1500) : false;

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
    const uint32_t to_ms = likelyAlive ? 120u : 80u;

    int32_t mv = 0;
    const bool ok = remote_meas_volt_mV_to(&mv, to_ms);

    lastProbeTick[id] = now;

    if (ok) {
        if (mv >  32767) mv =  32767;
        if (mv < -32768) mv = -32768;
        view.setMeasuredVolt_mV((int16_t)mv);
        if (model) model->noteAlive(id);
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

    // 小文字化
    char buf[64];
    size_t len = strlen(line);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    for (size_t i = 0; i < len; ++i) buf[i] = (char)tolower((unsigned char)line[i]);
    buf[len] = '\0';

    // ID抽出
    uint8_t id = 0xFF;
    const char* p = buf;
    if ((p[0] == '#' || p[0] == '@') && isxdigit((unsigned char)p[1])) {
        id = (uint8_t)strtoul(&p[1], nullptr, 16) & 0x0F;
        p += 2;
        while (*p == ' ' || *p == ':') ++p;
    }

    // パターン
    if (strcmp(p, "run") == 0) {
        view.notifyRunStopFromCLI(true);
    } else if (strcmp(p, "stop") == 0) {
        view.notifyRunStopFromCLI(false);
    } else if (strncmp(p, "stat:volt", 9) == 0) {
        p += 9; while (*p == ' ' || *p == ':') ++p;
        uint32_t v = (uint32_t)(atoi(p) / 1000);
        if (id != 0xFF && model) model->setDesiredValue(SettingType::Voltage, v, id);
    } else if (strncmp(p, "stat:phas", 9) == 0) {
        p += 9; while (*p == ' ' || *p == ':') ++p;
        uint32_t deg = (uint32_t)atoi(p);
        if (id != 0xFF && model) model->setDesiredValue(SettingType::Phase, deg, id);
    } else {
        view.showRemoteLine(line); // ログのみ
    }

    // ★表示中IDだけ即描画（遅延なし）
    if (id != 0xFF && id == g_currentID) {
        updateBothValuesFromModel(model, view); // Textをinvalidateまで面倒見てくれる
        view.triggerRefreshFromPresenter();     // 画面再描画
    }
}




void MainPresenter::setDesiredValueByID(uint8_t id, SettingType t, uint32_t v)
{
    if (model)
        model->setDesiredValue(t, v, id);

    // ★ IDが有効か確認（0xFFは無効値）し、かつ表示中IDのみ再描画
    if (id != 0xFF && id == g_currentID) {
        updateBothValuesFromModel(model, view);
        view.triggerRefreshFromPresenter();
    }
}




