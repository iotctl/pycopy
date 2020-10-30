#include <string.h>
#include "py/mpconfig.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "lib/timeutils/timeutils.h"
#include "zfs.h"
#include <fs/fs.h>
#include <fs/fs_sys.h>
// TODO gc hook to close the file if not already closed

struct zfs_file_obj_t {
	mp_obj_base_t base;
	struct fs_file_t file;
};

static mp_obj_t file_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void file_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_uint_t file_obj_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode);
static mp_uint_t file_obj_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode);
static mp_uint_t file_obj_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode);
static mp_obj_t file_obj___exit__(size_t n_args, const mp_obj_t *args);
static mp_obj_t file_open(struct vfs_zephyr_mount *vfs, const mp_obj_type_t *type, mp_arg_val_t *args);
static mp_obj_t zfs_builtin_open_self(mp_obj_t self_in, mp_obj_t path, mp_obj_t mode);

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(file_obj___exit___obj, 4, 4, file_obj___exit__);

static const mp_rom_map_elem_t rawfile_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
	{ MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
	{ MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&file_obj___exit___obj) },
};

static MP_DEFINE_CONST_DICT(rawfile_locals_dict, rawfile_locals_dict_table);

// Note: encoding is ignored for now; it's also not a valid kwarg for CPython's FileIO,
// but by adding it here we can use one single mp_arg_t array for open() and FileIO's constructor
static const mp_arg_t file_open_args[] = {
	{ MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_rom_obj = MP_ROM_PTR(&mp_const_none_obj) } },
	{ MP_QSTR_mode, MP_ARG_OBJ, { .u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_r) } },
	{ MP_QSTR_encoding, MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_rom_obj = MP_ROM_PTR(&mp_const_none_obj) } },
};
#define FILE_OPEN_NUM_ARGS MP_ARRAY_SIZE(file_open_args)

static const mp_stream_p_t textio_stream_p = {
	.read = file_obj_read,
	.write = file_obj_write,
	.ioctl = file_obj_ioctl,
	.is_text = true,
};

#if MICROPY_PY_IO_FILEIO
static const mp_stream_p_t fileio_stream_p = {
	.read = file_obj_read,
	.write = file_obj_write,
	.ioctl = file_obj_ioctl,
};

const mp_obj_type_t mp_type_vfs_fat_fileio = {
	{ &mp_type_type },
	.name = MP_QSTR_FileIO,
	.print = file_obj_print,
	.make_new = file_obj_make_new,
	.getiter = mp_identity_getiter,
	.iternext = mp_stream_unbuffered_iter,
	.protocol = &fileio_stream_p,
	.locals_dict = (mp_obj_dict_t *)&rawfile_locals_dict,
};
#endif

const mp_obj_type_t mp_type_vfs_fat_textio = {
	{ &mp_type_type },
	.name = MP_QSTR_TextIOWrapper,
	.print = file_obj_print,
	.make_new = file_obj_make_new,
	.getiter = mp_identity_getiter,
	.iternext = mp_stream_unbuffered_iter,
	.protocol = &textio_stream_p,
	.locals_dict = (mp_obj_dict_t *)&rawfile_locals_dict,
};

MP_DEFINE_CONST_FUN_OBJ_3(zfs_open_obj, zfs_builtin_open_self);

static mp_obj_t file_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
	mp_arg_val_t arg_vals[FILE_OPEN_NUM_ARGS];
	mp_arg_parse_all_kw_array(n_args, n_kw, args, FILE_OPEN_NUM_ARGS, file_open_args, arg_vals);
	return file_open(NULL, type, arg_vals);
}

static void file_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
	(void)kind;
	mp_printf(print, "<io.%s %p>", mp_obj_get_type_str(self_in), MP_OBJ_TO_PTR(self_in));
}

static mp_uint_t file_obj_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode)
{
	struct zfs_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

	ssize_t res = self->file.mp->fs->read(&self->file, buf, size);
	if (res < 0) {
		*errcode = res;
		return MP_STREAM_ERROR;
	}
	return res;
}

static mp_uint_t file_obj_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode)
{
	struct zfs_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

	ssize_t res = self->file.mp->fs->write(&self->file, buf, size);
	if (res < 0) {
		*errcode = res;
		return MP_STREAM_ERROR;
	}

	if (res != size) {
		// The FatFS documentation says that this means disk full.
		*errcode = MP_ENOSPC;
		return MP_STREAM_ERROR;
	}
	return res;
}

static mp_uint_t file_obj_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode)
{
	struct zfs_file_obj_t *self = MP_OBJ_TO_PTR(o_in);

	if (request == MP_STREAM_SEEK) {
		struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)(uintptr_t)arg;

#if FS_SEEK_SET != MP_SEEK_SET || FS_SEEK_CUR != MP_SEEK_CUR || FS_SEEK_END != MP_SEEK_END
#error "FIXME: Zehpyr, Micropython stream interface incompatibility"
#endif
		int res = self->file.mp->fs->lseek(&self->file, s->offset, s->whence);
		if (res < 0) {
			*errcode = res;
			return MP_STREAM_ERROR;
		}
		s->offset = self->file.mp->fs->tell(&self->file);
		return 0;

	} else if (request == MP_STREAM_FLUSH) {
		int res = self->file.mp->fs->sync(&self->file);
		if (res < 0) {
			*errcode = res;
			return MP_STREAM_ERROR;
		}
		return 0;

	} else if (request == MP_STREAM_CLOSE) {
		// if file.mp==NULL then the file is closed and in that case this method is a no-op
		if (self->file.mp != NULL) {
			int res = self->file.mp->fs->close(&self->file);
			if (res < 0) {
				*errcode = res;
				return MP_STREAM_ERROR;
			}
		}
		return 0;

	} else {
		*errcode = MP_EINVAL;
		return MP_STREAM_ERROR;
	}
}

static mp_obj_t file_obj___exit__(size_t n_args, const mp_obj_t *args)
{
	(void)n_args;
	return mp_stream_close(args[0]);
}

static mp_obj_t file_open(struct vfs_zephyr_mount *vfs, const mp_obj_type_t *type, mp_arg_val_t *args)
{
	int mode = 0;
	const char *mode_s = mp_obj_str_get_str(args[1].u_obj);

	// TODO make sure only one of r, w, x, a, and b, t are specified
	while (*mode_s) {
		switch (*mode_s++) {
			// TODO: fs API currenty does not support permissions
		case 'r':
			mode |= FS_O_READ;
			break;
		case 'w':
			mode |= FS_O_WRITE | FS_O_CREATE;
			break;
		case 'x':
			mode |= FS_O_WRITE | FS_O_CREATE;
			break;
		case 'a':
			mode |= FS_O_WRITE | FS_O_APPEND;
			break;
		case '+':
			mode |= FS_O_READ | FS_O_WRITE;
			break;

#if MICROPY_PY_IO_FILEIO
		case 'b':
			type = &mp_type_vfs_fat_fileio;
			break;
#endif
		case 't':
			type = &mp_type_vfs_fat_textio;
			break;
		}
	}

	struct zfs_file_obj_t *o = m_new_obj_with_finaliser(struct zfs_file_obj_t);
	o->base.type = type;
	o->file.mp = vfs->mount;

	const char *fname = zfs_get_path_obj(vfs, args[0].u_obj);
	assert(vfs != NULL);

	// TODO check that fname include root prefix, it should be added by vfs.c:278
	int res = vfs->mount->fs->open(&o->file, fname, mode);
	if (res < 0) {
		m_del_obj(struct zfs_file_obj_t, o);
		mp_raise_OSError(res);
	}

	// for 'a' mode, we must begin at the end of the file
	if ((mode & FS_O_APPEND) != 0) {
		vfs->mount->fs->lseek(&o->file, FS_SEEK_END, 0);
	}
	return MP_OBJ_FROM_PTR(o);
}

// Factory function for I/O stream classes
static mp_obj_t zfs_builtin_open_self(mp_obj_t self_in, mp_obj_t path, mp_obj_t mode)
{
	// TODO: analyze buffering args and instantiate appropriate type
	struct vfs_zephyr_mount *self = MP_OBJ_TO_PTR(self_in);
	mp_arg_val_t arg_vals[FILE_OPEN_NUM_ARGS];
	arg_vals[0].u_obj = path;
	arg_vals[1].u_obj = mode;
	arg_vals[2].u_obj = mp_const_none;
	return file_open(self, &mp_type_vfs_fat_textio, arg_vals);
}
