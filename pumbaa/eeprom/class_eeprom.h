
#include "pumbaa.h"

#if CONFIG_PUMBAA_CLASS_EEPROM == 1

struct class_eeprom_t {
    mp_obj_base_t base;
    struct i2c_soft_driver_t drv;
};
extern const mp_obj_type_t module_drivers_class_eeprom;
#endif