#ifndef COMPILER_CONFIG_H
#define COMPILER_CONFIG_H

#if defined(_WIN32)
#define IS_WINDOWS
#elif defined(__APPLE__)
#define IS_OSX
#else
#define IS_LINUX
#endif

#cmakedefine IS_GCC
#cmakedefine IS_CC
#cmakedefine IS_APPLE_CC

#cmakedefine SUPPORT_GUROBI

#endif // COMPILER_CONFIG_H