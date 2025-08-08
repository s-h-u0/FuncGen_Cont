
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


    // ★ 追加（MainView::setupScreen() で呼ばれてるやつ）
    void setADCHandle(MCP3428_HandleTypeDef* h);

    void activate()   override;
    void deactivate() override;

    // 種類を指定して値を保存/取得
    void     setDesiredValue(SettingType t, uint32_t v);
    uint32_t getDesiredValue(SettingType t) const;

    void setCurrentSetting(SettingType s);
    SettingType getCurrentSetting() const;

    // ★ 追加（1秒ごとに MainView から呼ばれるやつ）
    void updateMeasuredValues();

private:
    MainView& view;
    // ここに MCP3428 用ハンドルを保持
    MCP3428_HandleTypeDef* adcHandle = nullptr;  ///< ここを追加
};
