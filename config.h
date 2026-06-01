#ifndef CONFIG_H
#define CONFIG_H

#define WEBM_WINDOW_CLASS L"OXOP_AnimatedWallpapersReimagined_Class"
#define MUTEX_NAME L"Global\\OXOP_AnimatedWallpapersReimagined_Mutex"
#define DEFAULT_VIDEO_NAME L"video.mp4"

#if defined(_MSC_VER) && !defined(__clang__)
    #define COMPILER_NAME "Microsoft Visual C++"
#elif defined(__clang__)
    #define COMPILER_NAME "Clang/LLVM"
#elif defined(__GNUC__)
    #define COMPILER_NAME "GCC (MinGW)"
#else
    #define COMPILER_NAME "Unknown Compiler"
#endif

#endif