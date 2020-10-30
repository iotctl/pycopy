#include <fs/fs.h>
#include <py/obj.h>
#include <extmod/vfs.h>

#define ZFS_NATIVE       (0x0001) // readblocks[2]/writeblocks[2] contain native func
#define ZFS_FREE_OBJ     (0x0002) // fs_user_mount_t obj should be freed on umount
#define ZFS_HAVE_IOCTL   (0x0004) // new protocol with ioctl
#define ZFS_NO_FILESYSTEM (0x0008) // the block device has no filesystem on it

struct vfs_zephyr_mount {
  mp_obj_base_t base;
  uint16_t flags;
  struct fs_mount_t *mount;
  mp_obj_t cwd;
};

extern const mp_obj_type_t mp_zfs_type;
extern const mp_obj_type_t mp_type_vfs_fat_fileio;
extern const mp_obj_type_t mp_type_vfs_fat_textio;

MP_DECLARE_CONST_FUN_OBJ_3(zfs_open_obj);

const char *zfs_get_path_obj(void *self, mp_obj_t *path_in);

int vfs_zephyr_root(struct vfs_zephyr_mount *vfs_obj);

//mp_fat_vfs_type

