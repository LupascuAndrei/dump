
#include "pumbaa.h"
#if CONFIG_PUMBAA_CLASS_EEPROM == 1

static void class_eeprom_print(const mp_print_t *print_p,
                            mp_obj_t self_in,
                            mp_print_kind_t kind)
{
    struct class_eeprom_t *self_p;

    self_p = MP_OBJ_TO_PTR(self_in);
    mp_printf(print_p, "<0x%p>", self_p);
}


//called as .write(device, addr, buffer, (optional) size ) 
static mp_obj_t class_eeprom_write(mp_uint_t n_args,
                                   const mp_obj_t *args_p)
{
    struct class_eeprom_t *self_p = MP_OBJ_TO_PTR(args_p[0]);
    mp_buffer_info_t buffer_info;
    int device, addr, size;
    mp_get_buffer_raise(MP_OBJ_TO_PTR(args_p[3]),
                        &buffer_info,
                        MP_BUFFER_READ);

    device = mp_obj_get_int(args_p[1]);
    if(device < 0 || device > 7 ) 
    {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                   "device must be between [0,7]"));
    }
    addr = mp_obj_get_int(args_p[2]);
    if (n_args == 5) {
        size = mp_obj_get_int(args_p[4]);

        if (buffer_info.len < size) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                               "bad buffer length for given buffer"));
        }
    } else {
        size = buffer_info.len;
    }
    //int eeprom_write_buf(struct eeprom_soft_driver_t *self_p, int addr, char *buf, int size, int device)
    if(eeprom_write_buf (&self_p->drv, device, addr, buffer_info.buf, size)  != 0 )
    {        
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                           "eeprom_write() failed"));      
    }
    return (mp_const_none);
}
    
//called as .read(device, addr, size)     
static mp_obj_t class_eeprom_read(mp_uint_t n_args, const mp_obj_t *args_p)
{
    int device, addr, size ; 
    device = mp_obj_get_int(args_p[1]);
    if(device < 0 || device > 7 ) 
    {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                   "device must be between [0,7]"));
    }
    addr = mp_obj_get_int(args_p[2]); 
    size = mp_obj_get_int(args_p[3]);
    if(size < 1)
    {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                   "eeprom_read: size must be > 0"));        
    }
    struct class_eeprom_t *self_p = MP_OBJ_TO_PTR(args_p[0]);
    vstr_t vstr;
    vstr_init_len(&vstr, size);
    if(eeprom_read_buf (&self_p->drv, device, addr, vstr.buf, size)  != 0 )
    {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                   "eeprom_read() failed"));     
    }
    return (mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(class_eeprom_write_obj, 4, 5, class_eeprom_write);
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(class_eeprom_read_obj, 4, 4, class_eeprom_read);
static const mp_rom_map_elem_t class_eeprom_locals_dict_table[] = {
    /* Instance methods. */
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&class_eeprom_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&class_eeprom_read_obj) }
};
static MP_DEFINE_CONST_DICT(class_eeprom_locals_dict, class_eeprom_locals_dict_table);
static mp_obj_t class_eeprom_make_new(const mp_obj_type_t *type_p,
                                   mp_uint_t n_args,
                                   mp_uint_t n_kw,
                                   const mp_obj_t *args_p)
{
    
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin_scl, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_pin_sda, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_baudrate, MP_ARG_INT, { .u_int = 8000 }}
    };  
    mp_map_t kwargs;
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    mp_map_init(&kwargs, 0);
    mp_arg_parse_all(n_args,
                     args_p,
                     &kwargs,
                     MP_ARRAY_SIZE(allowed_args),
                     allowed_args,
                     args);
    struct class_eeprom_t *self_p;
    self_p = m_new0(struct class_eeprom_t, 1);
    self_p->base.type = &module_drivers_class_eeprom;
    int pin_sda, pin_scl, baudrate; 
    pin_scl = args[0].u_int; 
    if ((pin_scl < 0) || (pin_scl >= PIN_DEVICE_MAX)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                                           "bad pin for scl"));
    }    
    
    pin_sda = args[1].u_int; 
    if ((pin_sda < 0) || (pin_sda >= PIN_DEVICE_MAX)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                                           "bad pin for sda"));
    }
    
    baudrate = args[2].u_int; 
    if( eeprom_init(&self_p->drv, &pin_device[pin_scl], &pin_device[pin_sda], baudrate, 50000, 100) != 0 )
    {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError,
                                           "eeprom_init() failed"));        
    }
    return self_p;
   
}
const mp_obj_type_t module_drivers_class_eeprom = {
    { &mp_type_type },
    .name = MP_QSTR_Eeprom,
    .print = class_eeprom_print,
    .make_new = class_eeprom_make_new,
    .locals_dict = (mp_obj_t)&class_eeprom_locals_dict,
};
#endif