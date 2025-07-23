#pragma once
#include <mvp/Presenter.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstdint>

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

private:
    MainView& view;
    uint32_t  desiredValue{0};
};
