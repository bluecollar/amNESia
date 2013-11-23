// amNESia.h - This file should be included once EVERYWHERE
//
// Contains general purpose defines, includes, typedefs and extern refs for all modules
//
// Christopher Bond and Jason Pike, 12/2011
#pragma once


// Global Build Flags
#define BE_SAFE      (1)      // ASSERT's are called; .at() instead of []; etc.
//#define GO_APE_SHIT  (1)    // Just like the name says. Rock some performance

#ifdef BE_SAFE                // Don't try combining being safe with being wild
  #undef GO_APE_SHIT
#endif


// Compiler-specific directives
#ifdef _MSC_VER                  // Visual Studio
  #define WIN32_LEAN_AND_MEAN 
  #pragma warning(disable:4995)  // "name was marked as #pragma deprecated"
#endif


// Platform-specific includes
#if defined(_WIN32)                     // Windows
  #include "StdAfx.h"
  #include "Resource.h"
#elif defined(__GNUC__)


#endif


// Common System Includes
#include <vector>
#include <cassert>


// Common Project Includes
#include "fine_timer.h"
#include "logger.h"


// Namespace directives
using std::vector;


// Typedefs
typedef unsigned char  byte;
typedef unsigned short address;
typedef unsigned long  u32;
typedef vector<byte>   buffer_t;



// Globals confined within our namespace
namespace amnesia 
{
	extern double g_fps;
	extern double g_glps;
	extern FineTimer g_globalClock;
}


// Master clock and sub clocks
// * http://wiki.nesdev.com/w/index.php/Clock_rate
#define MASTER_CLOCK_FREQUENCY   (21477272) // 21.477272 MHz ± 40 Hz (236.25 MHz / 11 by definition)
#define PPU_CLOCK_FREQUENCY      (5369318)  // (MASTER_CLOCK_FREQUENCY / 4), 1 dot per ppu cycle
#define CPU_CLOCK_FREQUENCY      (1789772)  // (MASTER_CLOCK_FREQUENCY / 12), 3 ppu cycles per cpu cycle
#define PPU_CYCLES_PER_CPU_CYCLE (3)        // (PPU_CLOCK_FREQUENCY / CPU_CLOCK_FREQUENCY)


// Common Macros
#define DELETE_SAFE(x)  { if (x) { delete x; x=NULL; } }

#ifdef BE_SAFE
  #define ASSERT(x)     assert((x));
#else
  #define ASSERT(x)     ((0))
#endif 



/// sandbox //////////////////
// OS Defines:
// _WIN32    - win32 and win64
// _WIN64    - win64 only
// Macintosh - Mac OS 9
// __APPLE__ & __MACH__ - Mac OS X
// __linux   - Linux
// __CYGWIN__ - Cygwin environment
//
// 