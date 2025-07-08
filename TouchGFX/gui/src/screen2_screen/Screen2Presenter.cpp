#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui_generated/main_screen/MainViewBase.hpp>
#include "main.h"

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{

}

void Screen2Presenter::deactivate()
{

}


void Screen2View::LED_ONPressed()
{
    //Screen1ViewBase::tearDownScreen();
	HAL_GPIO_WritePin(PG13_GPIO_Port,PG13_Pin,GPIO_PIN_SET);
}

void Screen2View::LED_OFFPressed()
{
    //Screen1ViewBase::tearDownScreen();
	HAL_GPIO_WritePin(PG13_GPIO_Port,PG13_Pin,GPIO_PIN_RESET);
}

void Screen2View::Toggle_LEDPressed()
{
    //Screen1ViewBase::tearDownScreen();
	HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_14);
}


