#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <dpot_AD5292.h>
#include <touchgfx/Unicode.hpp>
#include <algorithm>
#include "main.h"            // HAL_GetTick(), GPIO など
#include "stm32f4xx_hal.h"



MainView::MainView()
    : MainViewBase()
{}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();
    // 初期状態：Run ボタンだけ有効、Stop ボタンは非活性
    isRunning = false;
    toggleButton_Run.setTouchable(true);
    toggleButton_Run.invalidate();
    toggleButton_Stop.setTouchable(false);
    toggleButton_Stop.invalidate();
}


void MainView::tearDownScreen()
{
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
    // デバウンス用タイマー
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastTick < 800) {
        return;
    }
    lastTick = now;

    // すでに Run 中なら何もしない
    if (isRunning) {
        return;
    }
    isRunning = true;

    // ボタンの切り替え
    toggleButton_Run.setTouchable(false);
    toggleButton_Run.invalidate();
    toggleButton_Stop.setTouchable(true);
    toggleButton_Stop.invalidate();

    // 実際の動作
    AD5292_Set(0x7FF); // 最大値設定
}

void MainView::Stop()
{
    // デバウンス用タイマー
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastTick < 800) {
        return;
    }
    lastTick = now;

    // Run 中でなければ何もしない
    if (!isRunning) {
        return;
    }
    isRunning = false;

    // ボタンの切り替え
    toggleButton_Stop.setTouchable(false);
    toggleButton_Stop.invalidate();
    toggleButton_Run.setTouchable(true);
    toggleButton_Run.invalidate();

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
