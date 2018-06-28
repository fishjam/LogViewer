///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ftlTypes.h
/// @brief  Fishjam Template Library Types Header File.
/// @author fujie
/// @version 0.6 
/// @date 03/30/2008
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FTL_TYPE_H
#define FTL_TYPE_H
#pragma once

namespace FTL
{
#if defined(_WIN32) || defined(_WIN64)

# include <WTypes.h>
    typedef unsigned __int8  u_int8_t;
    typedef unsigned __int16 u_int16_t;
    typedef unsigned __int32 u_int32_t;
    typedef unsigned __int64 u_int64_t;
# else  

# include <sys/types.h>
# ifndef __STDC_FORMAT_MACROS
#   define __STDC_FORMAT_MACROS
# endif  /** __STDC_FORMAT_MACROS */

# include <inttypes.h>
# if defined(__SUNPRO_CC) || (defined(__GNUC__) && defined(__sun__))
    typedef uint8_t u_int8_t;
    typedef uint16_t u_int16_t;
    typedef uint32_t u_int32_t;
    typedef uint64_t u_int64_t;
# endif

#endif  /** !_WIN32 */

typedef struct {
  u_int64_t high;
  u_int64_t low;
} u_int128_t;

}


#endif //FTL_TYPE_H