#pragma once

// Until std::unreachable is not available, define our own as a macro.
//   https://en.cppreference.com/w/cpp/utility/unreachable

// std::unreachable and our macro say the specified path of execution
//   is never taken.
// Also, compiler may trap execution in debug build and optimize away
//   the branch of execution in release builds.

// Testing the compiler with macros.
//   https://stackoverflow.com/a/28166605

#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE() __builtin_unreachable();
#elif defined(_MSC_VER)
#define UNREACHABLE() __assume(false);
#endif
