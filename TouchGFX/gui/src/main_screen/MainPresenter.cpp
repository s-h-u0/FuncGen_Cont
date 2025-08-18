#include <gui/main_screen/MainPresenter.hpp>
#include <gui/main_screen/MainView.hpp>
#include <gui/model/Model.hpp>

extern "C" {
#include "adc_MCP3428.h"
}

MainPresenter::MainPresenter(MainView& v) : view(v) {}

void MainPresenter::activate()
{
    if (!model) return;

    uint32_t vVolt = model->getDesiredValue(SettingType::Voltage);
    uint32_t vPhas = model->getDesiredValue(SettingType::Phase);
    view.updateBothValues(vVolt, vPhas);
}

void MainPresenter::deactivate() {}

void MainPresenter::setDesiredValue(SettingType t, uint32_t v)
{
    if (model) model->setDesiredValue(t, v);

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

void MainPresenter::setADCHandle(MCP3428_HandleTypeDef* h)
{
    adcHandle = h;   // ← ここを hadc3428 ではなく adcHandle に
}

void MainPresenter::updateMeasuredValues()
{
    if (!adcHandle) return;

    MCP3428_SetConfig(adcHandle,
        MCP3428_CHANNEL_4,
        MCP3428_RESOLUTION_16BIT,
        MCP3428_MODE_ONESHOT,
        MCP3428_GAIN_1X);

    int16_t mv = MCP3428_ReadMilliVolt(adcHandle);
    view.setMeasuredVolt_mV(mv);   // ★ 小数表示版を呼ぶ
}

