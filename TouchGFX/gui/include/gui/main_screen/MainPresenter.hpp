#pragma once
#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>
#include <gui/common/SettingType.hpp>

// ---------- 前方宣言 ----------
class MainView;



//--------------------------------
class MainPresenter : public touchgfx::Presenter,
                      public ModelListener
{
public:
    explicit MainPresenter(MainView& v);   // ★ 引数は View のみ

    void activate()   override;
    void deactivate() override;

    /* ModelListener からも呼べる汎用 API */
    void      setDesiredValue(uint32_t v);
    uint32_t  getDesiredValue() const;


    void setCurrentSetting(SettingType s);
    SettingType getCurrentSetting() const;

private:
    MainView& view;
    uint32_t  desiredValue{0};
    SettingType currentSetting = SettingType::Voltage;  // ← 追加
};
