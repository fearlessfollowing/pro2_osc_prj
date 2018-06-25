#include <hw/oled_light.h>
#include <hw/ins_i2c.h>
#include <log/stlog.h>
#include <util/msg_util.h>


using namespace std;

#define TAG "oled_light"

oled_light::oled_light()
{
    init();
}

oled_light::~oled_light()
{
    deinit();
}

void oled_light::init()
{
    mI2CLight = sp<ins_i2c>(new ins_i2c(0, 0x77, true));
    mI2CLight->i2c_write_byte(0x06, 0x00);
    mI2CLight->i2c_write_byte(0x07, 0x00);

}

#if 0
void oled_light::light_off(u8 uIndex)
{
    u8 val =~(0 << uIndex);
    set_light_val(val);
}

void oled_light::light_off_all()
{
    set_light_val(LIGHT_OFF);
}

void oled_light::light_on(u8 uIndex)
{
    u8 val = (0 << uIndex);
    set_light_val(val);
}

void oled_light::light_on_all()
{
    set_light_val(LIGHT_ALL);
}
#endif


void oled_light::set_light_val(u8 val)
{
    //keep audio on
    if (mI2CLight->i2c_write_byte(0x02, val) != 0)
    {
        Log.e(TAG, " oled write val 0x%x fail", val);
    }
}


int oled_light::factory_test(int icnt)
{
	
	/* 所有的灯:  白,红,绿,蓝  循环三次,间隔1s 
 	 * 白: 0x3f
 	 * 红: 0x09
 	 * 绿: 0x12
 	 * 蓝: 0x24
 	 * 全灭: 0x00
 	 */
 	int iRet = 0;

	Log.d(TAG, "factory_test ....");

	if (icnt < 3)
		icnt = 3;
	
	for (int i = 0; i < icnt; i++)
	{
		if (mI2CLight->i2c_write_byte(0x02, 0x3f) != 0)
		{
			Log.e(TAG," oled write val 0x%x fail", 0x3f);
			iRet = -1;
		}
			
		msg_util::sleep_ms(1000);
		
		if (mI2CLight->i2c_write_byte(0x02, 0x09) != 0)
		{
			Log.e(TAG," oled write val 0x%x fail", 0x09);
			iRet = -1;
		}
			
		msg_util::sleep_ms(1000);
		
		if (mI2CLight->i2c_write_byte(0x02, 0x12) != 0)
		{
			Log.e(TAG," oled write val 0x%x fail", 0x12);
			iRet = -1;
		}
			
		msg_util::sleep_ms(1000);
		
		if (mI2CLight->i2c_write_byte(0x02, 0x24) != 0)
		{
			Log.e(TAG," oled write val 0x%x fail", 0x24);
			iRet = -1;
		}	
		
		msg_util::sleep_ms(1000);
		mI2CLight->i2c_write_byte(0x02, 0x00);

	}

	return iRet;
	
}

void oled_light::deinit()
{

}



