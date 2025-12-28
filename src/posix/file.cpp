#ifdef _WIN32
#error This code unit is NOT for Windows !
#endif

#include "resources/file.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

#ifndef WIN32
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "log.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"

namespace ufps
{
    File::File(const std::filesystem::path &path, CreationMode mode)
        : _handle{-1, ::close},
          _map_view{nullptr, [this](void *view)
                    {
                        if (!_mapped)
                        {
                            return;
                        }
                        auto r = ::munmap(view, this->_filesize);
                        if (r != 0)
                        {
                            auto e = errno;
                            log::error("{}", e);
                            ensure(false, "failed to unmap view");
                        }
                    }},
          _filesize{0ul}, _mode(mode), _mapped(false)
    {
        if (mode == CreationMode::OPEN)
        {
            _handle.reset(::open(path.string().c_str(), O_RDONLY | O_LARGEFILE));
        }
        else
        {
            _handle.reset(::open(path.string().c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP));
        }
        if (_handle == -1)
        {
            auto e = errno;
            log::error("{}", e);
        }

        ensure(_handle != -1, "failed to open file");

        get_file_size();

        if (_filesize == 0)
        {
            // file is new
            auto res = ::truncate64(path.string().c_str(), 4096u);
            ensure(res == 0, "failed to extend file size");

            get_file_size();
        }

        ensure(_filesize > 0, "file has zero length");

        _map_view.reset(::mmap64(NULL, _filesize, _mode == CreationMode::OPEN ? PROT_READ : PROT_READ | PROT_WRITE, MAP_SHARED, _handle, 0ul));
        if (_map_view.get() == MAP_FAILED)
        {
            auto e = errno;
            log::error("{}", e);
        }
        ensure(_map_view.get() != MAP_FAILED, "failed to map file");

        _mapped = true;

        // int linuxHint = 0;
        // switch (_hint)
        // {
        // case Normal:
        //     linuxHint = MADV_NORMAL;
        //     break;
        // case SequentialScan:
        //     linuxHint = MADV_SEQUENTIAL;
        //     break;
        // case RandomAccess:
        //     linuxHint = MADV_RANDOM;
        //     break;
        // default:
        //     break;
        // }
        // // assume that file will be accessed soon
        // // linuxHint |= MADV_WILLNEED;
        // // assume that file will be large
        // // linuxHint |= MADV_HUGEPAGE;

        // ::madvise(_map_view, 0, linuxHint);
    }

    auto File::as_string() const -> std::string_view
    {
        return {reinterpret_cast<const char *>(_map_view.get()), _filesize};
    }

    auto File::as_bytes() const -> std::span<const std::byte>
    {
        return {reinterpret_cast<const std::byte *>(_map_view.get()), _filesize};
    }

    auto File::get_file_size() -> void
    {
        struct stat64 statInfo;

        auto result = ::fstat64(_handle, &statInfo);
        ensure(result >= 0, "failed to get file size");

        _filesize = statInfo.st_size;
    }

    auto File::resize_and_remap(std::size_t new_size) -> void
    {
        if (new_size <= _filesize)
        {
            return;
        }

        auto res = ::munmap(_map_view, _filesize);
        if (res != 0)
        {
            auto e = errno;
            log::error("{}", e);
            ensure(res == 0, "failed to unmap view");
        }

        _mapped = false;

        res = ::ftruncate64(_handle, new_size);
        if (res != 0)
        {
            auto e = errno;
            log::error("{}", e);
            ensure(res == 0, "failed to extend file size");
        }

        get_file_size();

        _map_view.reset(::mmap64(NULL, _filesize, _mode == CreationMode::OPEN ? PROT_READ : PROT_READ | PROT_WRITE, MAP_SHARED, _handle, 0ul));
        if (_map_view.get() == MAP_FAILED)
        {
            auto e = errno;
            log::error("{}", e);
        }
        ensure(_map_view.get() != MAP_FAILED, "failed to map file");

        _mapped = true;
    }

}