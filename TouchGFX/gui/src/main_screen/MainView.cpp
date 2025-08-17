#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <dpot_AD5292.h>
#include <touchgfx/Unicode.hpp>
#include <algorithm>
#include "main.h"            // HAL_GetTick(), GPIO など
#include "stm32f4xx_hal.h"
#include <dds_AD9833.h>
#include "meas_timer.h"



MainView::MainView()
    : MainViewBase()
{}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    isRunning = false;

    // タッチ可否
    toggleButton_Run .setTouchable(true);
    toggleButton_Stop.setTouchable(false);

    // ★ ここを forceState で初期化 ★
    toggleButton_Run .forceState(false);  // ReleasedImage（RUN）
    toggleButton_Stop.forceState(true);   // PressedImage （STOP）

    toggleButton_Run .invalidate();
    toggleButton_Stop.invalidate();

    extern MCP3428_HandleTypeDef hadc3428;   // main.c のグローバル変数
    presenter->setADCHandle(&hadc3428);


}




void MainView::tearDownScreen()
{
    MeasTimer_Stop();
    MainViewBase::tearDownScreen();
}

void MainView::updateBothValues(uint32_t vVolt, uint32_t vPhas)
{
    // ★ Volt 側
    Unicode::snprintf(
        Val_Set_VoltBuffer,
        VAL_SET_VOLT_SIZE,
        "%u",
        static_cast<unsigned>(vVolt)
    );
    Val_Set_Volt.invalidate();

    // ★ Phase 側
    Unicode::snprintf(
        Val_Set_PhasBuffer,
        VAL_SET_PHAS_SIZE,
        "%u",
        static_cast<unsigned>(vPhas)
    );
    Val_Set_Phas.invalidate();
}

void MainView::Run()
{
    // ★ デバウンス用タイマー
    static uint32_t lastRunTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastRunTick < 800) {
        // 800ms以内の再タッチは無視
        return;
    }
    lastRunTick = now;

    uint32_t vPhase = presenter->getDesiredValue(SettingType::Phase);      // deg
    // ※波形を UI で選べるようにしていれば presenter->getDesiredWave() などで取得
    AD9833_Set(50, AD9833_SINE, vPhase);

    // モデルから入力済みの電圧を取得
    uint32_t vVolt = presenter->getDesiredValue(SettingType::Voltage);  // 0…40V
    AD5292_SetVoltage(vVolt);

    // すでに Run 中なら何もしない
    if (isRunning) {
        return;
    }
    isRunning = true;

    // ★ 見た目を切り替え
    toggleButton_Run .forceState(true);  // PressedImage
    toggleButton_Stop.forceState(false); // ReleasedImage

    // ★ タッチ可否を排他切り替え
    toggleButton_Run .setTouchable(false);
    toggleButton_Stop.setTouchable(true);

    // ★ 再描画
    toggleButton_Run .invalidate();
    toggleButton_Stop.invalidate();

}

void MainView::Stop()
{
    // ★ デバウンス用タイマー
    static uint32_t lastStopTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastStopTick < 800) {
        // 800ms以内の再タッチは無視
        return;
    }
    lastStopTick = now;

    // Run 中でなければ何もしない
    if (!isRunning) {
        return;
    }
    isRunning = false;

    // ★ 見た目を切り替え
    toggleButton_Stop.forceState(true);  // PressedImage
    toggleButton_Run .forceState(false); // ReleasedImage

    // ★ タッチ可否を排他切り替え
    toggleButton_Stop.setTouchable(false);
    toggleButton_Run .setTouchable(true);

    // ★ 再描画
    toggleButton_Stop.invalidate();
    toggleButton_Run .invalidate();

    // 実際の動作
    AD5292_Set(0x400); // 最小値設定
}




void MainView::button_VoltClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Voltage);
        application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::button_PhasClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Phase);
        application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::setMeasuredCurr(int16_t val)
{
    Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "%d", static_cast<int>(val));
    Val_Meas_Curr.resizeToCurrentText();
    Val_Meas_Curr.invalidate();
}

void MainView::setMeasuredVolt(int16_t val)
{
    Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "%d", static_cast<int>(val));
    Val_Meas_Volt.resizeToCurrentText();
    Val_Meas_Volt.invalidate();
}


void MainView::handleTickEvent()
{
    static int cnt = 0;
    if (MeasTimer_Consume()) {      // 1秒ごと
        setMeasuredVolt(cnt % 100); // ←整数で確実に変わる
        cnt++;
    }
    MainViewBase::handleTickEvent();
}

