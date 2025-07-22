// ----------------------------------------------------------------------------
/**
 * @file        common_data_types.h
 * @author      Fei Li (LIF@xsyu.edu.cn)
 * @date        16 Feb 2016
 * @brief       Type definitions for common data types for various platforms.
 * @details
 * Different library files on different platforms use different conventions
 * for the typedefs for various data types (Int16, int16, int16_t etc).
 *
 * This file is a way of trying to get everything to play nicely together,
 * without lots of warnings for "redefinition of typedef" cropping up.
 *
 * This implementation also means that it shouldn't be necessary to mock
 * out this file when running unit tests on different platforms.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2016.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef COMMON_DATA_TYPES_H
#define COMMON_DATA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * stdint.h will include typedefs for standard integer types for your platform.
 * These should be POSIX standard types such as uint16_t, int16_t etc.
 * Generally, this doesn't include floating point types (stdINT.h !!).
 */
#include <stdint.h>


/*
 * Extra types for TI's 28xx platform - anything not included in stdint.h
 */
#ifdef __TMS320C28XX__

/*
 * HAVE_BIOS is used within all tools which use Thor,
 * as a mechanism for specifying that DSP/BIOS is used (-d"HAVE_BIOS").
 *
 * USING_BIOS_TYPES is used within tools which want to include std.h but
 * can't use the HAVE_BIOS flag - there are instances in the Thor code where
 * this flag does other stuff which isn't necessarily required.
 *
 * std.h includes tistdtypes.h for the standard TI types,
 * which provides typedefs for Uint32, Uint16, Uint8, Int32, Int16, Int8 and Bool.
 */
#if defined (HAVE_BIOS) || defined (USING_BIOS_TYPES)
#define THOR_TI_STANDARD_TYPES_DEFINED
#include <std.h>
#endif /* HAVE_BIOS */

/*
 * Add POSIX standard data types for other C28XX types which haven't been
 * included as part of tistdtypes.h or stdint.h (note that stdint.h
 * for the 28xx series doesn't have any 8 bit types).
 */
typedef unsigned char   uint8_t;        //lint !e1960 Re-use of C++ identifier
typedef signed char     int8_t;         //lint !e1960 Re-use of C++ identifier
typedef int16_t         bool_t;         // Make boolean type native width of processor.
typedef double          float32_t;      // Note use of double to avoid Lint warnings.
typedef long double     float64_t;      //lint !e586 long is deprecated but have to use it here.


/*
 * Extra types for standard WIN32 platform - anything not included in stdint.h
 */
#elif defined __WIN32__

typedef int32_t     bool_t;         // Make boolean type native width of processor.
typedef float       float32_t;
typedef double      float64_t;


/*
 * Extra types for standard Linux 32 bit platform - anything not included in stdint.h
 */
#elif defined __linux__

typedef int32_t     bool_t;         // Make boolean type native width of processor.
typedef float       float32_t;
typedef double      float64_t;


/*
 * If no platform has been defined then generate an error.
 */
#else
#error "No platform type defined in common_data_types.h"
#endif /* __TMS320C28XX__ / __WIN32__ */


/*
 * If the TI standard types haven't already been defined, then define them here.
 * These should not be used by the programmer (use the POSIX types), but are
 * simply to get any TI code to compile.
 */
#ifndef COMMON_TI_STANDARD_TYPES_DEFINED
#define COMMON_TI_STANDARD_TYPES_DEFINED
typedef uint8_t     Uint8;
typedef uint16_t    Uint16;
typedef uint32_t    Uint32;
typedef int8_t      Int8;
typedef int16_t     Int16;
typedef int32_t     Int32;
typedef bool_t      Bool;
#endif	/* COMMON_TI_STANDARD_TYPES_DEFINED */


/*
 * Other types which may be required for compatibility with TI code, but
 * are not part of tistdtypes.h or the above typedefs.
 * These should not be used by the programmer (use the POSIX types),
 * but, again, are simply to get any TI code to compile.
 */
typedef int8_t      int8;
typedef int16_t     int16;
typedef int32_t     int32;
typedef float32_t   float32;


/*
 * A type for plain characters.
 */
typedef char        char_t;


/**
 * Enumerated type indicating the data type of a variable.
 *
 * This is useful when fetching variables from individual modules
 * using a void*, as it allow us to know what to cast the pointer to
 * before dereferencing.
 */
typedef enum
{
    DATA_TYPE_FLOAT32,      ///< Floating point value, 32 bits.
    DATA_TYPE_FLOAT64,      ///< Floating point value, 64 bits.
    DATA_TYPE_INT8,         ///< Signed data type, 8 bits.
    DATA_TYPE_INT16,        ///< Signed data type, 16 bits.
    DATA_TYPE_INT32,        ///< Signed data type, 32 bits.
    DATA_TYPE_INT64,        ///< Signed data type, 64 bits.
    DATA_TYPE_UINT8,        ///< Unsigned data type, 8 bits.
    DATA_TYPE_UINT16,       ///< Unsigned data type, 16 bits.
    DATA_TYPE_UINT32,       ///< Unsigned data type, 32 bits.
    DATA_TYPE_UINT64,       ///< Unsigned data type, 64 bits.
    DATA_TYPE_CHAR,         ///< Plain character type.
    DATA_TYPE_BOOL,         ///< Boolean data type.
    DATA_TYPE_UNKNOWN       ///< Shouldn't be any of these - for unit testing.
} data_types_t;

/**
 * Structure which can be used with the above, for fetching variables
 * from modules using a const void*.
 */
typedef struct
{
    const void*     p_variable;     ///< Constant void pointer to the variable to fetch.
    data_types_t    data_type;      ///< The data type of the parameter.
} data_fetch_t;


/// #defines for TRUE, FALSE and NULL - note the include guards!
#ifdef __cplusplus

#ifndef true
#define true    (static_cast<bool_t>(1))    //lint !e1923 Macro could become const variable
#endif

#ifndef false
#define false   (static_cast<bool_t>(0))    //lint !e1923 Macro could become const variable
#endif

#else

#ifndef TRUE
#define TRUE    (bool_t)(1)                 //lint !e960 Disallowed definition for macro TRUE.
#endif

#ifndef FALSE
#define FALSE   (bool_t)(0)                 //lint !e960 Disallowed definition for macro FALSE.
#endif

#endif  // __cplusplus for true and false

#ifndef NULL
#define NULL    (void*)0                    //lint !e960 !e9071 Macro reserved by compiler
#endif

#ifdef __cplusplus
}
#endif

#endif // COMMON_DATA_TYPES_H
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
