
#include "py/mpconfig.h"
#if MICROPY_PY_ZEPHYR

#include <stdio.h>
#include <zephyr.h>
#include <debug/thread_analyzer.h>

#include "modzephyr.h"
#include "py/runtime.h"

STATIC mp_obj_t mod_is_preempt_thread(void) {
    return mp_obj_new_bool(k_is_preempt_thread());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_is_preempt_thread_obj, mod_is_preempt_thread);

STATIC mp_obj_t mod_current_tid(void) {
    return MP_OBJ_NEW_SMALL_INT(k_current_get());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_current_tid_obj, mod_current_tid);

#ifdef CONFIG_THREAD_ANALYZER
STATIC mp_obj_t mod_thread_analyze(void) {
    thread_analyzer_print();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_thread_analyze_obj, mod_thread_analyze);
#endif

#ifdef CONFIG_NET_SHELL

// int net_shell_cmd_iface(int argc, char *argv[]);

STATIC mp_obj_t mod_shell_net_iface(void) {
    net_shell_cmd_iface(0, NULL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_shell_net_iface_obj, mod_shell_net_iface);

#endif // CONFIG_NET_SHELL

STATIC const mp_rom_map_elem_t mp_module_time_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_zephyr) },
    { MP_ROM_QSTR(MP_QSTR_is_preempt_thread), MP_ROM_PTR(&mod_is_preempt_thread_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_tid), MP_ROM_PTR(&mod_current_tid_obj) },
    #ifdef CONFIG_THREAD_ANALYZER
    { MP_ROM_QSTR(MP_QSTR_thread_analyze), MP_ROM_PTR(&mod_thread_analyze_obj) },
    #endif

    #ifdef CONFIG_NET_SHELL
    { MP_ROM_QSTR(MP_QSTR_shell_net_iface), MP_ROM_PTR(&mod_shell_net_iface_obj) },
    #endif
    #ifdef CONFIG_DISK_ACCESS
    { MP_ROM_QSTR(MP_QSTR_DiskAccess), MP_ROM_PTR(&zephyr_disk_access_type) },
    #endif
    #ifdef CONFIG_FLASH_MAP
    { MP_ROM_QSTR(MP_QSTR_FlashArea), MP_ROM_PTR(&zephyr_flash_area_type) },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(mp_module_time_globals, mp_module_time_globals_table);

const mp_obj_module_t mp_module_test = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_time_globals,
};

#endif // MICROPY_PY_ZEPHYR
