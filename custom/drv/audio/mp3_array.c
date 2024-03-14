/**
 * @file amr_array.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "mp3_array.h"
#include "cm_fs.h"
#include "drv_uart.h"
#include "stdio.h"
#include "string.h"

#define DBG_NAME "mp3_array"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

static int load_amr_file(const unsigned char* amr, int length, const char* path)
{
    int ret = 0;
    int32_t fd = cm_fs_open(path, CM_FS_WB);
    ret = cm_fs_write(fd, amr, length);
    ret = cm_fs_close(fd);
    return ret;
}

void my_amr_load_files(void)
{
    int ret = 0;
    /*将要用到的音频文件载入文件系统*/
	ret += load_amr_file(linknet_mp3, ARRAY_SIZE(linknet_mp3), MP3_LINKNET_PATH);
	ret += load_amr_file(callstart_mp3, ARRAY_SIZE(callstart_mp3), MP3_CALLSTART_PATH);
	ret += load_amr_file(openlock_mp3, ARRAY_SIZE(openlock_mp3), MP3_OPENCLOK_PATH);
	ret += load_amr_file(closelock_mp3, ARRAY_SIZE(closelock_mp3), MP3_CLOSELOCK_PATH);
    if(ret == 0)DBG_I("All mp3 write successed\r\n");
}