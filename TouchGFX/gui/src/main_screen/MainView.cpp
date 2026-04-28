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

namespace {
constexpr uint32_t kTapLockMs = 120U;
static uint32_t s_lockUntilTick = 0U;

inline bool isTapLocked()
{
    return (int32_t)(HAL_GetTick() - s_lockUntilTick) < 0;
}

inline void lockTapFor(uint32_t ms = kTapLockMs)
{
    s_lockUntilTick = HAL_GetTick() + ms;
}

template <typename TextAreaT>
void refreshText(TextAreaT& text)
{
    text.resizeToCurrentText();
    text.invalidate();
}

struct FixedPoint2 {
    bool negative;
    int32_t whole;
    int32_t frac2;
};

FixedPoint2 toFixedPoint2(int32_t milli)
{
    FixedPoint2 out = {};

    out.negative = (milli < 0);
    int32_t absMilli = out.negative ? -milli : milli;

    out.whole = absMilli / 1000;
    out.frac2 = ((absMilli % 1000) + 5) / 10;

    if (out.frac2 == 100) {
        out.frac2 = 0;
        out.whole += 1;
    }

    return out;
}

template <typename TextAreaT>
void setFixedPoint2Text(TextAreaT& text,
                        Unicode::UnicodeChar* buffer,
                        uint16_t bufferSize,
                        int32_t milli,
                        int32_t invalidValue)
{
    if (milli == invalidValue) {
        Unicode::snprintf(buffer, bufferSize, "--");
        refreshText(text);
        return;
    }

    const FixedPoint2 fp = toFixedPoint2(milli);

    if (fp.negative) {
        Unicode::snprintf(buffer, bufferSize, "-%d.%02d",
                          (int)fp.whole, (int)fp.frac2);
    } else {
        Unicode::snprintf(buffer, bufferSize, "%d.%02d",
                          (int)fp.whole, (int)fp.frac2);
    }

    refreshText(text);
}
}


/* UI内部ヘルパ：Run/Stopの見た目 */
void MainView::updateRunStopUI(bool running)
{
    isRunning = running;

    toggleButton_Run .forceState(running);
    toggleButton_Stop.forceState(!running);
    toggleButton_Run .setTouchable(!running);
    toggleButton_Stop.setTouchable(running);

    toggleButton_Run.invalidate();
    toggleButton_Stop.invalidate();

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

    isRunning = false;
    updateRunStopUI(false);

    MeasTimer_Start();

    if (presenter) {
        updateBothValues(
            presenter->getDesiredValue(SettingType::Voltage),
            presenter->getDesiredValue(SettingType::TripCurrent),
            presenter->getDesiredValue(SettingType::Phase),
            AppRemote_GetID()
        );

        const uint8_t id = AppRemote_GetID();
        updateSyncButtonUI(presenter->isSyncNeeded(id));
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

void MainView::updateBothValues(uint32_t vVolt, uint32_t vCurr, uint32_t vPhas, uint32_t vID)
{
    Unicode::snprintf(Val_Set_VoltBuffer, VAL_SET_VOLT_SIZE, "%u", (unsigned)vVolt);
    refreshText(Val_Set_Volt);

    Unicode::snprintf(Val_Set_CurrBuffer, VAL_SET_CURR_SIZE, "%u", (unsigned)vCurr);
    refreshText(Val_Set_Curr);

    Unicode::snprintf(Val_Set_PhasBuffer, VAL_SET_PHAS_SIZE, "%u", (unsigned)vPhas);
    refreshText(Val_Set_Phas);

    setDipHex(static_cast<uint8_t>(vID & 0x0F));
}

/* Run/Stop ボタン */
void MainView::Run()
{
    if (isTapLocked() || isRunning) { updateRunStopUI(isRunning); return; }
    lockTapFor();

    if (presenter) {
        presenter->runCurrent();
    }
}

void MainView::Stop()
{
    if (isTapLocked() || !isRunning) { updateRunStopUI(isRunning); return; }
    lockTapFor();

    if (presenter) {
        presenter->stopCurrent();
    }
}

/* 設定ボタン */
void MainView::button_VoltClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Voltage);
        //application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::button_PhasClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Phase);
        //application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::button_CurrClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::TripCurrent);
        //application().gotoKeyboardScreenNoTransition();
    }
}

void MainView::button_IDClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::ID);
        application().gotoKeyboardScreenNoTransition();
    }
}



/* 測定表示 */
void MainView::setMeasuredCurr(int16_t val)
{
    Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "%d", static_cast<int>(val));
    refreshText(Val_Meas_Curr);
}

void MainView::setMeasuredVolt(int16_t val)
{
    Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "%d", static_cast<int>(val));
    refreshText(Val_Meas_Volt);
}

void MainView::setMeasuredVolt_mV(int16_t mv)
{
    setFixedPoint2Text(Val_Meas_Volt,
                       Val_Meas_VoltBuffer,
                       VAL_MEAS_VOLT_SIZE,
                       static_cast<int32_t>(mv),
                       static_cast<int32_t>(INT16_MIN));
}

void MainView::setMeasuredVolt_Phys_mV(int32_t mv)
{
    setFixedPoint2Text(Val_Meas_Volt,
                       Val_Meas_VoltBuffer,
                       VAL_MEAS_VOLT_SIZE,
                       mv,
                       INT32_MIN);
}

void MainView::setMeasuredCurr_mA(int32_t ma)
{
    setFixedPoint2Text(Val_Meas_Curr,
                       Val_Meas_CurrBuffer,
                       VAL_MEAS_CURR_SIZE,
                       ma,
                       INT32_MIN);
}

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


void MainView::requestRedraw()
{
    invalidate();
}



/** @brief Presenter からの確定反映後に、同期状態などの補助UIを更新して再描画する
 *  @details
 *   - 設定値そのものの反映は MainPresenter 側の updateBothValuesFromModel() が担当する
 *   - この関数は synced 表示など、補助的な画面状態の再評価と再描画を担当する
 */
void MainView::triggerRefreshFromPresenter()
{
    touchgfx::Rect fullRect(0, 0, 480, 320);
    touchgfx::Application::getInstance()->invalidateArea(fullRect);
}



void MainView::updateSyncButtonUI(bool needed)
{
    syncNeeded = needed;
    toggleButton_Sync.forceState(syncNeeded);
    toggleButton_Sync.invalidate();
}

void MainView::button_SyncClicked()
{
    if (!presenter) {
        return;
    }

    // ToggleButton の自動反転を、Model由来の正しい状態で打ち消す
    const uint8_t id = AppRemote_GetID();
    updateSyncButtonUI(presenter->isSyncNeeded(id));

    if (isTapLocked()) {
        return;
    }
    lockTapFor(200U);

    presenter->syncStart();
}
