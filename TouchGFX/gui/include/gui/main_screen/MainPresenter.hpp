
#ifdef __cplusplus
extern "C" {
#endif
#include "adc_MCP3428.h"    // Core/Inc にあるヘッダへの相対パスを調整
#ifdef __cplusplus
}
#endif


#pragma once
#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/SettingType.hpp>
#include <cstdint>

class MainView;

class MainPresenter : public touchgfx::Presenter,
                      public ModelListener
{
public:
    explicit MainPresenter(MainView& v);

    void activate()   override;
    void deactivate() override;

    // 種類を指定して値を保存/取得
    void     setDesiredValue(SettingType t, uint32_t v);
    uint32_t getDesiredValue(SettingType t) const;

    void setCurrentSetting(SettingType s);
    SettingType getCurrentSetting() const;

    void updateMeasuredValues();

    /// <summary>ADC ハンドルをセット</summary>
    void setADCHandle(MCP3428_HandleTypeDef* handle) { adcHandle = handle; }

private:
    MainView& view;
    // ここに MCP3428 用ハンドルを保持
    MCP3428_HandleTypeDef* adcHandle = nullptr;  ///< ここを追加
};
