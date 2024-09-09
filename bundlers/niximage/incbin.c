#define _GNU_SOURCE
#include "stdlib.h"
#include <errno.h>
#include <libgen.h>
#include <linux/memfd.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define __FILENAME__                                                           \
    (strchr(__FILE__, '\\')                                                    \
         ? ((strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1             \
                                     : __FILE__))                              \
         : ((strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)))

#define CHECK_ERRNO(compare, var, ret)                                         \
    {                                                                          \
        if (compare) {                                                         \
            var = ret;                                                         \
            fprintf(stderr, "[%s:%d](%s): %s\n", __FILENAME__, __LINE__,       \
                    #compare, strerror(errno));                                \
        }                                                                      \
    }

#define CHECK(compare, var, ret)                                               \
    {                                                                          \
        if (compare) {                                                         \
            var = ret;                                                         \
            fprintf(stderr, "[%s:%d](%s)\n", __FILENAME__, __LINE__,           \
                    #compare);                                                 \
        }                                                                      \
    }

__asm__(".section .rodata\n"
        ".global bwrap_start\n"
        "bwrap_start:\n"
        ".incbin \"bwrap\"\n"
        ".global bwrap_end\n"
        "bwrap_end:\n");
__asm__(".section .rodata\n"
        ".global fuse_start\n"
        "fuse_start:\n"
        ".incbin \"fuse\"\n"
        ".global fuse_end\n"
        "fuse_end:\n");

extern const char bwrap_start[];
extern const char bwrap_end[];
extern const char fuse_start[];
extern const char fuse_end[];
extern size_t appimage_get_elf_size(char *exe_path);

static char const *g_mountpoint_fmt = "/run/user/%d/%s";
static char const *g_mountpoint_name = ((char const *)"@wrappedDrv@") + 11;
static char *g_mountpoint = NULL;
static char *g_mounted_store = NULL;

int incbin_main(char *argv[], char const *buf, size_t len, bool silent,
                bool wait) {
    int ret = 0;
    int fd = -1;
    char *p = NULL;
    int pid = -1;

    do {
        fd = memfd_create("", MFD_CLOEXEC);
        CHECK_ERRNO(fd < 0, ret, -1);
        CHECK_ERRNO(write(fd, buf, len) != len, ret, -1);

        pid = fork();
        CHECK_ERRNO(pid < 0, ret, -1);
        if (pid == 0) {
            CHECK_ERRNO(asprintf(&p, "/proc/self/fd/%i", fd) < 0, ret, -1);
            if (silent) {
                close(1);
                close(2);
            }
            execv(p, argv);
        } else {
            if (wait)
                waitpid(pid, &ret, 0);
            else
                ret = pid;
        }
    } while (0);
    if (fd != -1) {
        close(fd);
    }
    return ret;
}

int mount_nix_store(char *exe_path) {
    int ret = 0;
    size_t offset = appimage_get_elf_size(exe_path);
    char *offset_arg = NULL;
    asprintf(&offset_arg, "offset=%zu", offset);
    char *argv[] = {"fuse", "-o", offset_arg, exe_path, g_mountpoint, NULL};

    struct stat st = {0};
    do {
        if (stat(g_mountpoint, &st) < 0) {
            mkdir(g_mountpoint, 0700);
        }

        if (stat(g_mounted_store, &st) < 0) {
            ret = incbin_main(argv, fuse_start, fuse_end - fuse_start, false,
                              true);
        }
    } while (0);
    free(offset_arg);
    return ret;
}

int run_exe_with_bwrap(char *exe_name, int argc, char *pargv[]) {
    int ret = 0;
    char *new_env = NULL;
    asprintf(&new_env, "@wrappedDrv@/bin/:%s", getenv("PATH"));
    char *argv[] = {"bwrap",
                    "--bind",
                    "/",
                    "/",
                    "--ro-bind",
                    g_mounted_store,
                    "/nix",
                    "--dev-bind",
                    "/dev/null",
                    "/dev/null",
                    "--dev-bind",
                    "/dev/zero",
                    "/dev/zero",
                    "--dev-bind",
                    "/dev/random",
                    "/dev/random",
                    "--dev-bind",
                    "/dev/urandom",
                    "/dev/urandom",
                    "--dev-bind",
                    "/dev/tty",
                    "/dev/tty",
                    "--unsetenv",
                    "PATH",
                    "--setenv",
                    "PATH",
                    new_env,
                    exe_name,
                    NULL};
    char **new_argv = malloc(argc * sizeof(void *) + sizeof(argv));

    memcpy(new_argv, argv, sizeof(argv));

    for (int i = 0; i < argc; i++) {
        new_argv[sizeof(argv) / sizeof(void *) + i - 1] = pargv[i];
    }
    new_argv[sizeof(argv) / sizeof(void *) + argc - 1] = NULL;

    ret = incbin_main(new_argv, bwrap_start, bwrap_end - bwrap_start, false,
                      true);
    free(new_env);
    free(new_argv);
    return ret;
}

int main(int argc, char *argv[]) {
    char *exe_path = NULL;
    char *exe_name = NULL;
    int ret = 0;
    do {
        exe_path = realpath("/proc/self/exe", NULL);
        CHECK_ERRNO(exe_path == NULL, ret, -1);
        exe_name = basename(argv[0]);

        CHECK_ERRNO(asprintf(&g_mountpoint, g_mountpoint_fmt, getuid(),
                             g_mountpoint_name) < 0,
                    ret, -1);
        CHECK_ERRNO(asprintf(&g_mounted_store, "%s/nix", g_mountpoint) < 0, ret,
                    -1);

        mount_nix_store(exe_path);

        if (strcmp(exe_name, "@name@") == 0) {
            exe_name = "bash";
        }

        run_exe_with_bwrap(exe_name, argc - 1, argv + 1);
    } while (0);

    free(exe_path);
    free(g_mountpoint);
    free(g_mounted_store);
    return ret;
}
