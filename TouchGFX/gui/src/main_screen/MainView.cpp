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
    uint32_t v = presenter ? presenter->getDesiredValue() : 0;
    v = std::min<uint32_t>(v, AD5292_FULL_SCALE_OHMS);
    AD5292_Set(v);
}
