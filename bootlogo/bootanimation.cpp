/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: bootanimation.cpp
** 功能描述: 启动动画服务
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年7月17日
** 修改记录:
** V1.0			Skymixos		2018-07-17		创建文件，添加注释
******************************************************************************************************/

#include <unistd.h>
#include <common/include_common.h>
#include <common/sp.h>
#include <common/check.h>
#include <update/dbg_util.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <hw/oled_module.h>


#ifdef BOOTA_USE_LIGHT
#include <hw/oled_light.h>
#endif

#include <log/stlog.h>


class BootAnimation {
public:
	BootAnimation();
	~BootAnimation();

	void startBootAnimation();

private:

#ifdef BOOTA_USE_LIGHT
	sp<oled_light> mOLEDLight;
#endif

	sp<oled_module> mOLEDModule;

};


BootAnimation::BootAnimation()
{

#ifdef BOOTA_USE_LIGHT
	mOLEDLight = (sp<oled_light>) (new oled_light());
    CHECK_NE(mOLEDLight, nullptr);
#endif
	
	mOLEDModule = (sp<oled_module>) (new oled_module());
    CHECK_NE(mOLEDModule, nullptr);

}


BootAnimation::~BootAnimation()
{
#ifdef BOOTA_USE_LIGHT
	mOLEDLight = nullptr;
#endif

	mOLEDModule = nullptr;
}




void BootAnimation::startBootAnimation()
{
	const u8* pret = (const u8 *)"Insta360";
	mOLEDModule->ssd1306_disp_16_str(pret, 8, 16);
}


int main(int argc, char* argv[])
{

	sp<BootAnimation> gBootAnimation = (sp<BootAnimation>)(new BootAnimation());

	gBootAnimation->startBootAnimation();

	sleep (2);
	
	return 0;
}

