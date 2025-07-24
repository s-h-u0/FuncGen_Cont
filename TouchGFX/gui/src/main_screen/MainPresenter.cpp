#include <gui/main_screen/MainPresenter.hpp>
#include <gui/main_screen/MainView.hpp>
#include <gui/common/SettingType.hpp>

MainPresenter::MainPresenter(MainView& v) : view(v) {}

void MainPresenter::activate()
{
    if (model) {
        desiredValue = model->getDesiredValue();  // Model から取得
    }
    view.updateSetVolt(desiredValue);             // View 表示更新
}

void MainPresenter::deactivate() {}

void MainPresenter::setDesiredValue(uint32_t v)
{
    desiredValue = v;
    view.updateSetVolt(desiredValue);
}

uint32_t MainPresenter::getDesiredValue() const
{
    return desiredValue;
}




void MainPresenter::setCurrentSetting(SettingType s)
 {
     if (model) model->setCurrentSetting(s);
 }

SettingType MainPresenter::getCurrentSetting() const
 {
     return model ? model->getCurrentSetting() : SettingType::Voltage;
 }


