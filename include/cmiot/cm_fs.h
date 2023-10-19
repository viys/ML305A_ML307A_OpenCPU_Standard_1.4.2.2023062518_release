/**
 * @file        cm_fs.h
 * @brief       文件系统通用API接口
 * @copyright   Copyright © 2021 China Mobile IOT. All rights reserved.
 * @author      By WangPeng
 * @date        2021/4/7
 *
 * @defgroup  fs
 * @ingroup fs
 * @{
 */

#ifndef __CM_FS_H__
#define __CM_FS_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CM_FS_RB            (0)      /*!< rb */
#define CM_FS_WB            (1)      /*!< wb */
#define CM_FS_AB            (2)      /*!< ab */
#define CM_FS_WBPLUS        (3)      /*!< wb+ */
#define CM_FS_ABPLUS        (4)      /*!< ab+ */

#define CM_FS_SEEK_SET      (0)      /*!< SEEK_SET */
#define CM_FS_SEEK_CUR      (1)      /*!< SEEK_CUR */
#define CM_FS_SEEK_END      (2)      /*!< SEEK_END */

/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef struct
{
    uint32_t free_size;  /*!< 当前可用文件系统大小 */
    uint32_t total_size; /*!< 文件系统总大小 */
} cm_fs_system_info_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************/

 /**
 *  @brief 打开一个文件
 *
 *  @param [in] filename 文件名称
 *  @param [in] flag 打开参数
 *
 *  @return
 *   >= 0 - 文件描述符 \n
 *   <  0 - 错误
 *
 *  @details 文件系统中无路径/文件夹功能，勿在filename入参中传入路径信息
 */
int32_t cm_fs_open(const char *filename, int32_t flag);

/**
 *  @brief 关闭一个文件
 *
 *  @param [in] fd 文件描述符
 *
 *  @return
 *   = 0 - 成功 \n
 *   < 0 - 失败
 */
int32_t cm_fs_close(int32_t fd);

/**
 *  @brief 读取文件内容
 *
 *  @param [in]  fd   文件描述符
 *  @param [out] buf  存储数据的缓冲区指针
 *  @param [in]  size 要读取的数据长度
 *
 *  @return
 *   >= 0 - 实际的读取长度 \n
 *   <  0 - 读取失败
 */
int32_t cm_fs_read(int32_t fd, void *buf, uint32_t size);

/**
 *  @brief 写入文件内容
 *
 *  @param [in] fd   文件描述符
 *  @param [in] buf  存储数据的缓冲区指针
 *  @param [in] size 要写入的数据长度
 *
 *  @return
 *   >= 0 - 实际的写入长度 \n
 *   <  0 - 写入失败
 */
int32_t cm_fs_write(int32_t fd, const void *buf, uint32_t size);

#if 0
/**
 *  @brief 将处于堆栈状态的文件与存储设备中的文件同步
 *
 *  @param [in] fd 文件描述符
 *
 *  @return
 *   = 0 - 同步成功 \n
 *   < 0 - 同步失败
 *
 *  @details 本平台不支持该接口
 */
int32_t cm_fs_sync(int32_t fd);
#endif

/**
 *  @brief 文件指针定位
 *
 *  @param [in] fd 文件描述符
 *  @param [in] offset 指针偏移量
 *  @param [in] base 偏移起始点，CM_FS_SEEK_SET：文件开头 CM_FS_SEEK_CUR：当前位置 CM_FS_SEEK_END：文件末尾
 *
 *  @return
 *   = 0 - 成功 \n
 *   < 0 - 失败
 *
 *  @details offset不支持为负数
 */
int32_t cm_fs_seek(int32_t fd, int32_t offset, int32_t base);

/**
 *  @brief 删除文件
 *
 *  @param [in] file_name 文件路径
 *
 *  @return
 *   >= 0 - 成功 \n
 *   <  0 - 失败
 */
int32_t cm_fs_delete(const char *file_name);

/**
 *  @brief 更改文件名
 *
 *  @param [in] src  原文件名
 *  @param [in] dest 新文件名
 *
 *  @return
 *   = 0 - 成功 \n
 *   < 0 - 失败
 */
int32_t cm_fs_move(const char *src, const char *dest);

/**
 *  @brief 检查文件是否存在
 *
 *  @param [in] file_name 文件名称
 *
 *  @return
 *   = 1 - 存在 \n
 *   = 0 - 不存在
 *
 *  @details 文件系统中无路径/文件夹功能，勿在path入参中传入路径信息
 */
int32_t cm_fs_exist(const char *file_name);

/**
 *  @brief 获取文件大小
 *
 *  @param [in] file_name 文件路径
 *  @return
 *
 *   >= 0 - 文件长度 \n
 *   <  0 - 操作失败
 *
 *  @details More details
 */
int32_t cm_fs_filesize(const char *file_name);

/**
 *  @brief 获取文件系统信息
 *
 *  @param [out] info 文件系统信息
 *
 *  @return
 *   = 0 - 成功 \n
 *   < 0 - 失败
 */
int32_t cm_fs_getinfo(cm_fs_system_info_t *info);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __CM_FS_H__ */

/** @}*/
