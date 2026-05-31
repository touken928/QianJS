#include "native/fs/fs_stat_js.h"

#include <qjs/object.h>

#include <cstdint>

#include <sys/stat.h>

static int64_t timespec_to_ms(const uv_timespec_t& t) {
    return static_cast<int64_t>(t.tv_sec) * 1000 + static_cast<int64_t>(t.tv_nsec) / 1000000;
}

qjs::Value fs_stat_to_value(qjs::Engine& engine, const uv_stat_t& st) {
#ifndef S_IFMT
#define S_IFMT 0170000
#endif
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif
#ifndef S_IFIFO
#define S_IFIFO 0010000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

    const uint64_t mode = st.st_mode;
    const bool is_file = (mode & S_IFMT) == S_IFREG;
    const bool is_dir = (mode & S_IFMT) == S_IFDIR;
    const bool is_symlink = (mode & S_IFMT) == S_IFLNK;
    const bool is_chr = (mode & S_IFMT) == S_IFCHR;
    const bool is_blk = (mode & S_IFMT) == S_IFBLK;
    const bool is_fifo = (mode & S_IFMT) == S_IFIFO;
    const bool is_sock = (mode & S_IFMT) == S_IFSOCK;

    return engine.object()
        .setInt64("dev", static_cast<int64_t>(st.st_dev))
        .setInt64("ino", static_cast<int64_t>(st.st_ino))
        .setInt64("mode", static_cast<int64_t>(st.st_mode))
        .setInt64("nlink", static_cast<int64_t>(st.st_nlink))
        .setInt64("uid", static_cast<int64_t>(st.st_uid))
        .setInt64("gid", static_cast<int64_t>(st.st_gid))
        .setInt64("rdev", static_cast<int64_t>(st.st_rdev))
        .setInt64("size", static_cast<int64_t>(st.st_size))
        .setInt64("blksize", static_cast<int64_t>(st.st_blksize))
        .setInt64("blocks", static_cast<int64_t>(st.st_blocks))
        .setDouble("atimeMs", static_cast<double>(timespec_to_ms(st.st_atim)))
        .setDouble("mtimeMs", static_cast<double>(timespec_to_ms(st.st_mtim)))
        .setDouble("ctimeMs", static_cast<double>(timespec_to_ms(st.st_ctim)))
        .setDouble("birthtimeMs", static_cast<double>(timespec_to_ms(st.st_birthtim)))
        .setBool("isFile", is_file)
        .setBool("isDirectory", is_dir)
        .setBool("isSymbolicLink", is_symlink)
        .setBool("isCharacterDevice", is_chr)
        .setBool("isBlockDevice", is_blk)
        .setBool("isFIFO", is_fifo)
        .setBool("isSocket", is_sock)
        .build();
}
