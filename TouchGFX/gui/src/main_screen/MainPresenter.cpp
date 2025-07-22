#include <gui/main_screen/MainPresenter.hpp>
#include <gui/main_screen/MainView.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{
    view.bindPresenter(this);
}

void MainPresenter::activate()   {}
void MainPresenter::deactivate() {}

void MainPresenter::setDesiredValue(uint32_t v) { desiredValue = v; }
uint32_t MainPresenter::getDesiredValue() const { return desiredValue; }
