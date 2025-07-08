#include "main.h"  // htim4 の宣言を取り込む
#include <gui/main_screen/MainView.hpp>

MainView::MainView() { }

void MainView::setupScreen()
{
    MainViewBase::setupScreen();  // 親の初期化を呼ぶ
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::sliderValueChanged(int value)
{
    // value は 0～255 の範囲
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, value);
}
