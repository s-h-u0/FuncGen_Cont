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


    // RS-485からの1行通知（Modelから届く）
    void onRemoteLine(const char* line) override;



    // 種類を指定して値を保存/取得
    void     setDesiredValue(SettingType t, uint32_t v);
    uint32_t getDesiredValue(SettingType t) const;

    void setDesiredValueByID(uint8_t id, SettingType t, uint32_t v);

    void setCurrentSetting(SettingType s);
    SettingType getCurrentSetting() const;

    /// ADC( MCP3428 ) を 1回測定→mV取得→View反映（小数2桁表示は View 側）
    void updateMeasuredValues();

    /** DIPスイッチ(4bit)の値を読み取り、Viewへ16進表示で反映 */
    void updateDipValue();

    void runCurrent();
    void stopCurrent();

private:
    MainView& view;
};
