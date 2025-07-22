#pragma once
#include <mvp/Presenter.hpp>            // ← ここを修正
#include <gui/model/ModelListener.hpp>
#include <cstdint>

class MainView;

/**
 * @brief  測定値／設定値の仲介を行う Presenter
 */
class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    explicit MainPresenter(MainView& v);

    void activate()   override;
    void deactivate() override;

    void setDesiredValue(uint32_t v);
    uint32_t getDesiredValue() const;

private:
    MainView& view;
    uint32_t  desiredValue {0};
};
