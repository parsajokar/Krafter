#pragma once

// STDCALL: the calling convention OpenGL uses for callbacks it invokes (such as
// the debug message callback). Required on 32-bit Windows/x86; a no-op on the
// platforms and ABIs where the default convention already matches.
#if defined(_MSC_VER)

#define STDCALL __stdcall

#elif defined(__GNUC__)

#if defined(__i386__)
#define STDCALL __attribute__((stdcall))
#else
#define STDCALL
#endif

#else

#define STDCALL

#endif
