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
    // presenter->activate();  // Presenter 側の activate 内で View を更新しているなら不要
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
        // 120ms以内の再タッチは無視
        return;
    }
    lastTick = now;

    AD5292_Set(0x7FF); //とりあえず最大値20kΩ
}

void MainView::Stop()
{
    // デバウンス用タイマー
    static uint32_t lastTick = 0;
    uint32_t now = HAL_GetTick();
    if (now - lastTick < 800) {
        // 120ms以内の再タッチは無視
        return;
    }
    lastTick = now;

    AD5292_Set(0x400); //とりあえず最小値0Ω
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
