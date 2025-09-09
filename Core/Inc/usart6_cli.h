#ifndef USART6_CLI_H
#define USART6_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 初期化（USART6割込み受信開始） */
void CLI_Init(void);

/* ポーリング：受信行を処理（main から周期呼び出し） */
void CLI_Poll(void);

/* 文字列送信（CR/LFは呼び出し側で付与） */
void CLI_Write(const char* s);

/* UI→CLI の RUN 状態通知（0:STOP, 1:RUN） */
void CLI_SetRunState_FromUI(int on);

/* RUN 状態の参照（0/1） */
int  CLI_GetRunState(void);

#ifdef __cplusplus
}
#endif

#endif /* USART6_CLI_H */
