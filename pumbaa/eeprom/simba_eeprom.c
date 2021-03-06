#include "simba.h"
struct eeprom_soft_driver_t i2c;



/**
 * Write a bit to I2C bus.
 *
 * SDA must be 0 (conf. output) entering and returning from this
 * function.
 */
int eeprom_init(struct eeprom_soft_driver_t *self_p,
                  struct pin_device_t *scl_dev_p,
                  struct pin_device_t *sda_dev_p,
                  long baudrate,
                  long max_clock_stretching_us,
                  long clock_stretching_sleep_us)
{
    self_p->scl_p = scl_dev_p;
    self_p->sda_p = sda_dev_p;
    self_p->baudrate = baudrate;
    self_p->baudrate_us = (1000000L / 2L / baudrate);
    self_p->max_clock_stretching_us = max_clock_stretching_us;
    self_p->clock_stretching_sleep_us = clock_stretching_sleep_us;

    pin_device_set_mode(self_p->scl_p, PIN_INPUT);
    pin_device_set_mode(self_p->sda_p, PIN_INPUT);

    /* The pin output values are always set to 0. The bus state is
       high by configuring the pin as an input with a pullup
       resistor. */
    pin_device_write_low(self_p->scl_p);
    pin_device_write_low(self_p->sda_p);

    return (0);
}
static int wait_for_clock_stretching_end(struct eeprom_soft_driver_t *self_p)
{
    long clock_stretching_us;

    clock_stretching_us = 0;

    do {
        if (pin_device_read(self_p->scl_p) == 1) {
            return (0);
        }

        thrd_sleep_us(self_p->clock_stretching_sleep_us);
        clock_stretching_us += self_p->clock_stretching_sleep_us;
    } while (clock_stretching_us < self_p->max_clock_stretching_us);

    return (-1);
}
static int write_bit(struct eeprom_soft_driver_t *self_p,
                     int value)
{
    if (value == 1) {
        pin_device_set_mode(self_p->sda_p, PIN_INPUT);
    } else {
        pin_device_set_mode(self_p->sda_p, PIN_OUTPUT);
    }

    /* SDA change propagation delay. */
    time_busy_wait_us(self_p->baudrate_us);

    /* Set SCL high to indicate a new valid SDA value is available */
    pin_device_set_mode(self_p->scl_p, PIN_INPUT);

    /* Wait for SDA value to be read by slave, minimum of 4us for
       standard mode. */
    time_busy_wait_us(self_p->baudrate_us);

    /* Clock stretching */
    if (wait_for_clock_stretching_end(self_p) != 0) {
        return (-1);
    }

    /* Clear the SCL to low in preparation for next change. */
    pin_device_set_mode(self_p->scl_p, PIN_OUTPUT);

    time_busy_wait_us(self_p->baudrate_us);
    return (0);
}

/**
 * Read a bit from the I2C bus.
 *
 * SDA must be 0 (conf. output) entering and returning from this
 * function.
 */
static int read_bit(struct eeprom_soft_driver_t *self_p,
                    uint8_t *value_p)
{
    /* Let the slave drive data. */
    pin_device_set_mode(self_p->sda_p, PIN_INPUT);

    /* Wait for SDA value to be written by slave, minimum of 4us for
       standard mode. */
    time_busy_wait_us(self_p->baudrate_us);

    /* Set SCL high to indicate a new valid SDA value is available. */
    pin_device_set_mode(self_p->scl_p, PIN_INPUT);

    /* Clock stretching. */
    if (wait_for_clock_stretching_end(self_p) != 0) {
        return (-1);
    }

    /* Wait for SDA value to be written by slave, minimum of 4us for
       standard mode. */
    time_busy_wait_us(self_p->baudrate_us);

    /* SCL is high, read out bit. */
    *value_p = pin_device_read(self_p->sda_p);

    /* Set SCL low in preparation for next operation. */
    pin_device_set_mode(self_p->scl_p, PIN_OUTPUT);

    time_busy_wait_us(self_p->baudrate_us);
    return (0);
}
/**
 * Send the start condition by setting SDA low while keeping SCL
 * high.
 *
 * Both SDA and SCL are set to 0 if this function returns 0.
 */
static int start_cond(struct eeprom_soft_driver_t *self_p)
{
    /* The line is busy if SDA is low. */
    pin_device_set_mode(self_p->sda_p, PIN_INPUT);
    time_busy_wait_us(self_p->baudrate_us);
    pin_device_set_mode(self_p->scl_p, PIN_INPUT);
    time_busy_wait_us(self_p->baudrate_us);
    if (pin_device_read(self_p->sda_p) == 0
        || pin_device_read(self_p->scl_p) == 0 ) {
        return (-1);
    }

    /* SCL is high, set SDA from 1 to 0. */
    pin_device_set_mode(self_p->sda_p, PIN_OUTPUT);
    time_busy_wait_us(self_p->baudrate_us);

    /* Set SCL low as preparation for the first transfer. */
    pin_device_set_mode(self_p->scl_p, PIN_OUTPUT);
    time_busy_wait_us(self_p->baudrate_us);
    return (0);
}

/**
 * Send the stop condition by pulling SDA high while SCL is high.
 *
 * Both SDA and SCL are floating (set to 1 by pullup) if this function
 * returns 0.
 */
static int stop_cond(struct eeprom_soft_driver_t *self_p)
{
    /* Set SDA to 0. */
    pin_device_set_mode(self_p->sda_p, PIN_OUTPUT);
    time_busy_wait_us(self_p->baudrate_us);

    /* SCL to 1. */
    pin_device_set_mode(self_p->scl_p, PIN_INPUT);

    /*
    if (wait_for_clock_stretching_end(self_p) != 0) {
        return (-1);
    }*/

    /* Stop bit setup time, minimum 4us. */
    time_busy_wait_us(self_p->baudrate_us);

    /* SCL is high, set SDA from 0 to 1. */
    pin_device_set_mode(self_p->sda_p, PIN_INPUT);
    time_busy_wait_us(self_p->baudrate_us);

    /* Make sure no device is pulling SDA low. */
    if (pin_device_read(self_p->sda_p) == 0) {
        return (-1);
    }

    time_busy_wait_us(self_p->baudrate_us);

    return (0);
}

static int write_byte(struct eeprom_soft_driver_t *self_p,
                      uint8_t byte, int ACK)
{
    int i;
    uint8_t nack = 0;

    for (i = 0; i < 8; i++) {
        if (write_bit(self_p, (byte & 0x80) != 0) != 0) {
            return (-1);
        }

        byte <<= 1;
    }
    if(ACK)
    {
        if (read_bit(self_p, &nack) != 0) {
            return (-1);
        }
    }
    return (nack);

}

/**
 * Read a byte from I2C bus.
 */
static int read_byte(struct eeprom_soft_driver_t *self_p,
                     uint8_t *byte_p, uint8_t ACK)
{
    uint8_t bit;
    int i;

    bit = 0;
    *byte_p = 0;
    pin_device_set_mode(self_p->sda_p, PIN_INPUT);
    time_busy_wait_us(self_p->baudrate_us);
    for (i = 0; i < 8; i++) {
        if (read_bit(self_p, &bit) != 0) {
            return (-1);
        }

        *byte_p = ((*byte_p << 1 ) | bit);
    }

    if(ACK)
    {
        /* Acknowledge that the byte was successfully received. */
        if (write_bit(self_p, 0) != 0) {
            return (-1);
        }
    }
    else 
    {
        if (write_bit(self_p, 1) != 0) {
            return (-1);
        }       
    }
    return (0);
}


#define EEPROM_PAGE_DIM 32
#define EEPROM_BYTE_READ   0b10100001
#define EEPROM_BYTE_WRITE  0b10100000


#define CHK(x) do { \
  int retval = (x); \
  if (retval != 0) { \
    fprintf(stderr, "Eeprom error: %s returned %d at %s:%d", #x, retval, __FILE__, __LINE__); \
    stop_cond(&i2c); \
    return -1 ; \
  } \
} while (0)
int eeprom_write_buf(struct eeprom_soft_driver_t *self_p,int device, int addr, char *buf, int size)
{
    int i; 

    for(i = addr; i < addr + size ; i ++ ) 
    {
        if( i == addr || i % EEPROM_PAGE_DIM == 0 )
        {
            //new page 
            if( i > addr ) 
            {
                //stop for previous stream 
                CHK(stop_cond(self_p));
            }
            //poll the write eeprom write cycle 
            while(1)
            {
                if(start_cond(self_p) == 0 
                   && write_byte(self_p, EEPROM_BYTE_WRITE | (device << 1),1 ) == 0)
                    break ; 
            }
            CHK(write_byte(self_p, (uint8_t)(i >> 8) & 255,1));
            CHK(write_byte(self_p, (uint8_t)i & 255,1));
        }
        //write in the next byte of the current page 
        CHK(write_byte(self_p, buf[i - addr], 1));
        
    }
    CHK(stop_cond(self_p));
    return 0; 
    
}
int eeprom_read_buf(struct eeprom_soft_driver_t *self_p, int device, int addr, char *buf, int size)
{
    int i;
    for(i = addr ; i < addr + size ; i ++ ) 
    {
        if( i == addr || i % EEPROM_PAGE_DIM == 0 )
        {
            if(i > addr ) 
            {
                
            }
            if( i == addr )
            {
                //poll the eeprom write cycle 
                while(1) 
                {
                    if(start_cond(self_p) == 0 
                        && write_byte(self_p, EEPROM_BYTE_WRITE | (device << 1), 1) == 0)
                        break; 
                }
                CHK(write_byte(self_p, (uint8_t)(i >> 8) & 255, 1) );
                CHK(write_byte(self_p, (uint8_t)i & 255,1));
                CHK(start_cond(self_p));
                CHK(write_byte(self_p, EEPROM_BYTE_READ | (device << 1),1));
            }
        }
        if( i == addr + size - 1 ) 
        {
            //read, no ack 
            CHK(read_byte(self_p, (uint8_t *) &buf[i - addr], 0));
        }
        else 
        {
            //sequential read, ack 
            CHK(read_byte(self_p, (uint8_t *) &buf[i - addr], 1));
        }
    }
    CHK(stop_cond(self_p));
    return 0;
}
int eeprom_module_init()
{
    return 0;
    
}