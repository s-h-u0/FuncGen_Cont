#pragma once
#include <gui_generated/main_screen/MainViewBase.hpp>

class MainPresenter;

/**
 * @brief  メイン画面 View
 */
class MainView : public MainViewBase
{
public:
    MainView();

    void setupScreen()    override;
    void tearDownScreen() override;
    void Run()            override;   // トグルボタン押下で呼ばれる

    void bindPresenter(MainPresenter* p) { presenter = p; }

private:
    MainPresenter* presenter {nullptr};
};
