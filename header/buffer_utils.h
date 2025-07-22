// ----------------------------------------------------------------------------
/**
 * @file        buffer_utils.h
 * @author      Fei Li (LIF@xsyu.edu.cn)
 * @date        8 Aug 2012
 * @brief       Header file for buffer_utils.c
 * @note        Please refer to the .c file for a detailed description.
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
#ifndef BUFFER_UTILS_H_
#define BUFFER_UTILS_H_

/// Enumerated values for buffer radix.
typedef enum
{
    BUFFER_RADIX_HEX,       ///< Buffer values are in hexadecimal.
    BUFFER_RADIX_DEC        ///< Buffer values are in decimal.
} buffer_radix_t;

char_t*     BUFFER_UTILS_4BitsToHex(char_t * const p_buffer,
                                    const uint8_t num);

char_t*     BUFFER_UTILS_8BitsToHex(char_t * const p_buffer,
                                    const uint8_t num);

char_t*     BUFFER_UTILS_16BitsToHex(char_t * const p_buffer,
                                     const uint16_t num);

char_t*     BUFFER_UTILS_32BitsToHex(char_t * const p_buffer,
                                     const uint32_t num);

uint16_t    BUFFER_UTILS_BufferToUpperCase(char_t * const p_buffer);

bool_t      BUFFER_UTILS_StringToUint16(const char_t * const p_buffer,
                                        uint16_t * const p_result_uint16,
                                        const buffer_radix_t radix);

char_t*     BUFFER_UTILS_Uint16ToDecimal(char_t * const p_buffer,
                                         uint16_t num,
                                         const bool_t b_IsZeroPaddingRequired);

char_t*     BUFFER_UTILS_Uint16ToFWDecimal(char_t * const p_buffer,
                                           const uint16_t num,
                                           const uint16_t NumberOfDigits);

char_t*     BUFFER_UTILS_Uint32ToDecimal(char_t * const p_buffer,
                                         uint32_t num,
                                         const bool_t b_IsZeroPaddingRequired);

uint16_t    BUFFER_UTILS_BackspaceRemoval(char_t * const p_buffer);

char_t*     BUFFER_UTILS_Float32ToDecimal(char_t * const p_buffer,
                                          const float32_t number_float32,
                                          const uint16_t precision);

char_t*     BUFFER_UTILS_Float32ToScientif(char_t * const p_buffer,
                                           const float32_t number_float32,
                                           const uint16_t precision);

bool_t      BUFFER_UTILS_StringToFloat32(const char_t * const p_buffer,
                                         float32_t * const p_result_float32);

char_t*     BUFFER_UTILS_nFloat32ToUint16(char_t * const p_buffer,
                                          float32_t number_float32);

char_t*     BUFFER_UTILS_DataValueBufferPut(char_t * const p_buffer,
                                            const void * const p_variable,
                                            const data_types_t data_type);

float32_t   BUFFER_UTILS_8bitBufToFloat32(const uint8_t * const p_Buffer);

float32_t   BUFFER_UTILS_16bitBufToFloat32(const uint16_t * const p_Buffer);

uint16_t    BUFFER_UTILS_8bitBufToUint16(const uint8_t * const p_Buffer);

uint32_t    BUFFER_UTILS_8bitBufToUint32(const uint8_t * const p_Buffer);

uint8_t*    BUFFER_UTILS_Uint16To8bitBuf(uint8_t * const p_Buffer,
                                         const uint16_t Value);

uint8_t*    BUFFER_UTILS_Uint32To8bitBuf(uint8_t * const p_Buffer,
                                         const uint32_t Value);

uint8_t*    BUFFER_UTILS_Float32To8bitBuf(uint8_t * const p_Buffer,
                                          const float32_t Value);

uint16_t*   BUFFER_UTILS_Float32To16bitBuf(uint16_t * const p_Buffer,
                                           const float32_t Value);

#endif /* BUFFER_UTILS_H_ */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

