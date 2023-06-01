#ifndef __UTILITIES_CAMERAUNIT_H__
#define __UTILITIES_CAMERAUNIT_H__

#include <sys/stat.h>
#include <sys/types.h>
#if defined(OS_WIN)
#include <windows.h>
#else
#include <dirent.h> // for *Nix directory access
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static bool file_exists(const char *file)
{
    if (file == NULL)
    {
        return false;
    }
#if defined(OS_WIN)
#if defined(WIN_API)
    // if you want the WinAPI, versus CRT
    if (strnlen(file, MAX_PATH + 1) > MAX_PATH)
    {
        // ... throw error here or ...
        return false;
    }
    DWORD res = GetFileAttributesA(file);
    return (res != INVALID_FILE_ATTRIBUTES &&
            !(res & FILE_ATTRIBUTE_DIRECTORY));
#else
    // Use Win CRT
    struct stat fi;
    if (_stat(file, &fi) == 0)
    {
#if defined(S_ISSOCK)
        // sockets come back as a 'file' on some systems
        // so make sure it's not a socket or directory
        // (in other words, make sure it's an actual file)
        return !(S_ISDIR(fi.st_mode)) &&
               !(S_ISSOCK(fi.st_mode));
#else
        return !(S_ISDIR(fi.st_mode));
#endif
    }
    return false;
#endif
#else
    struct stat fi;
    if (stat(file, &fi) == 0)
    {
#if defined(S_ISSOCK)
        return !(S_ISDIR(fi.st_mode)) &&
               !(S_ISSOCK(fi.st_mode));
#else
        return !(S_ISDIR(fi.st_mode));
#endif
    }
    return false;
#endif
}

static bool dir_exists(const char *folder)
{
    if (folder == NULL)
    {
        return false;
    }
#if defined(OS_WIN)
#if defined(WIN_API)
    // if you want the WinAPI, versus CRT
    if (strnlen(folder, MAX_PATH + 1) > MAX_PATH)
    {
        // ... throw error here or ...
        return false;
    }
    DWORD res = GetFileAttributesA(folder);
    return (res != INVALID_FILE_ATTRIBUTES &&
            (res & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat fi;
    if (_stat(folder, &fi) == 0)
    {
        return (S_ISDIR(fi.st_mode));
    }
    return false;
#endif
#else
    struct stat fi;
    if (stat(folder, &fi) == 0)
    {
        return (S_ISDIR(fi.st_mode));
    }
    return false;
#endif
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __UTILITIES_CAMERAUNIT_H__