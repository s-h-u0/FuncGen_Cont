/**************************************************************
 * @file    MainView.cpp
 **************************************************************/

#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <dpot_AD5292.h>
#include <touchgfx/Unicode.hpp>
#include <algorithm>
#include "main.h"
#include "stm32f4xx_hal.h"
#include <dds_AD9833.h>
#include "meas_timer.h"
#include <touchgfx/Color.hpp>
#include "usart6_cli.h"

// extern "C" ブロックは行を分ける（プリプロセッサ用）
extern "C" {
#include <stdint.h>
}

namespace {
    static MainView* s_activeView = nullptr;

    struct PendingCLI {
        volatile uint8_t has;      // 0/1
        SettingType t;
        uint32_t v;
    };
    static PendingCLI s_cli = {0, SettingType::Voltage, 0};
}

namespace {
    static struct {
        volatile uint8_t has;
        uint8_t running; // 0/1
    } s_cli_run = {0, 0};
}

/* UI内部ヘルパ：Run/Stopの見た目 */
void MainView::updateRunStopUI(bool running)
{
    isRunning = running;

    toggleButton_Run .forceState(running);
    toggleButton_Stop.forceState(!running);
    toggleButton_Run .setTouchable(!running);
    toggleButton_Stop.setTouchable(running);

    // ★ トグルも明示的に再描画（forceStateは無効矩形を出さない環境がある）
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

    // 数値テキストのワイルドカードを明示バインド（生成コードが済ませていても念のため）
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

    s_activeView = this;

    // 初期表示も明示反映
    updateBothValues(
        presenter->getDesiredValue(SettingType::Voltage),
        presenter->getDesiredValue(SettingType::Phase)
    );
}

/* 画面終了 */
void MainView::tearDownScreen()
{
    MeasTimer_Stop();
    if (s_activeView == this) s_activeView = nullptr;
    MainViewBase::tearDownScreen();
}

/* 設定値表示（Voltage/Phase） */
void MainView::updateBothValues(uint32_t vVolt, uint32_t vPhas)
{
    // Volt
    Unicode::snprintf(Val_Set_VoltBuffer, VAL_SET_VOLT_SIZE, "%u",
                      static_cast<unsigned>(vVolt));
    Val_Set_Volt.resizeToCurrentText();   // 可変幅なら毎回リサイズ
    Val_Set_Volt.invalidate();

    // Phase
    Unicode::snprintf(Val_Set_PhasBuffer, VAL_SET_PHAS_SIZE, "%u",
                      static_cast<unsigned>(vPhas));
    Val_Set_Phas.resizeToCurrentText();
    Val_Set_Phas.invalidate();
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

    uint32_t vPhase = presenter->getDesiredValue(SettingType::Phase);
    AD9833_Set(50, AD9833_SINE, vPhase);     // 必要に応じて presenter値に置換
    uint32_t vVolt  = presenter->getDesiredValue(SettingType::Voltage);
    AD5292_SetVoltage(vVolt);

    CLI_SetRunState_FromUI(1);    // CLI側のRUN?も1へ
}

void MainView::Stop()
{
    if (locked() || !isRunning) { updateRunStopUI(isRunning); return; }
    updateRunStopUI(false);
    lockFor(120);

    AD5292_Set(0x400);
    CLI_SetRunState_FromUI(0);    // CLI側のRUN?も0へ
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

    // CLI → UI : Run/Stop 反映
    if (s_activeView == this && s_cli_run.has) {
        s_cli_run.has = 0;
        notifyRunStopFromCLI(s_cli_run.running != 0);
    }

    // 既存の DesiredValue ペンディング処理
    if (s_activeView == this && s_cli.has) {
        s_cli.has = 0;
        presenter->setDesiredValue(s_cli.t, s_cli.v);
        updateBothValues(
            presenter->getDesiredValue(SettingType::Voltage),
            presenter->getDesiredValue(SettingType::Phase)
        );
    }

    if (MeasTimer_Consume()) {
        presenter->updateMeasuredValues();
        presenter->updateDipValue();
    }
    MainViewBase::handleTickEvent();
}

// --- C 側から呼ばれる関数を実装 ---
extern "C" void ui_notify_runstop(int running)
{
    s_cli_run.running = (running != 0);
    s_cli_run.has = 1;   // UI スレッドで処理させる
}

/* DIP表示 */
void MainView::setDipHex(uint8_t nibble)
{
    textArea2.invalidate();
    const uint8_t v = nibble & 0x0F;
    textArea2Buffer[0] = (touchgfx::Unicode::UnicodeChar)((v < 10) ? ('0' + v) : ('A' + (v - 10)));
    textArea2Buffer[1] = 0;
    textArea2.resizeToCurrentText();
    textArea2.invalidate();
}

/* === CLI→UI 公開ラッパ === */
void MainView::notifyRunStopFromCLI(bool running)
{
    updateRunStopUI(running);
}

void MainView::setDesiredValueFromCLI(SettingType t, uint32_t v)
{
    presenter->setDesiredValue(t, v);
    updateBothValues(
        presenter->getDesiredValue(SettingType::Voltage),
        presenter->getDesiredValue(SettingType::Phase)
    );
}


/* === C側から呼び出されるブリッジ === */
// --- 既存の extern "C" ブリッジを「積むだけ」に変更 ---
extern "C" void ui_set_desired_value(int which, uint32_t v)
{
    s_cli.t   = (which == 0) ? SettingType::Voltage : SettingType::Phase;
    s_cli.v   = v;
    s_cli.has = 1;  // ← ここでは UI を触らない！
}


