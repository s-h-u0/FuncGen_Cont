#pragma once
// MainPresenter を先に宣言しておく（テンプレート依存の解決）
class MainPresenter;

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <touchgfx/Unicode.hpp>
#include <gui/model/Model.hpp>


class MainView : public MainViewBase
{
public:
	MainView();
    void setupScreen()    override;
    void tearDownScreen() override;
    void Run()            override;

    /* Presenter から数値を描画 */
    void updateSetVolt(uint32_t v);

    void button_VoltClicked() override;
    void button_PhasClicked() override;

private:
    uint32_t toggleCounter = 0;
};
