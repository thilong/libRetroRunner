//
// Created by aidoo on 11/25/2024.
//

#include "vfs_context.h"

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
        return nullptr;
    }

    struct retro_vfs_file_handle *VirtualFileSystemContext::OpenImpl(const char *path, unsigned int mode, unsigned int hints) {
        return nullptr;
    }

    int VirtualFileSystemContext::CloseImpl(struct retro_vfs_file_handle *stream) {
        return 0;
    }

    int64_t VirtualFileSystemContext::SizeImpl(struct retro_vfs_file_handle *stream) {
        return 0;
    }

    int64_t VirtualFileSystemContext::TruncateImpl(struct retro_vfs_file_handle *stream, int64_t length) {
        return 0;
    }

    int64_t VirtualFileSystemContext::TellImpl(struct retro_vfs_file_handle *stream) {
        return 0;
    }

    int64_t VirtualFileSystemContext::SeekImpl(struct retro_vfs_file_handle *stream, int64_t offset, int seek_position) {
        return 0;
    }

    int64_t VirtualFileSystemContext::ReadImpl(struct retro_vfs_file_handle *stream, void *s, uint64_t len) {
        return 0;
    }

    int64_t VirtualFileSystemContext::Write(struct retro_vfs_file_handle *stream, const void *s, uint64_t len) {
        return 0;
    }

    int VirtualFileSystemContext::FlushImpl(struct retro_vfs_file_handle *stream) {
        return 0;
    }

    int VirtualFileSystemContext::RemoveImpl(const char *path) {
        return 0;
    }

    int VirtualFileSystemContext::RenameImpl(const char *old_path, const char *new_path) {
        return 0;
    }

    int VirtualFileSystemContext::StateImpl(const char *path, int32_t *size) {
        return 0;
    }

    int VirtualFileSystemContext::MkdirImpl(const char *dir) {
        return 0;
    }

    struct retro_vfs_dir_handle *VirtualFileSystemContext::OpenDirImpl(const char *dir, bool include_hidden) {
        return nullptr;
    }

    bool VirtualFileSystemContext::ReadDirImpl(struct retro_vfs_dir_handle *dirstream) {
        return false;
    }

    const char *VirtualFileSystemContext::DirEntGetNameImpl(struct retro_vfs_dir_handle *dirstream) {
        return nullptr;
    }

    bool VirtualFileSystemContext::DirEntIsDirImpl(struct retro_vfs_dir_handle *dirstream) {
        return false;
    }

    int VirtualFileSystemContext::CloseDirImpl(struct retro_vfs_dir_handle *dirstream) {
        return 0;
    }
}
