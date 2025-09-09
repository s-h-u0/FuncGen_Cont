/**************************************************************
 * @file    MainView.cpp
 * @brief   Main 画面の表示・操作ロジック（TouchGFX View 層）
 * @details
 *  - 役割: UIコンポーネントの状態管理、ボタン操作 → デバイス操作の起点
 *  - 非目標: デバイスの詳細制御（Presenter/Model 側で実施）
 *  - 重要不変条件 (invariants):
 *      * Run/Stop は常に相互排他（どちらか一方のみ有効）
 *      * UI は “先に反映” → その後デバイス制御（体感の応答性を最優先）
 *      * 誤連打対策として 120ms の軽いロック（wrap-around 安全）
 *  - レイアウト方針:
 *      * 数値と単位は分離。数値は右寄せ固定幅、単位は固定ラベル
 *      * エラー時（通信不良など）は "--" を表示（単位の表示は任意）
 *  - 生成物の編集禁止:
 *      * *ViewBase.* は TouchGFX Designer の生成物。ロジックは本ファイル側で追加
 **************************************************************/


#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <dpot_AD5292.h>
#include <touchgfx/Unicode.hpp>
#include <algorithm>
#include "main.h"            // HAL_GetTick(), GPIO など
#include "stm32f4xx_hal.h"
#include <dds_AD9833.h>
#include "meas_timer.h"
#include <touchgfx/Color.hpp>
#include "usart6_cli.h"



/**
 * @brief Run/Stopボタン群の見た目・タッチ可否とスピナー表示を一括更新する
 * @param[in] running  true: RUN中／false: 停止中
 *
 * @details 本関数は「UIのみ」を更新し、デバイス制御は行わない。
 *          - トグルの押下状態とタッチ可否を排他的に設定
 *          - RUN/STOP ラベルの色を更新
 *          - スピナー（animatedImage1）の可視/不可視と再生状態を更新
 *
 * @note  スピナーは Designer の Interaction では操作しない。本関数が唯一の制御点。
 *        停止時は `stopAnimation(); invalidate(); setVisible(false);` の順に行い、
 *        背景を再描画してから非表示化して残像を防ぐ。
 *        再開時に毎回先頭フレームから回したい場合は
 *        `startAnimation(false, true , true );//(rev,reset,loop)` // を用いる。
 */

void MainView::updateRunStopUI(bool running)
{
    isRunning = running;

    toggleButton_Run .forceState(running);
    toggleButton_Stop.forceState(!running);
    toggleButton_Run .setTouchable(!running);
    toggleButton_Stop.setTouchable(running);

    // ラベル色
    if (running) {
        RUN_Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
        STOP_Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    } else {
        STOP_Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
        RUN_Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    }
    RUN_Text.invalidate();
    STOP_Text.invalidate();

    // ★ スピナー制御
    if (running) {
        animatedImage1.setVisible(true);
        animatedImage1.startAnimation(false /*rev*/, true /*reset*/, true /*loop*/);
        animatedImage1.invalidate();
    } else {
        animatedImage1.stopAnimation();
        animatedImage1.invalidate();      // 背景を塗り直し
        animatedImage1.setVisible(false); // その後に非表示
    }
}


/** @brief コンストラクタ（重い処理はしない）
 *  @note  画面要素の初期化は setupScreen() で行う。
 */
MainView::MainView()
    : MainViewBase()
{}


/** @brief 画面初期化。UI初期状態を Stop 側に揃え、計測タイマを開始
 *  @details
 *   - ADCハンドルを Presenter に渡す（計測は Presenter→View の流れ）
 *   - 計測周期は MeasTimer_* に委譲
 */
void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // 必ず停止＆非表示から始める
    animatedImage1.stopAnimation();
    animatedImage1.setVisible(false);
    animatedImage1.invalidate();

    isRunning = false;
    updateRunStopUI(false);                   // ← 引数だけに

    extern MCP3428_HandleTypeDef hadc3428;
    presenter->setADCHandle(&hadc3428);

    MeasTimer_Start();
}


/** @brief 画面終了処理。計測タイマ停止（必要なら UI を停止側へ戻す） */
void MainView::tearDownScreen()
{
    MeasTimer_Stop();
    MainViewBase::tearDownScreen();
}

/** @brief 設定値（電圧・位相）の表示バッファを更新して再描画
 *  @param vVolt 表示用の電圧（単位は画面仕様に依存）
 *  @param vPhas 表示用の位相（同上）
 *  @note  フォーマットはここで固定。UIの右寄せ・固定幅ポリシと整合させる。
 */
void MainView::updateBothValues(uint32_t vVolt, uint32_t vPhas)
{
    // ★ Volt 側
    Unicode::snprintf(
        Val_Set_VoltBuffer,
        VAL_SET_VOLT_SIZE,
        "%u",
        static_cast<unsigned>(vVolt)
    );
    Val_Set_Volt.invalidate();

    // ★ Phase 側
    Unicode::snprintf(
        Val_Set_PhasBuffer,
        VAL_SET_PHAS_SIZE,
        "%u",
        static_cast<unsigned>(vPhas)
    );
    Val_Set_Phas.invalidate();
}


// ---- タップ誤操作抑制ロック -----------------
/** @brief ロック解除予定Tick（locked()で差分判定） */
static uint32_t s_lockUntilTick = 0;

/** @brief 標準ロック時間[ms] */
constexpr uint32_t kLockMs = 120;

/** @brief 現在ロック中か（HAL_GetTick の周回に安全な差分判定） */
inline bool locked() {
    return (int32_t)(HAL_GetTick() - s_lockUntilTick) < 0;
}
/** @brief 指定msだけロックを開始（デフォルト kLockMs） */
inline void lockFor(uint32_t ms = kLockMs) {
    s_lockUntilTick = HAL_GetTick() + ms;
}
// -----------------------------------------------------------------




void MainView::Run()
{
    if (locked() || isRunning) { updateRunStopUI(isRunning); return; }
    updateRunStopUI(true);
    lockFor(120);
    uint32_t vPhase = presenter->getDesiredValue(SettingType::Phase);
    AD9833_Set(1000, AD9833_TRIANGLE, vPhase);
    uint32_t vVolt  = presenter->getDesiredValue(SettingType::Voltage);
    AD5292_SetVoltage(vVolt);

    // ★CLIへ通知（RUN?が1になる）
    CLI_SetRunState_FromUI(1);
}

void MainView::Stop()
{
    if (locked() || !isRunning) { updateRunStopUI(isRunning); return; }
    updateRunStopUI(false);
    lockFor(120);
    AD5292_Set(0x400);

    // ★CLIへ通知（RUN?が0になる）
    CLI_SetRunState_FromUI(0);
}
/** @brief [UI] 電圧設定ボタンのハンドラ
 *  @details 現在の設定対象を Voltage に切り替え、キーボード画面へ遷移する。
 *  @note    デバイスI/Oは行わず、Presenterへの通知＋画面遷移のみ。
 */
void MainView::button_VoltClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Voltage);
        application().gotoKeyboardScreenNoTransition();
    }
}


/** @brief [UI] 位相設定ボタンのハンドラ
 *  @details 現在の設定対象を Phase に切り替え、キーボード画面へ遷移する。
 *  @note    デバイスI/Oは行わず、Presenterへの通知＋画面遷移のみ。
 */
void MainView::button_PhasClicked()
{
    if (presenter) {
        presenter->setCurrentSetting(SettingType::Phase);
        application().gotoKeyboardScreenNoTransition();
    }
}


/** @brief 測定電流値をそのまま表示バッファへ反映し再描画
 *  @param val 表示する値（単位/フォーマットは画面仕様に依存）
 *  @note  右寄せ固定幅レイアウトにする場合は resizeToCurrentText() を使わない。
 */
void MainView::setMeasuredCurr(int16_t val)
{
    Unicode::snprintf(Val_Meas_CurrBuffer, VAL_MEAS_CURR_SIZE, "%d", static_cast<int>(val));
    Val_Meas_Curr.resizeToCurrentText();
    Val_Meas_Curr.invalidate();
}


/** @brief 測定電圧値（整数表現）を表示バッファへ反映し再描画
 *  @param val 表示する値（単位/フォーマットは画面仕様に依存）
 *  @note  右寄せ固定幅レイアウトにする場合は resizeToCurrentText() を使わない。
 */
void MainView::setMeasuredVolt(int16_t val)
{
    Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "%d", static_cast<int>(val));
    Val_Meas_Volt.resizeToCurrentText();
    Val_Meas_Volt.invalidate();
}


/** @brief 電圧の測定値（mV単位）を小数2桁の V 表示へ整形して反映
 *  @param mv 単位 mV。INT16_MIN の場合は通信エラー扱いで "--" 表示
 *  @details
 *   - 10mV単位で四捨五入し、"[-]X.YY" に整形
 */
void MainView::setMeasuredVolt_mV(int16_t mv)
{
    // 通信エラー時は "--" 表示（必要なら任意の表示に変更OK）
    if (mv == INT16_MIN) {
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "--");
        Val_Meas_Volt.resizeToCurrentText();
        Val_Meas_Volt.invalidate();
        return;
    }

    bool neg = (mv < 0);
    int v = (int)mv;                   // 32bitに昇格
    if (v < 0) v = -v;                 // 安全に絶対値化

    int whole = v / 1000;              // V 部分
    int rem   = v % 1000;              // mV 残り 0..999

    // 小数2桁へ四捨五入（10mV=0.01V 単位に丸め）
    int frac2 = (rem + 5) / 10;        // 0..100
    if (frac2 == 100) {                // 9.995V などのキャリー
        frac2 = 0;
        whole += 1;
    }

    if (neg)
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE, "-%d.%02d", whole, frac2);
    else
        Unicode::snprintf(Val_Meas_VoltBuffer, VAL_MEAS_VOLT_SIZE,  "%d.%02d", whole, frac2);

    Val_Meas_Volt.invalidate();
}



/** @brief 1秒周期の計測トリガ（MeasTimer_*）を消費して Presenter に計測更新を依頼
 *  @note  UIの毎フレーム処理は MainViewBase::handleTickEvent() に委譲
 */
void MainView::handleTickEvent()
{
	if (MeasTimer_Consume()) {           // 1秒ごと
	   presenter->updateMeasuredValues();  // ADC更新
	   presenter->updateDipValue();       // ★ DIP更新を追加
	}
    MainViewBase::handleTickEvent();
}



void MainView::setDipHex(uint8_t nibble)
{
    // ① まず「古い領域」を無効化（これが超重要）
    textArea2.invalidate();

    const uint8_t v = nibble & 0x0F;

    // ② 1文字を直接代入（printfは使わない）
    textArea2Buffer[0] = (touchgfx::Unicode::UnicodeChar)((v < 10) ? ('0' + v) : ('A' + (v - 10)));
    textArea2Buffer[1] = 0;

    // ③ 新しい文字幅に合わせてサイズ更新
    textArea2.resizeToCurrentText();

    // ④ 新しい領域も無効化（再描画）
    textArea2.invalidate();
}

