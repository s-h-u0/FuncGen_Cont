#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <dpot_AD5292.h>
#include <algorithm>
#include <gui/common/SettingType.hpp>


MainView::MainView()
    : MainViewBase()
{}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();
    if (presenter) {
        presenter->activate();
    }
}

void MainView::tearDownScreen()
{
    if (presenter) {
        presenter->deactivate();
    }
    MainViewBase::tearDownScreen();
}


void MainView::updateSetVolt(uint32_t value)
{
    constexpr size_t BUF_SIZE = 11;
    static touchgfx::Unicode::UnicodeChar buf[BUF_SIZE];

    Unicode::snprintf(buf, BUF_SIZE, "%u", static_cast<unsigned>(value));
    Val_Set_Volt.setWildcard(buf);
    Val_Set_Volt.invalidate();
}

// MainView.cpp
void MainView::Run()
{
    // カウンタを1増やす
    toggleCounter++;

    // 奇数回目は 0Ω、偶数回目は 1000Ω を設定
    uint32_t ohms = (toggleCounter % 2 == 1) ? 0x400 : 0x07FF ;

    AD5292_Set(ohms);
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
