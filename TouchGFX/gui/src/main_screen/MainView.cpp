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

void MainView::Run()
{
    // 今回は常に 500Ω をセット
    constexpr uint32_t defaultOhms = 500;
    AD5292_Set(defaultOhms);
}
