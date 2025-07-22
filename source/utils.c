/**
 * \file
 * Implementation of basic loader utility functions
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

#include "common_data_types.h"
#include "utils.h"

Uint16 utils_toUint16( unsigned char * bytes, EndianType_t EndiannessOfInputData )
{
    Uint16 retval = 0;
 
    if( LITTLE_ENDIAN == EndiannessOfInputData )
    {
        retval = ((Uint16)bytes[1] << 8) & 0xff00;
        retval |= ( bytes[0] & 0xff );
    }
    else
    {
        retval = ((Uint16)bytes[0] << 8) & 0xff00;
        retval |= ( bytes[1] & 0xff );
    }
    
    return retval;
}

Uint32 utils_toUint32( unsigned char * bytes, EndianType_t EndiannessOfInputData )
{
    Uint32 retval = 0;
    Uint32 temp;
    
    if( LITTLE_ENDIAN == EndiannessOfInputData )
    {
        temp = (Uint32)bytes[3] << 8;
        temp <<= 8;
        temp <<= 8;
        retval = temp & 0xff000000;
        temp = (Uint32)bytes[2] << 8;
        temp <<= 8;
        retval |= ( temp & 0xff0000 );
        retval |= ( ((Uint32)bytes[1] << 8) & 0xff00);
        retval |= ((Uint32)bytes[0] & 0xff);
    }
    else
    {
        temp = (Uint32)bytes[0] << 8;
        temp <<= 8;
        temp <<= 8;
        retval = temp & 0xff000000;
        temp = (Uint32)bytes[1] << 8;
        temp <<= 8;
        retval |= ( temp & 0xff0000 );
        retval |= ( ((Uint32)bytes[2] << 8 ) & 0xff00 );
        retval |= ( ((Uint32)bytes[3] ) & 0xff );
    }
    
    return retval;
}

void utils_to2Bytes( unsigned char * bytes, Uint16 data, EndianType_t EndiannessOfOutputData )
{
    if( LITTLE_ENDIAN == EndiannessOfOutputData )
    {
        bytes[0] = data & 0xff;
        bytes[1] = ( data >> 8 ) & 0xff;
    }
    else
    {
        bytes[1] = data & 0xff;
        bytes[0] = ( data >> 8 ) & 0xff;
    }
}

void utils_to4Bytes( unsigned char * bytes, Uint32 data, EndianType_t EndiannessOfOutputData )
{
    if( LITTLE_ENDIAN == EndiannessOfOutputData )
    {
        bytes[0] = data & 0xff;
        bytes[1] = ( data >> 8 ) & 0xff;
        bytes[2] = ( data >> 16 ) & 0xff;
        bytes[3] = ( data >> 24 ) & 0xff;
    }
    else
    {
        bytes[3] = data & 0xff;
        bytes[2] = ( data >> 8 ) & 0xff;
        bytes[1] = ( data >> 16 ) & 0xff;
        bytes[0] = ( data >> 24 ) & 0xff;
    }
}
