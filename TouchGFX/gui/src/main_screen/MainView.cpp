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
#include "app_remote.h"
#include <cstdio>
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


}

/* コンストラクタ */
MainView::MainView()
    : MainViewBase()
{}

/* 画面初期化 */
void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    Val_Set_Volt.setWildcard(Val_Set_VoltBuffer);
    Val_Set_Curr.setWildcard(Val_Set_CurrBuffer);
    Val_Set_Phas.setWildcard(Val_Set_PhasBuffer);

    MeasTimer_Start();

    updateBothValues(
        presenter->getDesiredValue(SettingType::Voltage),
        presenter->getDesiredValue(SettingType::Current),
        presenter->getDesiredValue(SettingType::Phase),
        AppRemote_GetID()
    );

    if (presenter) {
        updateSyncStateUI(presenter->isCurrentIdSynced());
    }
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
void MainView::updateBothValues(uint32_t vVolt, uint32_t vCurr, uint32_t vPhas, uint32_t vID)
{
    // Voltage
    Unicode::snprintf(Val_Set_VoltBuffer, VAL_SET_VOLT_SIZE, "%u", (unsigned)vVolt);
    Val_Set_Volt.resizeToCurrentText();
    Val_Set_Volt.invalidate();

    // Current
    Unicode::snprintf(Val_Set_CurrBuffer, VAL_SET_CURR_SIZE, "%u", (unsigned)vCurr);
    Val_Set_Curr.resizeToCurrentText();
    Val_Set_Curr.invalidate();

    // Phase
    Unicode::snprintf(Val_Set_PhasBuffer, VAL_SET_PHAS_SIZE, "%u", (unsigned)vPhas);
    Val_Set_Phas.resizeToCurrentText();
    Val_Set_Phas.invalidate();

    // ID
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
    lockFor(120);

    if (presenter) {
        presenter->runCurrent();
    }
}

void MainView::Stop()
{
    if (locked() || !isRunning) { updateRunStopUI(isRunning); return; }
    lockFor(120);

    if (presenter) {
        presenter->stopCurrent();
    }
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

void MainView::button_CurrClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Current);
        application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::button_IDClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::ID); // ← 今回追加したIDを設定
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

void MainView::setMeasuredVolt_Phys_mV(int32_t mv)
{
    if (mv == INT32_MIN) {
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "--");
        Val_Meas_Volt.resizeToCurrentText();
        Val_Meas_Volt.invalidate();
        return;
    }

    bool neg = (mv < 0);
    int32_t v = mv;
    if (v < 0) v = -v;

    int32_t whole = v / 1000;
    int32_t rem   = v % 1000;
    int32_t frac2 = (rem + 5) / 10;

    if (frac2 == 100) {
        frac2 = 0;
        whole += 1;
    }

    int whole_i = (int)whole;
    int frac2_i = (int)frac2;

    if (neg) {
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "-%d.%02d",
                          whole_i, frac2_i);
    } else {
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "%d.%02d",
                          whole_i, frac2_i);
    }

    Val_Meas_Volt.resizeToCurrentText();
    Val_Meas_Volt.invalidate();
}

void MainView::setMeasuredCurr_mA(int32_t ma)
{
    if (ma == INT32_MIN) {
        Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "--");
        Val_Meas_Curr.resizeToCurrentText();
        Val_Meas_Curr.invalidate();
        return;
    }

    bool neg = (ma < 0);
    int32_t v = ma;
    if (v < 0) v = -v;

    int32_t whole = v / 1000;
    int32_t rem   = v % 1000;
    int32_t frac2 = (rem + 5) / 10;

    if (frac2 == 100) {
        frac2 = 0;
        whole += 1;
    }

    int whole_i = (int)whole;
    int frac2_i = (int)frac2;

    if (neg) {
        Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "-%d.%02d",
                          whole_i, frac2_i);
    } else {
        Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "%d.%02d",
                          whole_i, frac2_i);
    }

    Val_Meas_Curr.resizeToCurrentText();
    Val_Meas_Curr.invalidate();
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











void MainView::requestRedraw()
{
    invalidate();  // ← これは MainViewBase(Screen) 内の protected を呼べる
}


void MainView::triggerRefreshFromPresenter()
{
    if (presenter) {
        updateSyncStateUI(presenter->isCurrentIdSynced());
    }

    touchgfx::Rect fullRect(0, 0, 480, 320);
    touchgfx::Application::getInstance()->invalidateArea(fullRect);
}

void MainView::updateSyncStateUI(bool synced)
{
    SyncStatusText.setVisible(!synced);
    SyncStatusText.invalidate();
}
