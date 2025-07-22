/**
 * \file
 * Header file for some basic utility functions
 * 
 * @author Scott DiPasquale
 * @date 30 August 2004
 * 
 * (c) Copyright Schlumberger Technology Corp., unpublished work,
 * created 2004.  This computer program includes confidential,
 * proprietary information and is a trade secret of Schlumberger
 * Technology Corp.  All use, disclosure, and/or reproduction is
 * prohibited unless authorized in writing.  All Rights Reserved. 
 * 
 */
 
#ifndef UTILS_H
#define UTILS_H

/**
 * Enum specifying whether the conversion should be little-endian or big-endian
 */
typedef enum
{
    LITTLE_ENDIAN,
    BIG_ENDIAN
} EndianType_t;

typedef enum
{
	WIDTH_16BITS,
	WIDTH_32BITS
} TargetDataWidth_t;

/**
 * Converts an array of chars to a Uint16 (which can then be casted to an Int16 if
 * desired).
 * The array needs to have a length of 2.
 * Using LITTLE_ENDIAN puts bytes[0] in the LSB of the returned value.
 * Using BIG_ENDIAN puts bytes[0] in the MSB of the returned value.
 * 
 * @param bytes Array of 2 bytes to convert to a 16-bit number.
 * @param endian The intended endianness of the converted value
 * @return The 16-bit number formed from the byte array
 */
Uint16 utils_toUint16( unsigned char * bytes, EndianType_t EndiannessOfInputData ) ;

/**
 * Converts an array of chars into a Uint32 (which can then be casted to an Int32 if
 * desired).
 * The array needs to have a length of 4.
 * Using LITTLE_ENDIAN puts bytes[0] in the LSB of the returned value.
 * Using BIG_ENDIAN puts bytes[0] in the MSB of the returned value.
 * 
 * @param bytes Array of 4 bytes to convert to a 32-bit number.
 * @param endian The intended endianness of the converted value
 * @return The 32-bit number formed from the byte array
 */
Uint32 utils_toUint32( unsigned char * bytes, EndianType_t EndiannessOfInputData ) ;

/**
 * Converts a 16-bit number to an array of bytes
 * The array must have pre-allocated size of at least 2 (this function does not
 * allocate the array or guarantee its size).
 * Using LITTLE_ENDIAN puts the LSB of data into bytes[0]
 * Using BIG_ENDIAN puts the MSB of data into bytes[0]
 * 
 * @param bytes The byte array into which the converted number is placed
 * @param data The 16-bit number to convert
 * @param endian The endianness of the data parameter
 */
void utils_to2Bytes( unsigned char * bytes, Uint16 data, EndianType_t EndiannessOfOutputData );

/**
 * Converts a 32-bit number to an array of bytes
 * The array must have pre-allocated size of at least 4 (this function does not
 * allocate the array or guarantee its size).
 * Using LITTLE_ENDIAN puts the LSB of data into bytes[0].
 * Using BIG_ENDIAN puts the MSB of data into bytes[0], and the LSB into bytes[3]
 * 
 * @param bytes The byte array into which the converted number is placed
 * @param data The 32-bit number to convert
 * @param endian The endianness of the data parameter
 */
void utils_to4Bytes( unsigned char * bytes, Uint32 data, EndianType_t EndiannessOfOutputData );

#endif   // UTILS_H
