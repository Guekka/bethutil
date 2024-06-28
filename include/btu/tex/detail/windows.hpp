/// Wrapper for Windows.h header
/// Removes as much as possible from the global namespace

// NOLINTBEGIN(CppCoreGuidelines-macro-usage)
// NOLINTBEGIN(clang-diagnostic-unused-macros)

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
// We cannot define NOMSG, we need it
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

// NOLINTEND(CppCoreGuidelines-macro-usage)
// NOLINTEND(clang-diagnostic-unused-macros)

#pragma clang diagnostic pop

#ifdef _WIN32
#include <Windows.h>
#endif

/// Credits to https://github.com/Karandra/KxFramework/blob/master/kxf/System/UndefWindows.h

// No include guard, we need to include this file multiple times
#include <utility>

// Undefine Unicode wrapper macros
#undef min
#undef max
#undef GetObject
#undef SetCurrentDirectory
#undef CreateFile
#undef CopyFile
#undef MoveFile
#undef DeleteFile
#undef GetBinaryType
#undef MessageBox
#undef GetFreeSpace
#undef PlaySound
#undef RegisterClass
#undef CreateEvent
#undef GetFirstChild
#undef GetNextSibling
#undef GetPrevSibling
#undef GetWindowStyle
#undef GetShortPathName
#undef GetLongPathName
#undef GetFullPathName
#undef GetFileAttributes
#undef EnumResourceTypes
#undef LoadImage
#undef UpdateResource
#undef BeginUpdateResource
#undef EndUpdateResource
#undef EnumResourceLanguages
#undef FormatMessage
#undef GetCommandLine
#undef CreateProcess
#undef GetUserName
#undef FindFirstFile
#undef FindNextFile
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable
#undef ExitWindows
#undef GetCompressedFileSize
#undef GetTempPath
#undef CreateDirectory
#undef CompareString
#undef EnumDisplayDevices
#undef ExpandEnvironmentStrings
#undef EnumResourceNames
#undef GetMessage
#undef OpenService
#undef CreateService
#undef WriteConsole
#undef CreateProcess
#undef CreateDirectory
#undef RemoveDirectory
#undef GetCurrentDirectory
#undef SetCurrentDirectory
#undef SetPort
#undef CreateFont
#undef SendMessage
#undef SendMessageTimeout
#undef SendMessageCallback
#undef SendNotifyMessage
#undef PostMessage
#undef PostThreadMessage
#undef InsertMenu

#ifdef ZeroMemory
#undef ZeroMemory
inline void *ZeroMemory(void *ptr, size_t size) noexcept
{
    return std::memset(ptr, 0, size);
}
#endif

#ifdef CopyMemory
#undef CopyMemory
inline void *CopyMemory(void *dst, const void *src, size_t size) noexcept
{
    return std::memcpy(dst, src, size);
}
#endif

#ifdef MoveMemory
#undef MoveMemory
inline void *MoveMemory(void *dst, const void *src, size_t size) noexcept
{
    return std::memmove(dst, src, size);
}
#endif

#ifdef FillMemory
#undef FillMemory
inline void *FillMemory(void *dst, size_t size, int fill) noexcept
{
    return std::memset(dst, fill, size);
}
#endif

#ifdef EqualMemory
#undef EqualMemory
inline bool EqualMemory(const void *left, const void *right, size_t size) noexcept
{
    return std::memcmp(left, right, size) == 0;
}
#endif

#ifdef SecureZeroMemory
#undef SecureZeroMemory
inline void *SecureZeroMemory(void *ptr, size_t size) noexcept
{
    return ::RtlSecureZeroMemory(ptr, size);
}
#endif

#undef __pre
