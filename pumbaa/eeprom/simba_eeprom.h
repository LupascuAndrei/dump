#include "simba.h"
struct eeprom_soft_driver_t {
    struct pin_device_t *scl_p;
    struct pin_device_t *sda_p;
    long baudrate;
    long baudrate_us;
    long max_clock_stretching_us;
    long clock_stretching_sleep_us;
};
int eeprom_write_buf(struct eeprom_soft_driver_t *self_p,int device, int addr, char *buf, int size);
int eeprom_read_buf (struct eeprom_soft_driver_t *self_p,int device, int addr, char *buf, int size);
int eeprom_init(struct eeprom_soft_driver_t *self_p,
                  struct pin_device_t *scl_dev_p,
                  struct pin_device_t *sda_dev_p,
                  long baudrate,
                  long max_clock_stretching_us,
                  long clock_stretching_sleep_us);
int eeprom_module_init();