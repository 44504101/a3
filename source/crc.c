// ----------------------------------------------------------------------------
/**
 * @file    	common/src/crc.c
 * @author		Fei Li (LIF@xsyu.edu.cn)
 * @date		28 Mar 2013
 * @brief		CRC-CCITT functions.
 * @details
 * Calculates the 16 bit CRC-CCITT checksum for a buffer.
 *
 * Although this code is intended to be platform agnostic, it leans towards the
 * fact that the initial target platform has a minimum addressable unit of
 * 16 bits, so the buffer pointers used are 16 bits, but the length of the
 * buffer is given in bytes (to allow the standard test case below to work).
 * The CRC-CCITT result for ASCII 123456789 is 0x29B1 with an initial value
 * for the CRC of 0xFFFF, and is 0xE5CC with an initial value of 0x1D0F.
 *
 * Please refer to:
 * www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
 * www.lammertbies.nl/comm/info/crc-calculation.html
 * for more information.
 *
 * @warning
 * Data is processed in little-endian format.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. DD Lab, unpublished work, created 2013.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. DD Lab  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include "crc.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/**
 * CRC lookup table for CRC-16-CCITT (as used for HDLC, Bluetooth etc).
 * This table is for polynomial: x^16 + x^12 + x^5 + 1, processing from least
 * to most significant bits.
 */
static const uint16_t CRCtable[] =
{
     0x0000u, 0x1021u, 0x2042u, 0x3063u, 0x4084u, 0x50A5u, 0x60C6u, 0x70E7u,
     0x8108u, 0x9129u, 0xA14Au, 0xB16Bu, 0xC18Cu, 0xD1ADu, 0xE1CEu, 0xF1EFu,
     0x1231u, 0x0210u, 0x3273u, 0x2252u, 0x52B5u, 0x4294u, 0x72F7u, 0x62D6u,
     0x9339u, 0x8318u, 0xB37Bu, 0xA35Au, 0xD3BDu, 0xC39Cu, 0xF3FFu, 0xE3DEu,
     0x2462u, 0x3443u, 0x0420u, 0x1401u, 0x64E6u, 0x74C7u, 0x44A4u, 0x5485u,
     0xA56Au, 0xB54Bu, 0x8528u, 0x9509u, 0xE5EEu, 0xF5CFu, 0xC5ACu, 0xD58Du,
     0x3653u, 0x2672u, 0x1611u, 0x0630u, 0x76D7u, 0x66F6u, 0x5695u, 0x46B4u,
     0xB75Bu, 0xA77Au, 0x9719u, 0x8738u, 0xF7DFu, 0xE7FEu, 0xD79Du, 0xC7BCu,
     0x48C4u, 0x58E5u, 0x6886u, 0x78A7u, 0x0840u, 0x1861u, 0x2802u, 0x3823u,
     0xC9CCu, 0xD9EDu, 0xE98Eu, 0xF9AFu, 0x8948u, 0x9969u, 0xA90Au, 0xB92Bu,
     0x5AF5u, 0x4AD4u, 0x7AB7u, 0x6A96u, 0x1A71u, 0x0A50u, 0x3A33u, 0x2A12u,
     0xDBFDu, 0xCBDCu, 0xFBBFu, 0xEB9Eu, 0x9B79u, 0x8B58u, 0xBB3Bu, 0xAB1Au,
     0x6CA6u, 0x7C87u, 0x4CE4u, 0x5CC5u, 0x2C22u, 0x3C03u, 0x0C60u, 0x1C41u,
     0xEDAEu, 0xFD8Fu, 0xCDECu, 0xDDCDu, 0xAD2Au, 0xBD0Bu, 0x8D68u, 0x9D49u,
     0x7E97u, 0x6EB6u, 0x5ED5u, 0x4EF4u, 0x3E13u, 0x2E32u, 0x1E51u, 0x0E70u,
     0xFF9Fu, 0xEFBEu, 0xDFDDu, 0xCFFCu, 0xBF1Bu, 0xAF3Au, 0x9F59u, 0x8F78u,
     0x9188u, 0x81A9u, 0xB1CAu, 0xA1EBu, 0xD10Cu, 0xC12Du, 0xF14Eu, 0xE16Fu,
     0x1080u, 0x00A1u, 0x30C2u, 0x20E3u, 0x5004u, 0x4025u, 0x7046u, 0x6067u,
     0x83B9u, 0x9398u, 0xA3FBu, 0xB3DAu, 0xC33Du, 0xD31Cu, 0xE37Fu, 0xF35Eu,
     0x02B1u, 0x1290u, 0x22F3u, 0x32D2u, 0x4235u, 0x5214u, 0x6277u, 0x7256u,
     0xB5EAu, 0xA5CBu, 0x95A8u, 0x8589u, 0xF56Eu, 0xE54Fu, 0xD52Cu, 0xC50Du,
     0x34E2u, 0x24C3u, 0x14A0u, 0x0481u, 0x7466u, 0x6447u, 0x5424u, 0x4405u,
     0xA7DBu, 0xB7FAu, 0x8799u, 0x97B8u, 0xE75Fu, 0xF77Eu, 0xC71Du, 0xD73Cu,
     0x26D3u, 0x36F2u, 0x0691u, 0x16B0u, 0x6657u, 0x7676u, 0x4615u, 0x5634u,
     0xD94Cu, 0xC96Du, 0xF90Eu, 0xE92Fu, 0x99C8u, 0x89E9u, 0xB98Au, 0xA9ABu,
     0x5844u, 0x4865u, 0x7806u, 0x6827u, 0x18C0u, 0x08E1u, 0x3882u, 0x28A3u,
     0xCB7Du, 0xDB5Cu, 0xEB3Fu, 0xFB1Eu, 0x8BF9u, 0x9BD8u, 0xABBBu, 0xBB9Au,
     0x4A75u, 0x5A54u, 0x6A37u, 0x7A16u, 0x0AF1u, 0x1AD0u, 0x2AB3u, 0x3A92u,
     0xFD2Eu, 0xED0Fu, 0xDD6Cu, 0xCD4Du, 0xBDAAu, 0xAD8Bu, 0x9DE8u, 0x8DC9u,
     0x7C26u, 0x6C07u, 0x5C64u, 0x4C45u, 0x3CA2u, 0x2C83u, 0x1CE0u, 0x0CC1u,
     0xEF1Fu, 0xFF3Eu, 0xCF5Du, 0xDF7Cu, 0xAF9Bu, 0xBFBAu, 0x8FD9u, 0x9FF8u,
     0x6E17u, 0x7E36u, 0x4E55u, 0x5E74u, 0x2E93u, 0x3EB2u, 0x0ED1u, 0x1EF0u
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * CRC_Check calculates the CRC for a certain number of bytes, and compares
 * the result against an expected value.
 *
 * @param	pBuffer			Pointer to buffer to calculate CRC of.
 * @param	LengthInBytes	Number of BYTES in the buffer.
 * @param   InitialValue    Initial CRC seed value.
 * @param	ExpectedCRC		Expected CRC value from buffer.
 * @retval	bool_t			TRUE if CRC's match, FALSE if they don't match.
 *
 */
// ----------------------------------------------------------------------------
bool_t CRC_Check(const uint16_t* const pBuffer, const uint32_t LengthInBytes,
                    const uint16_t InitialValue, const uint16_t ExpectedCRC)
{
    uint16_t 	result;
    bool_t		bCRCIsOK = FALSE;

    result = CRC_CCITTCalculate(pBuffer, LengthInBytes, InitialValue);

    if (result == ExpectedCRC)
    {
        bCRCIsOK = TRUE;
    }

    return bCRCIsOK;
}


// ----------------------------------------------------------------------------
/**
 * CRC_CCITTCalculate calculates the CCITT CRC checksum for a certain number
 * of bytes.  The data is assumed to be presented in little-endian format.
 *
 * @param	pBuffer			Pointer to buffer to calculate CRC of.
 * @param	LengthInBytes	Number of BYTES in the buffer.
 * @param   InitialValue    Initial value to start the CRC off with.
 * @retval	uint16_t		Calculated CRC.
 *
 */
// ----------------------------------------------------------------------------
uint16_t CRC_CCITTCalculate(const uint16_t * const pBuffer,
                            uint32_t LengthInBytes,
                            const uint16_t InitialValue)
{
    uint16_t	Crc;
//    uint16_t	next_byte;
//    uint16_t	tmp;
//    uint16_t	bitshift = 0u;	// Initial bit shift such that we do LSB first
    uint16_t	index = 0u;
    uint16_t    tmp_L;
    uint16_t    tmp_H;
    uint16_t  next_byte_L;
    uint16_t  next_byte_H;

    // Initial value for CCITT is normally either 0xFFFF or 0x1D0F.
    // Set the initial value to whatever is required here.
    Crc = InitialValue;

//    while (LengthInBytes != 0u)
//    {
//    	// Get next byte by reading from 16 bit location, applying the
//    	// appropriate bit shift and masking off the top 8 bits.
//    	next_byte  = (pBuffer[index] >> bitshift) & 0x00FFu;
//
//    	// Add next byte to running CRC result.
//    	tmp = (Crc >> 8) ^ next_byte;
//    	Crc = (Crc << 8) ^ CRCtable[tmp];
//
//    	// Swap bit shift to do the 'other' byte - note that we
//    	// only increment the index when we've done both bytes.
//    	bitshift ^= 0x08u;
//    	if (bitshift == 0u)
//    	{
//    		index++;
//    	}
//
//    	LengthInBytes--;
//    }
    //岳crc校验20211005
    while (LengthInBytes != 0u)
     {
         // Get next byte by reading from 16 bit location, applying the
         // appropriate bit shift and masking off the top 8 bits.
         next_byte_L  = (pBuffer[index] >> 0) & 0x00FFu;
         next_byte_H  = (pBuffer[index] >> 8) & 0x00FFu;
         // Add next byte to running CRC result.
         tmp_L = (Crc >> 8) ^ next_byte_L;
         Crc = (Crc << 8) ^ CRCtable[tmp_L];

         tmp_H = (Crc >> 8) ^ next_byte_H;
         Crc = (Crc << 8) ^ CRCtable[tmp_H];

         // Swap bit shift to do the 'other' byte - note that we
         // only increment the index when we've done both bytes.
         index++;
         LengthInBytes--;
     }
    return Crc;
}

// ----------------------------------------------------------------------------
/**
 * CRC_CCITTOnByteCalculate calculates the CCITT CRC checksum for a certain number
 * of bytes.
 *
 * @param	pBuffer			Pointer to buffer to calculate CRC of.
 * @param	LengthInBytes	Number of BYTES in the buffer.
 * @param   InitialValue    Initial value to start the CRC off with.
 * @retval	uint16_t		Calculated CRC.
 *
 */
// ----------------------------------------------------------------------------
uint16_t CRC_CCITTOnByteCalculate(const uint8_t * const pBuffer,
                                  uint32_t LengthInBytes,
                                  const uint16_t InitialValue)
{
    uint16_t	Crc;
    uint16_t	next_byte;
    uint16_t	tmp;
    uint16_t	index = 0u;

    // Initial value for CCITT is normally either 0xFFFF or 0x1D0F.
    // Set the initial value to whatever is required here.
    Crc = InitialValue;

    while (LengthInBytes != 0u)
    {
    	// Get next byte by reading from 16 bit location, applying the
    	// appropriate bit shift and masking off the top 8 bits.
        //lint -e{921} Cast to uint16_t
    	next_byte  = (uint16_t)pBuffer[index];

    	// Add next byte to running CRC result - the cast is to stop Lint
    	// complaining about the << operator.
    	tmp = (Crc >> 8) ^ next_byte;
    	Crc = (Crc << 8) ^ CRCtable[tmp];
    	index++;
    	LengthInBytes--;
    }

    return Crc;
}


/*
 * 2022/9/8 雷戈添加，现有计算CRC与我们的校验和计算不符
 *
 * 将所有的内容相加
 *
 * */
uint16_t CheckNum_Calculate(const uint8_t * const pBuffer,
                                  uint32_t LengthInBytes,
                                  const uint16_t InitialValue){
    uint16_t    Crc;
    uint16_t    index = 0u;

    Crc = InitialValue;

    while (LengthInBytes != 0u)
    {
        Crc += pBuffer[index];
        index++;
        LengthInBytes--;
    }
    return Crc;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
