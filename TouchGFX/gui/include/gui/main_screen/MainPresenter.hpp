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

private:
    MainView& view;

};
