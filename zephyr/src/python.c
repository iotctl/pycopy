#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <kernel.h>
#include <console/console.h>
#include <storage/flash_map.h>
#include <logging/log.h>

#include <py/compile.h>
#include <py/gc.h>
#include <py/stackctrl.h>
#include <lib/utils/pyexec.h>

#if MICROPY_VFS
#include <zfs.h>
#endif

LOG_MODULE_REGISTER(python, LOG_LEVEL_INF);

static char heap[MICROPY_HEAP_SIZE];

extern struct fs_mount_t fs_nand_fat;

// TODO create INIT macro
static struct vfs_zephyr_mount vfs_root = {
	.base = {
		.type = &mp_zfs_type 
    },
	.mount = &fs_nand_fat,
	.cwd = MP_OBJ_NEW_QSTR(MP_QSTR_)
};

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    return console_getchar();
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        char c = *str++;
        while (console_putchar(c) == -1) {
            k_sleep(K_MSEC(1));
        }
    }
}

int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_printk(fmt, ap);
    va_end(ap);
    return 0;
}

int python_init(void) {
    mp_stack_ctrl_init();
    // Make MicroPython's stack limit somewhat smaller than full stack available
    mp_stack_set_limit(CONFIG_MAIN_STACK_SIZE - 512);

soft_reset:
    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_init(mp_sys_argv, 0);

    #if MICROPY_VFS
    int res = vfs_zephyr_root(&vfs_root);
	if (!res) {
        // TODO append search path according to root mount point
        // append to search path /NAND:/ and /NAND:/lib
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_NAND_colon__slash_));
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_NAND_colon__slash_lib));
	}
    #endif

    #if MICROPY_MODULE_FROZEN || MICROPY_VFS
    pyexec_file_if_exists("boot.py");
    pyexec_file_if_exists("main.py");
    #endif

    //MP_STATE_PORT(pyb_config_main) = MP_OBJ_NULL;

    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

    gc_sweep_all();
    LOG_DBG("soft reboot");

    #if MICROPY_PY_MACHINE
    machine_pin_deinit();
    #endif

    goto soft_reset;

    return 0;
}

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)MP_STATE_THREAD(stack_top) - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    // gc_dump_info();
}

#if !MICROPY_READER_VFS
mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(ENOENT);
}
#endif

// mp_import_stat_t mp_import_stat(const char *path) {
//     #if MICROPY_VFS
//     return mp_vfs_import_stat(path);
//     #else
//     return MP_IMPORT_STAT_NO_EXIST;
//     #endif
// }

// mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
//     #if MICROPY_VFS
//     return mp_vfs_open(n_args, args, kwargs);
//     #else
//     return mp_const_none;
//     #endif
// }
// MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

FUNC_NORETURN void nlr_jump_fail(void *val) {
    LOG_ERR("");
    k_oops();
    CODE_UNREACHABLE;
}
