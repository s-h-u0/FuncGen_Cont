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
void MainPresenter::updateMeasuredValues()
{
    if (!adcHandle) return;

    MCP3428_SetConfig(adcHandle,
        MCP3428_CHANNEL_4,
        MCP3428_RESOLUTION_16BIT,
        MCP3428_MODE_ONESHOT,
        MCP3428_GAIN_1X);

    int16_t mv = MCP3428_ReadMilliVolt(adcHandle);
    view.setMeasuredVolt_mV(mv);   // ★ 小数表示版を呼ぶ
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




