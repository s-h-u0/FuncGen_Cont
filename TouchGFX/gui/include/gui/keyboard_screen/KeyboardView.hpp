#pragma once
#include <gui_generated/keyboard_screen/KeyboardViewBase.hpp>

class KeyboardPresenter;

/**
 * Keyboard 画面の View。
 * ボタン押下 → Presenter → View.updateDisplay() でテキストを書き換える。
 */
class KeyboardView : public KeyboardViewBase
{
public:
    KeyboardView();
    virtual ~KeyboardView() {}

    // 画面ライフサイクル
    virtual void setupScreen()    override;
    virtual void tearDownScreen() override;

    // ボタン・コールバック
    virtual void One_()    override;
    virtual void Two_()    override;
    virtual void Three_()  override;
    virtual void Four_()   override;
    virtual void Five_()   override;
    virtual void Six_()    override;
    virtual void Seven_()  override;
    virtual void Eight_()  override;
    virtual void Nine_()   override;
    virtual void Zero_()   override;
    virtual void Delete_() override;
    virtual void Enter_()  override;

    // Presenter→View: 数値を表示に反映
    void updateDisplay();

    // Presenter バインド用
    void bindPresenter(KeyboardPresenter* p) { presenter = p; }

private:
    KeyboardPresenter* presenter {nullptr};
};
