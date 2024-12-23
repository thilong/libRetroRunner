//
// Created by Aidoo.TK on 11/25/2024.
//

#ifndef _VFS_CONTEXT_H
#define _VFS_CONTEXT_H

#include <libretro-common/include/libretro.h>

namespace libRetroRunner {
    class VirtualFileSystemContext {
    public:
        static const char *GetPathImpl(struct retro_vfs_file_handle *stream);

        static struct retro_vfs_file_handle *OpenImpl(const char *path, unsigned mode, unsigned hints);

        static int CloseImpl(struct retro_vfs_file_handle *stream);

        static int64_t SizeImpl(struct retro_vfs_file_handle *stream);

        static int64_t TruncateImpl(struct retro_vfs_file_handle *stream, int64_t length);

        static int64_t TellImpl(struct retro_vfs_file_handle *stream);

        static int64_t SeekImpl(struct retro_vfs_file_handle *stream, int64_t offset, int seek_position);

        static int64_t ReadImpl(struct retro_vfs_file_handle *stream, void *s, uint64_t len);

        static int64_t Write(struct retro_vfs_file_handle *stream, const void *s, uint64_t len);

        static int FlushImpl(struct retro_vfs_file_handle *stream);

        static int RemoveImpl(const char *path);

        static int RenameImpl(const char *old_path, const char *new_path);

        static int StateImpl(const char *path, int32_t *size);

        static int MkdirImpl(const char *dir);

        static struct retro_vfs_dir_handle *OpenDirImpl(const char *dir, bool include_hidden);

        static bool ReadDirImpl(struct retro_vfs_dir_handle *dirstream);

        static const char *DirEntGetNameImpl(struct retro_vfs_dir_handle *dirstream);

        static bool DirEntIsDirImpl(struct retro_vfs_dir_handle *dirstream);

        static int CloseDirImpl(struct retro_vfs_dir_handle *dirstream);


    public:
        static struct retro_vfs_interface vfsInterface;
    };
}
#endif
