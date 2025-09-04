
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

    /// ADC( MCP3428 ) を 1回測定→mV取得→View反映（小数2桁表示は View 側）
    void updateMeasuredValues();

    /** DIPスイッチ(4bit)の値を読み取り、Viewへ16進表示で反映 */
    void updateDipValue();

private:
    MainView& view;
    // ここに MCP3428 用ハンドルを保持
    MCP3428_HandleTypeDef* adcHandle = nullptr;  ///< ここを追加
};
