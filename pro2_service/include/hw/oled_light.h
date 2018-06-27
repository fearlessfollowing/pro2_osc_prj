//
// Created by vans on 17-3-1.
//

#ifndef PROJECT_OLED_LIGHT_H
#define PROJECT_OLED_LIGHT_H

#include <sys/ins_types.h>
#include <common/sp.h>

class ins_i2c;

class oled_light
{
public:
    explicit oled_light();
    ~oled_light();

//    void light_on(u8 uIndex);
//    void light_off(u8 uIndex);
//    void light_on_all();
//    void light_off_all();

    void set_light_val(u8 val);
	int factory_test(int icnt = 3);

private:
   void init();
   void deinit();
   sp<ins_i2c> mI2CLight;
}; 

#endif