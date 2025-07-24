#pragma once
#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>
#include <gui/common/SettingType.hpp>



// ---------- 前方宣言 ----------
class KeyboardView;

//--------------------------------
class KeyboardPresenter : public touchgfx::Presenter,
                          public ModelListener
{
public:
    explicit KeyboardPresenter(KeyboardView& v);

    virtual void activate() override;  // ← ★追加

    void onDigit(uint8_t d);
    void onDelete();
    void onEnter();
    void reset();

    uint32_t getCurrentValue() const;   // ← 宣言だけ

private:
    KeyboardView& view;
    uint32_t      currentValue{0};
    static constexpr uint32_t MAX_INPUT = 9999;

};
