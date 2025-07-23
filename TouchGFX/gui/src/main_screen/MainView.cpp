#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <dpot_AD5292.h>
#include <algorithm>

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

void MainView::Run()
{
    // 今回は常に 500Ω をセット
    constexpr uint32_t defaultOhms = 500;
    AD5292_Set(defaultOhms);
}
