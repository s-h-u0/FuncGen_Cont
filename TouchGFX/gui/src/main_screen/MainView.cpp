#include "main.h"
#include <gui/main_screen/MainView.hpp>

MainView::MainView()
    : MainViewBase()  // ベースクラスのコンストラクタ呼び出し
{
    // 必要ならここで自分側の初期化を。
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::sliderValueChanged(int sliderValue)
{
    // 例：PWM とゲージの更新
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, sliderValue);

    int mapped = sliderValue * 50 / 255;  // 0–255 → 0–50 に線形マッピング
    gauge1.setValue(mapped);
    gauge1.invalidate();
}
