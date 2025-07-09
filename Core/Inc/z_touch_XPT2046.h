/*
 * 	z_touch_XPT2046.h
 *	rel. TouchGFX.1.30
 *
 *  作成日: 2023年6月5日
 *      著者: mauro
 *
 *  ライセンス: https://github.com/maudeve-it/ILI9XXX-XPT2046-STM32/blob/c097f0e7d569845c1cf98e8d930f2224e427fd54/LICENSE
 *
 *  このライブラリをインストールおよび使用するには、以下の手順に従ってください:
 *  https://github.com/maudeve-it/ILI9XXX-XPT2046-STM32 を参照
 *
 *  警告:
 *	main.h のインクルードは z_displ_ILIxxxx.h のインクルードの下に配置してください
 *
 *  TouchGFX を使用する場合は、STM32TouchController.cpp 内に
 *  #include "main.h"
 *  を追加し、sampleTouch() を以下のように変更してください:
 *
 *  bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
 *  {
 *      return ((bool) Touch_TouchGFXSampleTouch(&x, &y));
 *  }
 *
 *  詳細は z_displ_ili9XXX.h も参照してください
 */
#ifndef __XPT2046_H
#define __XPT2046_H


/*||||||||||| ユーザー／プロジェクト設定 |||||||||||*/

/*****************     STEP 1      *****************
 **************** ポート設定パラメータ ************
 ** CubeMX で定義した SPI ポートに合わせて以下を設定してください **
 ***********************************************/

#define TOUCH_SPI_PORT 	hspi2
#define TOUCH_SPI 		SPI2



/*****************     STEP 2      *****************
 **********   TouchGFX キーリピート設定   ***********
 * TouchGFX 統合時のみ使用します
 * - 0 より大きい値: キーリピートが始まるまでのタイムアウト(ms)
 * - 0: キーリピート無効(単一タッチ)
 * - -1: "ドラッグ" ウィジェット用に連続タッチ
 * (詳細は上記 GitHub ページ参照)
 **************************************************/
#define DELAY_TO_KEY_REPEAT 0

/*|||||||| 設定ここまで ||||||||*/



/*|||||||||||||| デバイスパラメータ |||||||||||||||||*/
/* 以降、通常は変更不要です */

/**************************************************
 *  XPT2046 に軸ポーリングコマンドを送信する際のコマンド
 **************************************************/
#define X_AXIS		0xD0
#define Y_AXIS		0x90
#define Z_AXIS		0xB0


/**********************************************************************************
 *  XPT2046 から返ってくる値が以下の閾値を超えると、タッチなしと判定されます。
 *  注意: タッチなしでも乱数的に範囲内の値が返ることがあるので、
 *  タッチ判定には少なくとも連続２回の読み取りを行ってください
 **********************************************************************************/
#ifdef ILI9341
#define X_THRESHOLD		0x0200	//below threeshold there is no touch
#define Z_THRESHOLD		0x0200	//below threeshold there is no touch
#endif
#ifdef ILI9488
#define X_THRESHOLD		0x0500	//below threeshold there is no touch
#define Z_THRESHOLD		0x0500	//below threeshold there is no touch
#endif


/**********************************************************************************
 ***************************** キャリブレーション係数 *****************************
 **********************************************************************************
 * タッチセンサの読み取り値を画面座標に変換する線形式の係数
 * 数式:
 * Xdispl = AX * Xtouch + BX
 * Ydispl = AY * Ytouch + BY
 **********************************************************************************/

#ifdef ILI9341
#define T_ROTATION_0
#define AX 0.00801f
#define BX -11.998f
#define AY 0.01119f
#define BY -39.057f
#endif


#ifdef ILI9488_V1 //構成済　SN:6-4
#define T_ROTATION_0
#define AX -0.010560f
#define BX 331.857483f
#define AY 0.015808f
#define BY -22.500736f
#endif




#ifdef ILI9488_V2
#define T_ROTATION_0
#define AX -0.0112f
#define BX 336.0f
#define AY 0.0166f
#define BY -41.38f
#endif




/**********************************************************************************
 * 画面／タッチの向き設定:
 * T_ROTATION_xxx が有効な場合、
 * タッチの 0°／90°／180°／270° をそれぞれどの画面回転に対応させるかを定義
 **********************************************************************************/
#ifdef T_ROTATION_0
#define TOUCH0 			Displ_Orientat_0
#define TOUCH90 		Displ_Orientat_90
#define TOUCH180 		Displ_Orientat_180
#define TOUCH270 		Displ_Orientat_270
#define TOUCH_0_WIDTH 	DISPL_WIDTH
#define TOUCH_0_HEIGHT	DISPL_HEIGHT
#endif

#ifdef T_ROTATION_90
#define TOUCH0 			Displ_Orientat_90
#define TOUCH90 		Displ_Orientat_180
#define TOUCH180 		Displ_Orientat_270
#define TOUCH270 		Displ_Orientat_0
#define TOUCH_0_WIDTH 	DISPL_HEIGHT
#define TOUCH_0_HEIGHT	DISPL_WIDTH
#endif

#ifdef T_ROTATION_180
#define TOUCH0 			Displ_Orientat_180
#define TOUCH90 		Displ_Orientat_270
#define TOUCH180 		Displ_Orientat_0
#define TOUCH270 		Displ_Orientat_90
#define TOUCH_0_WIDTH 	DISPL_WIDTH
#define TOUCH_0_HEIGHT	DISPL_HEIGHT
#endif

#ifdef T_ROTATION_270
#define TOUCH0 			Displ_Orientat_270
#define TOUCH90 		Displ_Orientat_0
#define TOUCH180 		Displ_Orientat_90
#define TOUCH270 		Displ_Orientat_180
#define TOUCH_0_WIDTH 	DISPL_HEIGHT
#define TOUCH_0_HEIGHT	DISPL_WIDTH
#endif



/*|||||||||||||| インターフェースパラメータ |||||||||||||||||*/

/**********************************************************************************
 * TouchGFX 統合時に使用されるパラメータ:
 * Touch_TouchGFXSampleTouch() および Touch_GotATouch() 内での
 * ドラッグスクロールなどの挙動に影響します。
 * ディスプレイが荒れたり、動作が重たくなった場合に調整可能です。
 **********************************************************************************/

#define TOUCHGFX_TIMING    20   // 連続読み取り間隔 (ms)
#define TOUCHGFX_SENSITIVITY 1  // 同じ値と見なすピクセル² (1: 無効)
#define TOUCHGFX_MOVAVG   1    // 移動平均サンプル数 (1: 無効)
#define TOUCHGFX_REPEAT_IT 0   // 長押し後に繰り返す回数 (0: 無効)
#if DELAY_TO_KEY_REPEAT==-1
#define TOUCHGFX_REPEAT_NO 0   // REPEAT_IT 後のノータッチ繰り返し (0: 無効)
#else
#define TOUCHGFX_REPEAT_NO 0   // REPEAT_IT 後のノータッチ繰り返し (0: 無効)
#endif

/*||||||||||| ここまでインターフェース設定 ||||||||||||*/


/*|||||||||||||| 関数宣言 |||||||||||||||||*/

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

uint8_t Touch_In_XY_area(uint16_t xpos,uint16_t ypos,uint16_t width,uint16_t height);
uint8_t Touch_GotATouch(uint8_t reset);
uint8_t Touch_WaitForUntouch(uint16_t delay);
uint8_t Touch_WaitForTouch(uint16_t delay);
uint8_t Touch_PollTouch();
void Touch_GetXYtouch(uint16_t *x, uint16_t *y, uint8_t *isTouch);

#ifdef DISPLAY_USING_TOUCHGFX
uint8_t Touch_TouchGFXSampleTouch(int32_t *x, int32_t *y);
#endif /* DISPLAY_USING_TOUCHGFX */

#endif /* __XPT2046_H */

