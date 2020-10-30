#include "py/mpconfig.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include <string.h>
#include <py/objstr.h>

#include "lib/timeutils/timeutils.h"
#include "zfs.h"
#include <fs/fs_sys.h>

#define mp_obj_zfs_t struct vfs_zephyr_mount

struct mp_zfs_ilistdir_it_t {
	mp_obj_base_t base;
	mp_fun_1_t iternext;
	bool is_str;
	struct fs_dir_t dir;
};

static const char*zfs_get_path_str(void *self, const char *path_in);
static mp_obj_t zfs_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_import_stat_t zfs_import_stat(void *vfs_in, const char *path);
static mp_obj_t zfs_mkfs(mp_obj_t bdev_in);
static mp_obj_t mp_zfs_ilistdir_it_iternext(mp_obj_t self_in);
static mp_obj_t zfs_ilistdir_func(size_t n_args, const mp_obj_t *args);
static mp_obj_t zfs_remove(mp_obj_t vfs_in, mp_obj_t path_in);
static mp_obj_t zfs_rmdir(mp_obj_t vfs_in, mp_obj_t path_in);
static mp_obj_t zfs_rename(mp_obj_t vfs_in, mp_obj_t path_in, mp_obj_t path_out);
static mp_obj_t zfs_mkdir(mp_obj_t vfs_in, mp_obj_t path_o);
static mp_obj_t zfs_chdir(mp_obj_t vfs_in, mp_obj_t path_in);
static mp_obj_t zfs_getcwd(mp_obj_t vfs_in);
static mp_obj_t zfs_stat(mp_obj_t vfs_in, mp_obj_t path_in);
static mp_obj_t zfs_statvfs(mp_obj_t vfs_in, mp_obj_t path_in);
static mp_obj_t zfs_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs);
static mp_obj_t zfs_umount(mp_obj_t self_in);

#if _FS_REENTRANT
static mp_obj_t zfs_del(mp_obj_t self_in);
static MP_DEFINE_CONST_FUN_OBJ_1(zfs_del_obj, zfs_del);
#endif

STATIC MP_DEFINE_CONST_FUN_OBJ_1(zfs_mkfs_fun_obj, zfs_mkfs);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(zfs_mkfs_obj, MP_ROM_PTR(&zfs_mkfs_fun_obj));
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(zfs_ilistdir_obj, 1, 2, zfs_ilistdir_func);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(zfs_remove_obj, zfs_remove);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(zfs_rmdir_obj, zfs_rmdir);
STATIC MP_DEFINE_CONST_FUN_OBJ_3(zfs_rename_obj, zfs_rename);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(zfs_mkdir_obj, zfs_mkdir);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(zfs_chdir_obj, zfs_chdir);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(zfs_getcwd_obj, zfs_getcwd);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(zfs_stat_obj, zfs_stat);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(zfs_statvfs_obj, zfs_statvfs);
STATIC MP_DEFINE_CONST_FUN_OBJ_3(zfs_mount_obj, zfs_mount);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(zfs_umount_obj, zfs_umount);

static const mp_rom_map_elem_t zfs_locals_dict_table[] = {
#if _FS_REENTRANT
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&zfs_del_obj) },
#endif
	{ MP_ROM_QSTR(MP_QSTR_mkfs), MP_ROM_PTR(&zfs_mkfs_obj) },
	{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&zfs_open_obj) },
	{ MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&zfs_ilistdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&zfs_mkdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&zfs_rmdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&zfs_chdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&zfs_getcwd_obj) },
	{ MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&zfs_remove_obj) },
	{ MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&zfs_rename_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&zfs_stat_obj) },
	{ MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&zfs_statvfs_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&zfs_mount_obj) },
	{ MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&zfs_umount_obj) },
};
static MP_DEFINE_CONST_DICT(zfs_locals_dict, zfs_locals_dict_table);

static const mp_vfs_proto_t zfs_proto = { .import_stat = zfs_import_stat };

const mp_obj_type_t mp_zfs_type = {
	{ &mp_type_type },
	.name = MP_QSTR_Zfs,
	.make_new = zfs_make_new,
	.protocol = &zfs_proto,
	.locals_dict = (mp_obj_dict_t *)&zfs_locals_dict,
};

static mp_obj_t zfs_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
	mp_arg_check_num(n_args, n_kw, 1, 1, false);

	// create new object
	struct vfs_zephyr_mount *vfs = m_new_obj(struct vfs_zephyr_mount);
	vfs->base.type = type;
	vfs->flags = ZFS_FREE_OBJ;

	// load block protocol methods
	/*
  mp_load_method(args[0], MP_QSTR_readblocks, vfs->readblocks);
  mp_load_method_maybe(args[0], MP_QSTR_writeblocks, vfs->writeblocks);
  mp_load_method_maybe(args[0], MP_QSTR_ioctl, vfs->u.ioctl);
  if (vfs->u.ioctl[0] != MP_OBJ_NULL) {
    // device supports new block protocol, so indicate it
    vfs->flags |= FSUSER_HAVE_IOCTL;
  } else {
    // no ioctl method, so assume the device uses the old block protocol
    mp_load_method_maybe(args[0], MP_QSTR_sync, vfs->u.old.sync);
    mp_load_method(args[0], MP_QSTR_count, vfs->u.old.count);
  }
  // mount the block device so the VFS methods can be used
  FRESULT res = f_mount(&vfs->fatfs);
  if (res == FR_NO_FILESYSTEM) {
    // don't error out if no filesystem, to let mkfs()/mount() create one if
    // wanted
    vfs->flags |= FSUSER_NO_FILESYSTEM;
  } else if (res != FR_OK) {
    mp_raise_OSError(fresult_to_errno_table[res]);
  }
   */
	// TODO arguments for flash dev / disk dev

	return MP_OBJ_FROM_PTR(vfs);
}

#if _FS_REENTRANT
static mp_obj_t zfs_del(mp_obj_t self_in)
{
	struct vfs_zephyr_mount *self = MP_OBJ_TO_PTR(self_in);
	// umount only needs to be called to release the sync object
	self->mount->fs->unmount(self->mount);
	return mp_const_none;
}
#endif

static mp_obj_t zfs_mkfs(mp_obj_t bdev_in)
{
	// create new object
	/*
	fs_user_mount_t *vfs = MP_OBJ_TO_PTR(fat_vfs_make_new(&mp_fat_vfs_type, 1, 0, &bdev_in));

	// make the filesystem
	uint8_t working_buf[FF_MAX_SS];
	FRESULT res = f_mkfs(&vfs->fatfs, FM_FAT | FM_SFD, 0, working_buf, sizeof(working_buf));
	if (res == FR_MKFS_ABORTED) { // Probably doesn't support FAT16
		res = f_mkfs(&vfs->fatfs, FM_FAT32, 0, working_buf, sizeof(working_buf));
	}
	if (res != FR_OK) {
		mp_raise_OSError(fresult_to_errno_table[res]);
	}
	 return mp_const_none;
	*/
	mp_raise_NotImplementedError("mkfs is not implemented yet");
}

static mp_obj_t mp_zfs_ilistdir_it_iternext(mp_obj_t self_in)
{
	struct mp_zfs_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);

	for (;;) {
		struct fs_dirent entry;
		int res = self->dir.mp->fs->readdir(&self->dir, &entry);

		char *fn = entry.name;
		if (res < 0 || fn[0] == 0) {
			// stop on error or end of dir
			break;
		}

		// Note that FatFS already filters . and .., so we don't need to

		// make 4-tuple with info about this entry
		mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(4, NULL));
		if (self->is_str) {
			t->items[0] = mp_obj_new_str(fn, strlen(fn));
		} else {
			t->items[0] = mp_obj_new_bytes((const byte *)fn, strlen(fn));
		}
		if (entry.type == FS_DIR_ENTRY_DIR) {
			// dir
			t->items[1] = MP_OBJ_NEW_SMALL_INT(MP_S_IFDIR);
		} else {
			// file
			t->items[1] = MP_OBJ_NEW_SMALL_INT(MP_S_IFREG);
		}
		t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // no inode number
		t->items[3] = mp_obj_new_int_from_uint(entry.size);

		return MP_OBJ_FROM_PTR(t);
	}

	// ignore error because we may be closing a second time
	self->dir.mp->fs->closedir(&self->dir);

	return MP_OBJ_STOP_ITERATION;
}

static mp_obj_t zfs_ilistdir_func(size_t n_args, const mp_obj_t *args)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(args[0]);
	bool is_str_type = true;
	const char *path;

	if (n_args == 2) {
		if (mp_obj_get_type(args[1]) == &mp_type_bytes) {
			is_str_type = false;
		}
		path = zfs_get_path_obj(self, args[1]);
	} else {
		path = zfs_get_path_str(self, "");
	}

	// Create a new iterator object to list the dir
	struct mp_zfs_ilistdir_it_t *iter = m_new_obj(struct mp_zfs_ilistdir_it_t);
	iter->base.type = &mp_type_polymorph_iter;
	iter->iternext = mp_zfs_ilistdir_it_iternext;
	iter->is_str = is_str_type;
	iter->dir.mp = self->mount;

	int res = self->mount->fs->opendir(&iter->dir, path);
	if (res < 0) {
		mp_raise_OSError(res);
	}

	return MP_OBJ_FROM_PTR(iter);
}

static mp_obj_t zfs_remove_internal(mp_obj_t vfs_in, mp_obj_t path_in, enum fs_dir_entry_type type)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	const char *path = zfs_get_path_obj(self, path_in);
	struct fs_dirent entry;

	int res = self->mount->fs->stat(self->mount, path, &entry);
	if (res < 0) {
		mp_raise_OSError(res);
	}

	// check if path is a file or directory
	if (entry.type != type) {
		mp_raise_OSError(entry.type == FS_DIR_ENTRY_FILE ? MP_ENOTDIR : MP_EISDIR);
	}

	res = self->mount->fs->unlink(self->mount, path);

	if (res < 0) {
		mp_raise_OSError(res);
	}
	return mp_const_none;
}

static mp_obj_t zfs_remove(mp_obj_t vfs_in, mp_obj_t path_in)
{
	return zfs_remove_internal(vfs_in, path_in, FS_DIR_ENTRY_FILE);
}

static mp_obj_t zfs_rmdir(mp_obj_t vfs_in, mp_obj_t path_in)
{
	return zfs_remove_internal(vfs_in, path_in, FS_DIR_ENTRY_DIR);
}

static mp_obj_t zfs_rename(mp_obj_t vfs_in, mp_obj_t path_in, mp_obj_t path_out)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	const char *old_path = zfs_get_path_obj(self, path_in);
	const char *new_path = zfs_get_path_obj(self, path_out);

	// TODO if path is relative it must be counted to absolute: mount point root + cwd + path
	int res = self->mount->fs->rename(self->mount, old_path, new_path);

	if (res < 0) {
		mp_raise_OSError(res);
	}

	return mp_const_none;
}

static mp_obj_t zfs_mkdir(mp_obj_t vfs_in, mp_obj_t path_o)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	const char *path = zfs_get_path_obj(self, path_o);

	// TODO if path is relative it must be counted to absolute: mount point root + cwd + path
	int res = self->mount->fs->mkdir(self->mount, path);

	if (res < 0) {
		mp_raise_OSError(res);
	}

	return mp_const_none;
}

/// Change current directory.
static mp_obj_t zfs_chdir(mp_obj_t vfs_in, mp_obj_t path_in)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	const char *path = zfs_get_path_str(self, path_in);


	// 1. make absolute path
	// 2. stat if it exists
	// 3. update mount cwd field
	/*
	FRESULT res = f_chdir(&self->fatfs, path);

	if (res != FR_OK) {
		mp_raise_OSError(fresult_to_errno_table[res]);
	}
	*/

	return mp_const_none;
}

/// Get the current directory.
static mp_obj_t zfs_getcwd(mp_obj_t vfs_in)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	return self->cwd;
}

/// \function stat(path)
/// Get the status of a file or directory.
static mp_obj_t zfs_stat(mp_obj_t vfs_in, mp_obj_t path_in)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	const char *path = zfs_get_path_obj(self, path_in);
	struct fs_dirent entry;

	int res = self->mount->fs->stat(self->mount, path, &entry);
	if (res < 0) {
		mp_raise_OSError(res);
	}

	/*
	FILINFO fno;
	if (path[0] == 0 || (path[0] == '/' && path[1] == 0)) {
		// stat root directory
		fno.fsize = 0;
		fno.fdate = 0x2821; // Jan 1, 2000
		fno.ftime = 0;
		fno.fattrib = AM_DIR;
	} else {
		FRESULT res = f_stat(&self->fatfs, path, &fno);
		if (res != FR_OK) {
			mp_raise_OSError(fresult_to_errno_table[res]);
		}
	}
	 */

	mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
	mp_int_t mode = 0;
	if (entry.type == FS_DIR_ENTRY_DIR) {
		mode |= MP_S_IFDIR;
	} else {
		mode |= MP_S_IFREG;
	}
	//	mp_int_t seconds = timeutils_seconds_since_2000(1980 + ((fno.fdate >> 9) & 0x7f), (fno.fdate >> 5) & 0x0f,
	//							fno.fdate & 0x1f, (fno.ftime >> 11) & 0x1f,
	//							(fno.ftime >> 5) & 0x3f, 2 * (fno.ftime & 0x1f));
	t->items[0] = MP_OBJ_NEW_SMALL_INT(mode); // st_mode
	t->items[1] = MP_OBJ_NEW_SMALL_INT(0); // st_ino
	t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // st_dev
	t->items[3] = MP_OBJ_NEW_SMALL_INT(0); // st_nlink
	t->items[4] = MP_OBJ_NEW_SMALL_INT(0); // st_uid
	t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // st_gid
	t->items[6] = mp_obj_new_int_from_uint(entry.size); // st_size
	t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // st_atime
	t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // st_mtime
	t->items[9] = MP_OBJ_NEW_SMALL_INT(0); // st_ctime

	return MP_OBJ_FROM_PTR(t);
}

// Get the status of a VFS.
static mp_obj_t zfs_statvfs(mp_obj_t vfs_in, mp_obj_t path_in)
{
	mp_obj_zfs_t *self = MP_OBJ_TO_PTR(vfs_in);
	const char *path = zfs_get_path_obj(self, path_in);
	struct fs_statvfs stat;

	int res = self->mount->fs->statvfs(self->mount, path, &stat);

	if (res < 0) {
		mp_raise_OSError(res);
	}

	mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
	t->items[0] = MP_OBJ_NEW_SMALL_INT(stat.f_bsize); // f_bsize
	t->items[1] = MP_OBJ_NEW_SMALL_INT(stat.f_frsize); // f_frsize
	t->items[2] = MP_OBJ_NEW_SMALL_INT(stat.f_blocks); // f_blocks
	t->items[3] = MP_OBJ_NEW_SMALL_INT(stat.f_bfree); // f_bfree
	t->items[4] = t->items[3]; // f_bavail
	t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // f_files
	t->items[6] = MP_OBJ_NEW_SMALL_INT(0); // f_ffree
	t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // f_favail
	t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // f_flags
	t->items[9] = MP_OBJ_NEW_SMALL_INT(0); // f_namemax
	// FIXME filesystem dependent, for fatfs we should use FF_MAX_LFN
	//t->items[9] = MP_OBJ_NEW_SMALL_INT(FF_MAX_LFN); // f_namemax

	return MP_OBJ_FROM_PTR(t);
}

static mp_obj_t zfs_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs)
{
	struct vfs_zephyr_mount *self = MP_OBJ_TO_PTR(self_in);

	// Read-only device indicated by writeblocks[0] == MP_OBJ_NULL.
	// User can specify read-only device by:
	//  1. readonly=True keyword argument
	//  2. nonexistent writeblocks method (then writeblocks[0] == MP_OBJ_NULL already)
	/*
	if (mp_obj_is_true(readonly)) {
		self->writeblocks[0] = MP_OBJ_NULL;
	}

	// check if we need to make the filesystem
	FRESULT res = (self->flags & FSUSER_NO_FILESYSTEM) ? FR_NO_FILESYSTEM : FR_OK;
	if (res == FR_NO_FILESYSTEM && mp_obj_is_true(mkfs)) {
		uint8_t working_buf[FF_MAX_SS];
		res = f_mkfs(&self->fatfs, FM_FAT | FM_SFD, 0, working_buf, sizeof(working_buf));
	}
	if (res != FR_OK) {
		mp_raise_OSError(fresult_to_errno_table[res]);
	}
	self->flags &= ~FSUSER_NO_FILESYSTEM;
	 return mp_const_none;
	 */

	mp_raise_NotImplementedError("mount is not implemented yet");
}

static mp_obj_t zfs_umount(mp_obj_t self_in)
{
	(void)self_in;
	// keep the FAT filesystem mounted internally so the VFS methods can still be used
	return mp_const_none;
}

static mp_import_stat_t zfs_import_stat(void *vfs_in, const char *path_in)
{
	struct vfs_zephyr_mount *vfs = vfs_in;
	const char *path = zfs_get_path_str(vfs, path_in);
	struct fs_dirent entry;
	int rc;

	assert(vfs != NULL);
	assert(vfs->mount->fs != NULL);

	rc = vfs->mount->fs->stat(vfs->mount, path, &entry);

	if (rc < 0) {
		return MP_IMPORT_STAT_NO_EXIST;
	}

	return entry.type == FS_DIR_ENTRY_FILE ? MP_IMPORT_STAT_FILE : MP_IMPORT_STAT_DIR;
}

static const char*zfs_get_path_str(void *self, const char *path)
{
	struct vfs_zephyr_mount *vfs = self;
	const struct fs_mount_t *fs = vfs->mount;
	vstr_t *zfs_path;
	GET_STR_LEN(vfs->cwd, len);

	if (*path == '/') {
		zfs_path = vstr_new(fs->mountp_len + strlen(path) + 1);
		vstr_add_str(zfs_path, fs->mnt_point);
		vstr_add_str(zfs_path, path);
	} else {
		zfs_path = vstr_new(fs->mountp_len + len + (len != 0) + strlen(path) + 2);
		vstr_add_str(zfs_path, fs->mnt_point);
		vstr_add_char(zfs_path, '/');
		if(len) {
			vstr_add_str(zfs_path, vfs->cwd);
			vstr_add_char(zfs_path, '/');
		}
		vstr_add_str(zfs_path, path);
	}

	return vstr_str(zfs_path);
}

const char*zfs_get_path_obj(void *self, mp_obj_t *path_in)
{
	const char *path = mp_obj_str_get_str(path_in);
	return zfs_get_path_str(self, path);
}

int vfs_zephyr_root(struct vfs_zephyr_mount *vfs_obj)
{
	// mount the flash device (there should be no other devices mounted at this
	// point) we allocate this structure on the heap because vfs->next is a root
	// pointer
	mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
	if (vfs == NULL) {
		return -ENOMEM;
	}

	int res = fs_mount(vfs_obj->mount);
	if(res < 0 && res != -EBUSY) {
		return res;
	}

	vfs->str = vfs_obj->mount->mnt_point;
	vfs->len = vfs_obj->mount->mountp_len;
	vfs->obj = MP_OBJ_FROM_PTR(vfs_obj);
	vfs->next = NULL;
	MP_STATE_VM(vfs_mount_table) = vfs;

	// The current directory is used as the boot up directory.
	// It is set to the internal flash filesystem by default.
	MP_STATE_PORT(vfs_cur) = vfs;
	return 0;
}