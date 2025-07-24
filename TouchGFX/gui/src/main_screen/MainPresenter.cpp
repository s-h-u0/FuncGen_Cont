#include <gui/main_screen/MainPresenter.hpp>
#include <gui/main_screen/MainView.hpp>
#include <gui/model/Model.hpp>

MainPresenter::MainPresenter(MainView& v) : view(v) {}

void MainPresenter::activate()
{
    if (!model) return;

    // ★常に両方取得して View に渡す
    uint32_t vVolt = model->getDesiredValue(SettingType::Voltage);
    uint32_t vPhas = model->getDesiredValue(SettingType::Phase);
    view.updateBothValues(vVolt, vPhas);
}

void MainPresenter::deactivate() {}

void MainPresenter::setDesiredValue(SettingType t, uint32_t v)
{
    if (model) model->setDesiredValue(t, v);

    // 変更後も両方表示を最新化
    uint32_t vVolt = model->getDesiredValue(SettingType::Voltage);
    uint32_t vPhas = model->getDesiredValue(SettingType::Phase);
    view.updateBothValues(vVolt, vPhas);
}

uint32_t MainPresenter::getDesiredValue(SettingType t) const
{
    return model ? model->getDesiredValue(t) : 0;
}

void MainPresenter::setCurrentSetting(SettingType s)
{
    if (model) model->setCurrentSetting(s);
}

SettingType MainPresenter::getCurrentSetting() const
{
    return model ? model->getCurrentSetting() : SettingType::Voltage;
}
