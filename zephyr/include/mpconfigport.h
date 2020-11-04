#pragma once

#include <alloca.h>
#include <zephyr.h>

#define MICROPY_ALLOC_PATH_MAX      (512)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)

#define MICROPY_COMP_CONST          (0)
#define MICROPY_COMP_CONST_FOLDING  (0)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (0)
#define MICROPY_COMP_MODULE_CONST   (0)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (0)

#define MICROPY_CPYTHON_COMPAT      (0)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_DEBUG_VERBOSE       (0)
#define MICROPY_EMIT_INLINE_THUMB   (0)
#define MICROPY_EMIT_THUMB          (0)
#define MICROPY_EMIT_X64            (0)
#define MICROPY_ENABLE_DOC_STRING   (0)

#define MICROPY_ENABLE_GC           (1)
#define MICROPY_ENABLE_SCHEDULER    (1)
#define MICROPY_ENABLE_SOURCE_LINE  (1)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE) //
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_FLOAT)
//#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_HELPER_LEXER_UNIX   (1) //
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_LONGLONG)
//#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_MEM_STATS           (0) //
#define MICROPY_MODULE_FROZEN_MPY   (0) //
#define MICROPY_MODULE_FROZEN_STR   (1)
#define MICROPY_MODULE_WEAK_LINKS   (1)
#define MICROPY_PY_ARRAY            (0)
#define MICROPY_PY_ASYNC_AWAIT      (1)
#define MICROPY_PY_ATTRTUPLE        (0)
#define MICROPY_PY_BUILTINS_BYTEARRAY (0)
#define MICROPY_PY_BUILTINS_COMPLEX (0)
#define MICROPY_PY_BUILTINS_ENUMERATE (0)
#define MICROPY_PY_BUILTINS_FILTER  (0)
#define MICROPY_PY_BUILTINS_HELP    (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT zephyr_help_text
#define MICROPY_PY_BUILTINS_MIN_MAX (0)
#define MICROPY_PY_BUILTINS_PROPERTY (0)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS (0)
#define MICROPY_PY_BUILTINS_REVERSED (0)
#define MICROPY_PY_BUILTINS_SET     (0)
#define MICROPY_PY_BUILTINS_SLICE   (0)
#define MICROPY_PY_BUILTINS_STR_COUNT (0)
#define MICROPY_PY_CMATH            (0)
#define MICROPY_PY_COLLECTIONS      (1) 
#define MICROPY_PY___FILE__         (0) //
#define MICROPY_PY_GC               (1) //
#define MICROPY_PY_IO               (0)
#define MICROPY_PY_IO_FILEIO        (1) //
#define MICROPY_PY_MACHINE          (0)
#define MICROPY_PY_MACHINE_I2C      (0)
#define MICROPY_PY_MACHINE_I2C_MAKE_NEW machine_hard_i2c_make_new
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW mp_pin_make_new
#define MICROPY_PY_MATH             (0) //
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_PY_STRUCT           (0)
#define MICROPY_PY_SYS              (1)
#define MICROPY_PY_SYS_MODULES      (0)
#define MICROPY_PY_UBINASCII        (1)
#define MICROPY_PY_UHASHLIB         (1)
#define MICROPY_PY_UOS              (1)
#define MICROPY_PY_UTIME            (1)
#define MICROPY_PY_UTIME_MP_HAL     (1)
#define MICROPY_PY_ZEPHYR           (1)
#define MICROPY_PY_ZSENSOR          (0)
#define MICROPY_QSTR_BYTES_IN_HASH  (1) //
#define MICROPY_READER_POSIX        (0) //
#define MICROPY_READER_VFS          (MICROPY_VFS)
#define MICROPY_REPL_AUTO_INDENT    (1)
#define MICROPY_STACK_CHECK         (1)
#define MICROPY_USE_INTERNAL_PRINTF (0)
#define MICROPY_VFS                 (1)
#define MICROPY_PY_UHEAPQ           (1)

#define MICROPY_HEAP_SIZE (16 * 1024)

#ifdef CONFIG_NETWORKING
// If we have networking, we likely want errno comfort
#define MICROPY_PY_UERRNO           (1)
#define MICROPY_PY_USOCKET          (1)
#endif

#define MICROPY_PY_SYS_PLATFORM "zephyr"
#define MICROPY_HW_BOARD_NAME "zephyr-" CONFIG_BOARD
#define MICROPY_HW_MCU_NAME CONFIG_SOC

typedef int mp_int_t; // must be pointer size
typedef unsigned mp_uint_t; // must be pointer size
typedef long mp_off_t;

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8]; \
    void *machine_pin_irq_list; /* Linked list of pin irq objects */

extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t mp_module_time;
extern const struct _mp_obj_module_t mp_module_uos;
extern const struct _mp_obj_module_t mp_module_usocket;
extern const struct _mp_obj_module_t mp_module_zephyr;
extern const struct _mp_obj_module_t mp_module_zsensor;

#if MICROPY_PY_UOS
#define MICROPY_PY_UOS_DEF { MP_ROM_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_uos) },
#else
#define MICROPY_PY_UOS_DEF
#endif

#if MICROPY_PY_USOCKET
#define MICROPY_PY_USOCKET_DEF { MP_ROM_QSTR(MP_QSTR_usocket), MP_ROM_PTR(&mp_module_usocket) },
#else
#define MICROPY_PY_USOCKET_DEF
#endif

#if MICROPY_PY_UTIME
#define MICROPY_PY_UTIME_DEF { MP_ROM_QSTR(MP_QSTR_utime), MP_ROM_PTR(&mp_module_time) },
#else
#define MICROPY_PY_UTIME_DEF
#endif

#if MICROPY_PY_ZEPHYR
#define MICROPY_PY_ZEPHYR_DEF { MP_ROM_QSTR(MP_QSTR_zephyr), MP_ROM_PTR(&mp_module_zephyr) },
#else
#define MICROPY_PY_ZEPHYR_DEF
#endif

#if MICROPY_PY_ZSENSOR
#define MICROPY_PY_ZSENSOR_DEF { MP_ROM_QSTR(MP_QSTR_zsensor), MP_ROM_PTR(&mp_module_zsensor) },
#else
#define MICROPY_PY_ZSENSOR_DEF
#endif

#if MICROPY_PY_MACHINE
#define MICROPY_PY_MACHINE_DEF { MP_ROM_QSTR(MP_QSTR_machine), MP_ROM_PTR(&mp_module_machine) }, 
#else
#define MICROPY_PY_MACHINE_DEF
#endif

#define MICROPY_PORT_BUILTIN_MODULES \
    MICROPY_PY_MACHINE_DEF \
    MICROPY_PY_UOS_DEF \
    MICROPY_PY_USOCKET_DEF \
    MICROPY_PY_UTIME_DEF \
    MICROPY_PY_ZEPHYR_DEF \
    MICROPY_PY_ZSENSOR_DEF \

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },

#define mp_import_stat mp_vfs_import_stat
#define mp_builtin_open mp_vfs_open
#define mp_builtin_open_obj mp_vfs_open_obj
#define MICROPY_BEGIN_ATOMIC_SECTION irq_lock
#define MICROPY_END_ATOMIC_SECTION irq_unlock
