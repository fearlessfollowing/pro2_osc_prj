/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: main.cpp
** 功能描述: UI核心进程的入口
**
**
**
** 作     者: Wans
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016-12-01		创建文件
** V2.0			Skymixos		2018-06-05		添加注释
******************************************************************************************************/

#include <util/msg_util.h>
#include <sys/sig_util.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <update/update_oled.h>
#include <util/icon_ascii.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>

void start_all();
void init_fifo();
void debug_version_info();

#define TAG "pro_service"

const char *log_name = "/home/nvidia/insta360/log/p_log";

int main(int argc ,char *argv[])
{
    int iRet = 0;
    debug_version_info();
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    arlog_configure(true, true, log_name, false);

    iRet = __system_properties_init();	/* 属性区域初始化 */
    if (iRet)
    {
    	Log.e(TAG, "pro_service service exit: __system_properties_init() faile, ret = %d\n", iRet);
        return -1;
    }

    Log.d(TAG, "ro.version [%s]", property_get("ro.version"));

    init_fifo();
    start_all();

    while (1)
    {
        msg_util::sleep_ms(5 * 1000);
    }

    Log.d(TAG, "main pro over\n");
}
