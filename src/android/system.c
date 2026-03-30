/*
Copyright (C) 2026
*/

#include "shared/shared.h"
#include "common/cmd.h"
#include "common/common.h"
#include "common/cvar.h"
#include "common/files.h"
#if USE_REF
#include "client/video.h"
#endif
#include "system/system.h"

#include <android/log.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_messagebox.h>
#include <SDL2/SDL_system.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef DTTOIF
#define DTTOIF(type) \
    ((type) == DT_DIR ? S_IFDIR : \
    ((type) == DT_REG ? S_IFREG : \
    ((type) == DT_LNK ? S_IFLNK : 0)))
#endif

extern SDL_Window *get_sdl_window(void);

static char baseDirectory[PATH_MAX];
static bool terminate;
static bool flush_logs;

cvar_t  *sys_basedir;
cvar_t  *sys_libdir;
cvar_t  *sys_homedir;
cvar_t  *sys_forcegamelib;

static void sys_log_print(int priority, const char *text)
{
    __android_log_print(priority, PRODUCT, "%s", text);
}

static void usr1_handler(int signum)
{
    (void)signum;
    flush_logs = true;
}

static void term_handler(int signum)
{
    sys_log_print(ANDROID_LOG_WARN, strsignal(signum));
    terminate = true;
}

static void kill_handler(int signum)
{
#if USE_REF
    if (vid.fatal_shutdown) {
        vid.fatal_shutdown();
    }
#endif

    sys_log_print(ANDROID_LOG_FATAL, strsignal(signum));
    exit(EXIT_FAILURE);
}

static void ensure_dir(const char *path)
{
    if (os_mkdir(path) == -1 && errno != EEXIST) {
        Sys_Error("Failed to create %s: %s", path, strerror(errno));
    }
}

static const char *pick_storage_root(void)
{
    const char *path = SDL_AndroidGetExternalStoragePath();

    if (!path || !path[0]) {
        path = SDL_AndroidGetInternalStoragePath();
    }

    return path;
}

static void prepare_storage_layout(const char *storage_root)
{
    char path[MAX_OSPATH];

    if (Q_concat(baseDirectory, sizeof(baseDirectory), storage_root, "/q2rtx") >= sizeof(baseDirectory)) {
        Sys_Error("Android storage path too long");
    }

    ensure_dir(baseDirectory);

    if (Q_concat(path, sizeof(path), baseDirectory, "/baseq2") < sizeof(path)) {
        ensure_dir(path);
    }
    if (Q_concat(path, sizeof(path), baseDirectory, "/screenshots") < sizeof(path)) {
        ensure_dir(path);
    }
    if (Q_concat(path, sizeof(path), baseDirectory, "/save") < sizeof(path)) {
        ensure_dir(path);
    }
}

void Sys_DebugBreak(void)
{
    raise(SIGTRAP);
}

unsigned Sys_Milliseconds(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;
}

void Sys_Quit(void)
{
#if USE_SDL
    SDL_Quit();
#endif
    exit(EXIT_SUCCESS);
}

void Sys_AddDefaultConfig(void)
{
}

void Sys_Sleep(int msec)
{
    struct timespec req = {
        .tv_sec = msec / 1000,
        .tv_nsec = (msec % 1000) * 1000000
    };

    nanosleep(&req, NULL);
}

const char *Sys_ErrorString(int err)
{
    return strerror(err);
}

#if USE_AC_CLIENT
bool Sys_GetAntiCheatAPI(void)
{
    Sys_Sleep(1500);
    return false;
}
#endif

bool Sys_SetNonBlock(int fd, bool nb)
{
    int ret = fcntl(fd, F_GETFL, 0);

    if (ret == -1) {
        return false;
    }
    if ((bool)(ret & O_NONBLOCK) == nb) {
        return true;
    }

    return fcntl(fd, F_SETFL, ret ^ O_NONBLOCK) == 0;
}

bool Sys_IsDir(const char *path)
{
    struct stat sb;

    if (stat(path, &sb) == -1) {
        return false;
    }

    return S_ISDIR(sb.st_mode);
}

bool Sys_IsFile(const char *path)
{
    struct stat sb;

    if (stat(path, &sb) == -1) {
        return false;
    }

    return S_ISREG(sb.st_mode);
}

void Sys_Init(void)
{
    const char *storage_root;
    cvar_t *sys_parachute;

    signal(SIGTERM, term_handler);
    signal(SIGINT, term_handler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, term_handler);
    signal(SIGUSR1, usr1_handler);

    storage_root = pick_storage_root();
    if (!storage_root || !storage_root[0]) {
        Sys_Error("Android storage root unavailable");
    }

    prepare_storage_layout(storage_root);

    sys_basedir = Cvar_Get("basedir", baseDirectory, CVAR_NOSET);
    sys_homedir = Cvar_Get("homedir", baseDirectory, CVAR_NOSET);
    sys_libdir = Cvar_Get("libdir", baseDirectory, CVAR_NOSET);
    sys_forcegamelib = Cvar_Get("sys_forcegamelib", "", CVAR_NOSET);
    sys_parachute = Cvar_Get("sys_parachute", "1", CVAR_NOSET);

    if (sys_parachute->integer) {
        signal(SIGSEGV, kill_handler);
        signal(SIGILL, kill_handler);
        signal(SIGFPE, kill_handler);
        signal(SIGTRAP, kill_handler);
    }

    sys_log_print(ANDROID_LOG_INFO, va("Using Android data root %s", baseDirectory));
}

void Sys_Error(const char *error, ...)
{
    va_list argptr;
    char text[MAXERRORMSG];

    va_start(argptr, error);
    Q_vsnprintf(text, sizeof(text), error, argptr);
    va_end(argptr);

#if USE_CLIENT
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PRODUCT " Fatal Error", text, get_sdl_window());
#endif

#if USE_REF
    if (vid.fatal_shutdown) {
        vid.fatal_shutdown();
    }
#endif

    sys_log_print(ANDROID_LOG_FATAL, text);
    exit(EXIT_FAILURE);
}

void Sys_FreeLibrary(void *handle)
{
    if (handle && dlclose(handle)) {
        Com_Error(ERR_FATAL, "dlclose failed on %p: %s", handle, dlerror());
    }
}

static void *sys_load_library_module(const char *name, const char *sym, void **handle)
{
    void *module;
    void *entry = NULL;

    dlerror();
    module = dlopen(name, RTLD_NOW);
    if (!module) {
        Com_SetLastError(dlerror());
        return NULL;
    }

    if (sym) {
        dlerror();
        entry = dlsym(module, sym);
        if (!entry) {
            Com_SetLastError(dlerror());
            dlclose(module);
            return NULL;
        }
    }

    *handle = module;
    return entry;
}

void *Sys_LoadLibrary(const char *path, const char *sym, void **handle)
{
    char module_name[MAX_QPATH];
    const char *base_name;
    void *entry;

    *handle = NULL;

    entry = sys_load_library_module(path, sym, handle);
    if (entry) {
        return entry;
    }

    base_name = COM_SkipPath(path);
    if (base_name && *base_name) {
        Q_strlcpy(module_name, base_name, sizeof(module_name));
        entry = sys_load_library_module(module_name, sym, handle);
        if (entry) {
            return entry;
        }

        if (strncmp(module_name, "lib", 3) != 0) {
            Q_snprintf(module_name, sizeof(module_name), "lib%s", base_name);
            entry = sys_load_library_module(module_name, sym, handle);
            if (entry) {
                return entry;
            }
        }
    }

    return NULL;
}

void *Sys_GetProcAddress(void *handle, const char *sym)
{
    void *entry;

    dlerror();
    entry = dlsym(handle, sym);
    if (!entry) {
        Com_SetLastError(dlerror());
    }

    return entry;
}

void Sys_ListFiles_r(listfiles_t *list, const char *path, int depth)
{
    struct dirent *ent;
    DIR *dir;
    struct stat st;
    char fullpath[MAX_OSPATH];
    char *name;
    void *info;

    dir = opendir(path);
    if (!dir) {
        return;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }

        if (Q_concat(fullpath, sizeof(fullpath), path, "/", ent->d_name) >= sizeof(fullpath)) {
            continue;
        }

        st.st_mode = 0;
#ifdef _DIRENT_HAVE_D_TYPE
        if (!(list->flags & FS_SEARCH_EXTRAINFO)
            && ent->d_type != DT_UNKNOWN
            && ent->d_type != DT_LNK) {
            st.st_mode = DTTOIF(ent->d_type);
        }
#endif
        if (st.st_mode == 0 && stat(fullpath, &st) == -1) {
            continue;
        }

        if ((list->flags & (FS_SEARCH_BYFILTER | FS_SEARCH_RECURSIVE))
            && S_ISDIR(st.st_mode) && depth < MAX_LISTED_DEPTH) {
            Sys_ListFiles_r(list, fullpath, depth + 1);
            if (list->count >= MAX_LISTED_FILES) {
                break;
            }
        }

        if (list->flags & FS_SEARCH_DIRSONLY) {
            if (!S_ISDIR(st.st_mode)) {
                continue;
            }
        } else if (!S_ISREG(st.st_mode)) {
            continue;
        }

        if (list->filter) {
            if (list->flags & FS_SEARCH_BYFILTER) {
                if (!FS_WildCmp(list->filter, fullpath + list->baselen)) {
                    continue;
                }
            } else if (!FS_ExtCmp(list->filter, ent->d_name)) {
                continue;
            }
        }

        name = (list->flags & FS_SEARCH_SAVEPATH) ? fullpath + list->baselen : ent->d_name;
        if (list->flags & FS_SEARCH_STRIPEXT) {
            *COM_FileExtension(name) = 0;
            if (!*name) {
                continue;
            }
        }

        if (list->flags & FS_SEARCH_EXTRAINFO) {
            info = FS_CopyInfo(name, st.st_size, st.st_ctime, st.st_mtime);
        } else {
            info = FS_CopyString(name);
        }

        list->files = FS_ReallocList(list->files, list->count + 1);
        list->files[list->count++] = info;
        if (list->count >= MAX_LISTED_FILES) {
            break;
        }
    }

    closedir(dir);
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
            sys_log_print(ANDROID_LOG_INFO, com_version_string);
            return EXIT_SUCCESS;
        }
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            sys_log_print(ANDROID_LOG_INFO, "Use the Android launcher activity to start Q2RTX.");
            return EXIT_SUCCESS;
        }
    }

    Qcommon_Init(argc, argv);
    while (!terminate) {
        if (flush_logs) {
            Com_FlushLogs();
            flush_logs = false;
        }
        Qcommon_Frame();
    }

    Com_Quit(NULL, ERR_DISCONNECT);
    return EXIT_FAILURE;
}
