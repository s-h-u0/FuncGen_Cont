#pragma once
class MainPresenter;

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <touchgfx/Unicode.hpp>
#include <cstdint>                      // ★ 忘れずに
#include <gui/common/SettingType.hpp>   // いるなら

class MainView : public MainViewBase
{
public:
	/* 関数名"MainView"
	 * コンストラクタ（最初に呼ばれる初期化の入り口）。
     * ここでは重い処理はしません。準備だけ。
	 */
    MainView();

    /* 関数名"setupScreen"
     * 画面が表示されるときに呼ばれる。
     * 画面の初期状態を整える（最初は STOP 状態にする、アニメ停止する）。
     * 測定タイマーをスタートさせる。→ 一定周期でデータを測り始める。
     */
    void setupScreen()    override;

    /* 関数名"tearDownScreen"
     * 画面から離れる直前に呼ばれる。
     * 測定タイマーを止めて、一時的な状態のクリア。後片付けをする。
　　　　* 👉「一時的な状態」とは、この画面にいるときだけ動かしているもの（測定タイマーやアニメーションなど）のこと。
     * → 次の画面に移ったら不要になるから止める。
     */
    void tearDownScreen() override;

    /* 関数名"Run"
     * 「RUNボタンが押された」時に呼ばれる。
     * やること：
　　　　* 1.見た目（UI）を先に RUN 状態に変える。
     * 2.誤連打防止のために120msロック。
     * 3.Presenterから設定値をもらって → AD9833（信号発生IC）と AD5292（電圧制御IC）に送る。
     * presenter->getDesiredValue() を使って「目標値（ユーザーが入力した数値）」を取り出している。
     */
    void Run()            override;

    /* 関数名"Stop"
     * 「STOPボタンが押された」時に呼ばれる。
     * やること：
　　　　* 1.見た目（UI）を先に RUN 状態に変える。
     * 2.誤連打防止のために120msロック。
     * 3.Presenterから設定値をもらって → AD5292 に「停止値（0x400）」を書き込んで止める
     */
    void Stop()            override;

    /* 関数名"updateBothValues"
     * 電圧と位相の設定値をまとめて画面に書き込む。
     * Unicode::snprintf() でテキストを数字に変換して、ラベルに入れる。
　　　　* invalidate() で「再描画してください」と指示 → 画面が更新され
     * invalidate() = 「この部品の見た目が変わったから、もう一度描き直して」と頼む命令
     */
    void updateBothValues(uint32_t volt, uint32_t phas);

    /* 関数名"button_XXXClicked"
     * 「電圧設定ボタン」が押されたときに呼ばれる。
     * Presenterに「今からVoltage（またはPhase）の入力をするよ」と通知。
　　　　* そしてキーボード画面へ画面遷移させる
     * invalidate() = 「この部品の見た目が変わったから、もう一度描き直して」と頼む命令
     */
    void button_VoltClicked() override;
    void button_PhasClicked() override;


    /* 関数名"setMeasuredXXX"
     * 実際に測った電圧や電流を画面に表示する関数。
     * 数字を文字列に変えてラベルへ表示。
　　　　* setMeasuredVolt_mV では小数点付き（"3.14V" みたいに整形）にして出してる
     */
    void setMeasuredVolt(int16_t val);  /** 測定電圧を設定（表示用） */
    void setMeasuredVolt_mV(int16_t mv);/** 測定電圧(mV)を設定（表示用） */
    void setMeasuredCurr(int16_t val);  /** 測定電流を設定（表示用） */


    /* 関数名"handleTickEvent"
     * 毎フレーム呼ばれる定期更新処理。
     * TouchGFXがVSync（画面の更新タイミング）ごとに呼ぶ。
　　　　* ここでは「1秒に1回測定更新する」トリガに使っている
     * VSync = 画面を描き換えるタイミング。例えば1秒間に60回更新するなら「60HzのVSync」。
     */
    virtual void handleTickEvent() override;

private:
    //< Run 中かどうかを保持
    bool isRunning = false;

    /* 関数名"updateRunStopUI"
     * UI専用の見た目切り替え。
     * RUN/STOPボタンの有効・無効や、文字色を変えたり、アニメ（スピナー）を回したり止めたりする。
     * 実際の「信号出力ON/OFF」はここではしない。
     */
    void updateRunStopUI(bool running);

};
