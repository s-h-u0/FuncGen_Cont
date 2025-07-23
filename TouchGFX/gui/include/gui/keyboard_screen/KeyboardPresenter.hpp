#pragma once
#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>

class KeyboardView;

/**
 * Keyboard 画面の Presenter。
 * 直接 touchgfx::Presenter を継承し、モデルが不要なら ModelListener は省略。
 */
class KeyboardPresenter : public touchgfx::Presenter,
                          public ModelListener
{
public:
    explicit KeyboardPresenter(KeyboardView& v);
    virtual ~KeyboardPresenter() {}

    // 画面表示／非表示
    virtual void activate()   override {}
    virtual void deactivate() override {}

    // 数字キー、Delete、Enter
    void onDigit(uint8_t d);
    void onDelete();
    void onEnter();

    // 値の取得／クリア
    uint32_t getCurrentValue() const { return currentValue; }
    void     reset();

private:
    KeyboardView& view;
    uint32_t      currentValue {0};
    static constexpr uint32_t MAX_INPUT = 99'999'999;
};
