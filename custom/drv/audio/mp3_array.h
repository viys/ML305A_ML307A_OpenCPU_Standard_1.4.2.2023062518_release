/**
 * @file amr_array.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

/* 连接网络语音 */
#define MP3_LINKNET_PATH "linknet.mp3"
extern const unsigned char linknet_mp3[2592];
/* 呼叫语音 */
#define MP3_CALLSTART_PATH "callstart.mp3"
extern const unsigned char callstart_mp3[2088];
/* 开门语音 */
#define MP3_OPENLOCK_PATH "openlock.mp3"
extern const unsigned char openlock_mp3[1224];
/* 锁门语音 */
#define MP3_CLOSELOCK_PATH "closelock.mp3"
extern const unsigned char closelock_mp3[1224];
/* 指纹解锁失败录音 */
#define MP3_VFY_ERR_PATH "vfy_err.mp3"
extern const unsigned char vfy_err_mp3[1872];

void my_amr_load_files(void);
