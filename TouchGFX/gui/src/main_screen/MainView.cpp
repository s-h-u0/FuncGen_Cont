#include <gui/main_screen/MainView.hpp>


MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // アニメーションを短く
    slideMenuLeft.setAnimationDuration(4);                        // 8フレーム ≒0.13s

    // イージングを線形補間に変更
    slideMenuLeft.setAnimationEasingEquation(
        touchgfx::EasingEquations::linearEaseNone);               // ← 正しい関数名と列挙名

    // 初期状態を「畳まれた」状態に
    slideMenuLeft.setState(touchgfx::SlideMenu::COLLAPSED);

    slideMenuLeft.setExpandedStateTimeout(0);      // 0 ⇒ 無限
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::collapseAllOtherSlideMenu(const touchgfx::SlideMenu& value)
{
    if (&value != &slideMenuLeft)
    {
        slideMenuLeft.animateToState(SlideMenu::COLLAPSED);
    }
}
