#ifndef PROJECT_OLED_LIGHT_H
#define PROJECT_OLED_LIGHT_H

#include <sys/ins_types.h>
#include <common/sp.h>

class ins_i2c;

class oled_light {
public:
    explicit oled_light();
    ~oled_light();

    void set_light_val(u8 val);
	int factory_test(int icnt = 3);

    /* 保存恢复灯的状态 */
    void suspend_led_status();
    void resume_led_status();

private:
    void init();
    void deinit();

    u8 light_restore_val = 0;

    sp<ins_i2c> mI2CLight;
}; 

#endif
