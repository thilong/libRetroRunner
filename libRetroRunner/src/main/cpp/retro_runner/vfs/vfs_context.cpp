//
// Created by aidoo on 11/25/2024.
//

#include "vfs_context.h"
#include <libretro-common/include/vfs/vfs.h>
#include <libretro-common/include/vfs/vfs_implementation.h>

namespace libRetroRunner {

    struct retro_vfs_interface VirtualFileSystemContext::vfsInterface = {
            &VirtualFileSystemContext::GetPathImpl,
            &VirtualFileSystemContext::OpenImpl,
            &VirtualFileSystemContext::CloseImpl,
            &VirtualFileSystemContext::SizeImpl,
            &VirtualFileSystemContext::TellImpl,
            &VirtualFileSystemContext::SeekImpl,
            &VirtualFileSystemContext::ReadImpl,
            &VirtualFileSystemContext::Write,
            &VirtualFileSystemContext::FlushImpl,
            &VirtualFileSystemContext::RemoveImpl,
            &VirtualFileSystemContext::RenameImpl,
            &VirtualFileSystemContext::TruncateImpl,
            &VirtualFileSystemContext::StateImpl,
            &VirtualFileSystemContext::MkdirImpl,
            &VirtualFileSystemContext::OpenDirImpl,
            &VirtualFileSystemContext::ReadDirImpl,
            &VirtualFileSystemContext::DirEntGetNameImpl,
            &VirtualFileSystemContext::DirEntIsDirImpl,
            &VirtualFileSystemContext::CloseDirImpl
    };

    const char *VirtualFileSystemContext::GetPathImpl(struct retro_vfs_file_handle *stream) {
        return retro_vfs_file_get_path_impl(stream);
    }

    struct retro_vfs_file_handle *VirtualFileSystemContext::OpenImpl(const char *path, unsigned int mode, unsigned int hints) {
        return retro_vfs_file_open_impl(path, mode, hints);
    }

    int VirtualFileSystemContext::CloseImpl(struct retro_vfs_file_handle *stream) {
        return retro_vfs_file_close_impl(stream);
    }

    int64_t VirtualFileSystemContext::SizeImpl(struct retro_vfs_file_handle *stream) {
        return retro_vfs_file_size_impl(stream);
    }

    int64_t VirtualFileSystemContext::TruncateImpl(struct retro_vfs_file_handle *stream, int64_t length) {
        return retro_vfs_file_truncate_impl(stream, length);
    }

    int64_t VirtualFileSystemContext::TellImpl(struct retro_vfs_file_handle *stream) {
        return retro_vfs_file_tell_impl(stream);
    }

    int64_t VirtualFileSystemContext::SeekImpl(struct retro_vfs_file_handle *stream, int64_t offset, int seek_position) {
        return retro_vfs_file_seek_impl(stream, offset, seek_position);
    }

    int64_t VirtualFileSystemContext::ReadImpl(struct retro_vfs_file_handle *stream, void *s, uint64_t len) {
        return retro_vfs_file_read_impl(stream, s, len);
    }

    int64_t VirtualFileSystemContext::Write(struct retro_vfs_file_handle *stream, const void *s, uint64_t len) {
        return retro_vfs_file_write_impl(stream, s, len);
    }

    int VirtualFileSystemContext::FlushImpl(struct retro_vfs_file_handle *stream) {
        return retro_vfs_file_flush_impl(stream);
    }

    int VirtualFileSystemContext::RemoveImpl(const char *path) {
        return retro_vfs_file_remove_impl(path);
    }

    int VirtualFileSystemContext::RenameImpl(const char *old_path, const char *new_path) {
        return retro_vfs_file_rename_impl(old_path, new_path);
    }

    int VirtualFileSystemContext::StateImpl(const char *path, int32_t *size) {
        return retro_vfs_stat_impl(path, size);
    }

    int VirtualFileSystemContext::MkdirImpl(const char *dir) {
        return retro_vfs_mkdir_impl(dir);
    }

    struct retro_vfs_dir_handle *VirtualFileSystemContext::OpenDirImpl(const char *dir, bool include_hidden) {
        return retro_vfs_opendir_impl(dir, include_hidden);
    }

    bool VirtualFileSystemContext::ReadDirImpl(struct retro_vfs_dir_handle *dirstream) {
        return retro_vfs_readdir_impl(dirstream);
    }

    const char *VirtualFileSystemContext::DirEntGetNameImpl(struct retro_vfs_dir_handle *dirstream) {
        return retro_vfs_dirent_get_name_impl(dirstream);
    }

    bool VirtualFileSystemContext::DirEntIsDirImpl(struct retro_vfs_dir_handle *dirstream) {
        return retro_vfs_dirent_is_dir_impl(dirstream);
    }

    int VirtualFileSystemContext::CloseDirImpl(struct retro_vfs_dir_handle *dirstream) {
        return retro_vfs_closedir_impl(dirstream);
    }
}
