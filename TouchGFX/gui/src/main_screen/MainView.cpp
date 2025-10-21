/**************************************************************
 * @file    MainView.cpp
 **************************************************************/

#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <touchgfx/Unicode.hpp>
#include <algorithm>
#include "main.h"
#include "stm32f4xx_hal.h"

#include "meas_timer.h"
#include <touchgfx/Color.hpp>
#include <touchgfx/Application.hpp>

// extern "C" ブロックは行を分ける（プリプロセッサ用）
extern "C" {
#include <stdint.h>
#include "remote_client.h"   // ★ 追加：remote_* API をCリンケージで
#include <cstdio>
#include "rs485_bridge.h"   // ← これを追加
#include <cctype>

}





/* UI内部ヘルパ：Run/Stopの見た目 */
void MainView::updateRunStopUI(bool running)
{
    isRunning = running;

    toggleButton_Run .forceState(running);
    toggleButton_Stop.forceState(!running);
    toggleButton_Run .setTouchable(!running);
    toggleButton_Stop.setTouchable(running);

    // ★ 再描画
    toggleButton_Run.invalidate();
    toggleButton_Stop.invalidate();

    // ラベル色を更新
    if (running) {
        RUN_Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
        STOP_Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    } else {
        STOP_Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
        RUN_Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    }
    RUN_Text.invalidate();
    STOP_Text.invalidate();

    // スピナー制御
    if (running) {
        animatedImage1.setVisible(true);
        animatedImage1.startAnimation(false /*rev*/, true /*reset*/, true /*loop*/);
        animatedImage1.invalidate();
    } else {
        animatedImage1.stopAnimation();
        animatedImage1.invalidate();
        animatedImage1.setVisible(false);
    }
}

/* コンストラクタ */
MainView::MainView()
    : MainViewBase()
{}

/* 画面初期化 */
void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // 数値テキストのワイルドカードを明示バインド
    Val_Set_Volt.setWildcard(Val_Set_VoltBuffer);
    Val_Set_Phas.setWildcard(Val_Set_PhasBuffer);

    // スピナー初期状態
    animatedImage1.stopAnimation();
    animatedImage1.setVisible(false);
    animatedImage1.invalidate();

    isRunning = false;
    updateRunStopUI(false);

    extern MCP3428_HandleTypeDef hadc3428;
    presenter->setADCHandle(&hadc3428);

    MeasTimer_Start();

    // 初期表示も明示反映
    updateBothValues(
        presenter->getDesiredValue(SettingType::Voltage),
        presenter->getDesiredValue(SettingType::Phase),
		remote_get_id()  // または g_currentID を extern 宣言して使う
    );

}

void MainView::showRemoteLine(const char* line)
{
    // 画面上に出さないならログだけでも可
    printf("[UI] RS485 line: %s\r\n", line ? line : "(null)");
    // 必要なら今後ここでテキストエリアに追記など
}

/* 画面終了 */
void MainView::tearDownScreen()
{
    MeasTimer_Stop();
    MainViewBase::tearDownScreen();
}

// MainView.cpp
void MainView::updateBothValues(uint32_t vVolt, uint32_t vPhas, uint32_t vID)
{
    // Voltage
    Unicode::snprintf(Val_Set_VoltBuffer, VAL_SET_VOLT_SIZE, "%u", vVolt);
    Val_Set_Volt.resizeToCurrentText();
    Val_Set_Volt.invalidate();

    // Phase
    Unicode::snprintf(Val_Set_PhasBuffer, VAL_SET_PHAS_SIZE, "%u", vPhas);
    Val_Set_Phas.resizeToCurrentText();
    Val_Set_Phas.invalidate();

    // ID (1桁Hex)
    setDipHex(static_cast<uint8_t>(vID & 0x0F));
}



/* タップ誤操作抑制ロック */
static uint32_t s_lockUntilTick = 0;
constexpr uint32_t kLockMs = 120;
inline bool locked() { return (int32_t)(HAL_GetTick() - s_lockUntilTick) < 0; }
inline void lockFor(uint32_t ms = kLockMs) { s_lockUntilTick = HAL_GetTick() + ms; }

/* Run/Stop ボタン */
void MainView::Run()
{
    if (locked() || isRunning) { updateRunStopUI(isRunning); return; }
    updateRunStopUI(true);
    lockFor(120);

    // 設定値は Enter で既に送られているので、ここでは RUN のみ
    remote_run();
}


void MainView::Stop()
{
    if (locked() || !isRunning) { updateRunStopUI(isRunning); return; }
    updateRunStopUI(false);
    lockFor(120);

    remote_stop();
}


/* 設定ボタン */
void MainView::button_VoltClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Voltage);
        application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::button_PhasClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Phase);
        application().gotoKeyboardScreenNoTransition();
    }
}

/* 測定表示 */
void MainView::setMeasuredCurr(int16_t val)
{
    Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "%d", static_cast<int>(val));
    Val_Meas_Curr.resizeToCurrentText();
    Val_Meas_Curr.invalidate();
}

void MainView::setMeasuredVolt(int16_t val)
{
    Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "%d", static_cast<int>(val));
    Val_Meas_Volt.resizeToCurrentText();
    Val_Meas_Volt.invalidate();
}

void MainView::setMeasuredVolt_mV(int16_t mv)
{
    if (mv == INT16_MIN) {
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "--");
        Val_Meas_Volt.resizeToCurrentText();
        Val_Meas_Volt.invalidate();
        return;
    }
    bool neg = (mv < 0);
    int v = (int)mv; if (v < 0) v = -v;
    int whole = v / 1000;
    int rem   = v % 1000;
    int frac2 = (rem + 5) / 10;
    if (frac2 == 100) { frac2 = 0; whole += 1; }
    if (neg) Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "-%d.%02d", whole, frac2);
    else     Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE,  "%d.%02d", whole, frac2);
    Val_Meas_Volt.resizeToCurrentText();
    Val_Meas_Volt.invalidate();
}

// --- MainView::handleTickEvent() 内に追加 ---
void MainView::handleTickEvent()
{
    if (MeasTimer_Consume()) {
        presenter->updateMeasuredValues();
    }
    MainViewBase::handleTickEvent();
}


/* DIP表示 */
void MainView::setDipHex(uint8_t nibble)
{
    ID.invalidate();
    const uint8_t v = nibble & 0x0F;
    IDBuffer[0] = (touchgfx::Unicode::UnicodeChar)((v < 10) ? ('0' + v) : ('A' + (v - 10)));
    IDBuffer[1] = 0;
    ID.resizeToCurrentText();
    ID.invalidate();
}

/* === CLI→UI 公開ラッパ === */
void MainView::notifyRunStopFromCLI(bool running)
{
    updateRunStopUI(running);
}




void MainView::button_IDClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::ID); // ← 今回追加したIDを設定
        application().gotoKeyboardScreenNoTransition();
    }
}






void MainView::requestRedraw()
{
    invalidate();  // ← これは MainViewBase(Screen) 内の protected を呼べる
}


void MainView::triggerRefreshFromPresenter()
{
    // 画面全体を再描画
    touchgfx::Rect fullRect(0, 0, 480, 320);
    // または touchgfx::HAL::DISPLAY_WIDTH / HEIGHT でもOK
    touchgfx::Application::getInstance()->invalidateArea(fullRect);
}
