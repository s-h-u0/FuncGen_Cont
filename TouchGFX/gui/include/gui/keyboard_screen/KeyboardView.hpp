#pragma once
class KeyboardPresenter;   // 前方宣言

#include <gui_generated/keyboard_screen/KeyboardViewBase.hpp>

class KeyboardView : public KeyboardViewBase
{
public:
	KeyboardView();
    void setupScreen()    override;
    void tearDownScreen() override;

    /* キーボード表示更新用 */
    void updateDisplay();

    /* Presenter から呼ぶ画面遷移ラッパ */
    void gotoMainScreen();

    /* 個別ボタンコールバック（Base に virtual 定義あり）*/
    void One_()    override;
    void Two_()    override;
    void Three_()  override;
    void Four_()   override;
    void Five_()   override;
    void Six_()    override;
    void Seven_()  override;
    void Eight_()  override;
    void Nine_()   override;
    void Zero_()   override;
    void Delete_() override;
    void Enter_()  override;
};
