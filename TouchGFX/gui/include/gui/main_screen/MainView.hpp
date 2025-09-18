/**************************************************************
 * MainView.hpp  (FULL)
 **************************************************************/
#pragma once

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/common/SettingType.hpp>

class MainPresenter;

class MainView : public MainViewBase
{
public:
    MainView();

    // 画面ライフサイクル
    virtual void setupScreen();
    virtual void tearDownScreen();

    // 自動生成のボタンハンドラ（MainViewBase が呼んでくる）
    virtual void Run();
    virtual void Stop();
    virtual void button_VoltClicked();
    virtual void button_PhasClicked();

    // 測定表示（Presenter から）
    void setMeasuredCurr(int16_t val);
    void setMeasuredVolt(int16_t val);
    void setMeasuredVolt_mV(int16_t mv);

    // DIP 表示（Presenter から）
    void setDipHex(uint8_t nibble);

    // 設定値（表示）を一括更新（Presenter/CLI 共用）
    // 既存: 2引数
    // void updateBothValues(uint32_t vVolt, uint32_t vPhas);

    // 修正版: 3引数（IDも一緒に反映）
    void updateBothValues(uint32_t vVolt, uint32_t vPhas, uint32_t vID);


    // CLI→UI ブリッジ（C から呼ぶ）
    void notifyRunStopFromCLI(bool running);
    void setDesiredValueFromCLI(SettingType t, uint32_t v);

    // tick（計測トリガ）
    virtual void handleTickEvent();

    // 見た目更新API（CLIブリッジからも使うので public）
    void updateRunStopUI(bool running);

    // ★追加: Designerのbutton_IDと対応するコールバック
    virtual void button_IDClicked();

protected:
    bool isRunning {false};
};
