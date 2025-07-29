#pragma once
class MainPresenter;

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <touchgfx/Unicode.hpp>
#include <cstdint>                      // ★ 忘れずに
#include <gui/common/SettingType.hpp>   // いるなら

class MainView : public MainViewBase
{
public:
    MainView();
    void setupScreen()    override;
    void tearDownScreen() override;
    void Run()            override;
    void Stop()            override;

    // ★両方書き込む関数
    void updateBothValues(uint32_t volt, uint32_t phas);

    // 使わない二重宣言は削除してOK
    // void updateValue(uint32_t v, SettingType type);  ← もう不要なら消す

    void button_VoltClicked() override;
    void button_PhasClicked() override;

    /** 測定電圧を設定（表示用） */
    void setMeasuredVolt(int16_t val);
    /** 測定電流を設定（表示用） */
    void setMeasuredCurr(int16_t val);

    virtual void handleTickEvent() override;

private:
    bool isRunning = false;  ///< Run 中かどうかを保持
};
