#pragma once

#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>

class KeyboardView;

/**
 * @brief 数値入力 Presenter
 */
class KeyboardPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    KeyboardPresenter(KeyboardView& v);

    void activate() override;
    void deactivate() override;

    void onDigit(uint8_t d);
    void onDelete();
    void onEnter();
    void reset();

    uint32_t getCurrentValue() const;

    // ModelListener から継承した仮想関数
    void onSomeEvent() override;

private:
    KeyboardView& view;
    uint32_t currentValue {0};
};
