// ----------------------------------------------------------------------------
/**
 * @file        buffer_utils.c
 * @author      Simon Haworth (SHaworth@slb.com)
 * @date        8 Aug 2012
 * @brief       Functions to format data in a buffer in various different ways.
 * @details
 * Converts 16 and 32 bit values to a number of digits, with or without leading
 * zeroes.  Also converts strings to hex \ floats, and floats to strings.
 *
 * This avoids having to use stdio.h, which is not permitted by MISRA-C:2004.
 *
 * @warning
 * These functions don't check for buffer overrun, so it is up to the user to
 * ensure that the buffer has sufficient space to accept the data.
 *
 * @attention
 * (c) Copyright Xi'an Shiyou Univ. Technology Corp., unpublished work, created 2012.
 * This computer program includes confidential, proprietary information and is a
 * trade secret of Xi'an Shiyou Univ. Technology Corp.  All use, disclosure, and/or
 * reproduction is prohibited unless authorized in writing.  All Rights Reserved.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include "common_data_types.h"
#include <string.h>
#include "buffer_utils.h"


// ----------------------------------------------------------------------------
// Defines section - add all #defines here:


// ----------------------------------------------------------------------------
// Function prototypes for functions which only have scope within this module:

static uint16_t convert_single_char_to_hex(const char_t character);


// ----------------------------------------------------------------------------
// Variables which only have scope within this module:

/// Lookup table for converting a 4 bit hex value into an ASCII character
static const char_t HexLookUp[16]=  {'0','1','2','3','4','5','6','7',
                                             '8','9','A','B','C','D','E','F'};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// CODE STARTS HERE - FUNCTIONS WITH GLOBAL SCOPE - CALLED BY OTHER MODULES
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_4BitsToHex converts the least significant 4 bits of a 16 bit
 * hex value into a single ASCII digit and places it in a buffer.  Adds a NULL
 * to the end of the buffer, in case this is the last data to be written into
 * the buffer.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   num         Value to convert into ASCII.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_4BitsToHex(char_t * const p_buffer, const uint8_t num)
{
    p_buffer[0u] = HexLookUp[num & 0x0Fu];
    p_buffer[1u] = '\0';
    return &p_buffer[1u];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_8BitsToHex converts an 8 bit hex value into 2 ASCII digits
 * and places them in a buffer.  Adds a NULL to the end of the buffer, in case
 * this is the last data to be written into the buffer.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   num         Value to convert into ASCII.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_8BitsToHex(char_t * const p_buffer, const uint8_t num)
{
    p_buffer[0u] = HexLookUp[(num>>4u) & 0x0Fu];
    p_buffer[1u] = HexLookUp[num & 0x0Fu];
    p_buffer[2u] = '\0';
    return &p_buffer[2u];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_16BitsToHex converts a 16 bit hex value into 4 ASCII digits
 * and places them in a buffer.  Adds a NULL to the end of the buffer, in case
 * this is the last data to be written into the buffer.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   num         Value to convert into ASCII.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_16BitsToHex(char_t * const p_buffer, const uint16_t num)
{
    p_buffer[0u] = HexLookUp[(num>>12u) & 0x0Fu];
    p_buffer[1u] = HexLookUp[(num>>8u) & 0x0Fu];
    p_buffer[2u] = HexLookUp[(num>>4u) & 0x0Fu];
    p_buffer[3u] = HexLookUp[num & 0x0Fu];
    p_buffer[4u] = '\0';
    return &p_buffer[4u];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_32BitsToHex converts a 32 bit hex value into 8 ASCII digits
 * and places them in a buffer.  Adds a NULL to the end of the buffer, in case
 * this is the last data to be written into the buffer.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   num         Value to convert into ASCII.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_32BitsToHex(char_t * const p_buffer, const uint32_t num)
{
    p_buffer[0u] = HexLookUp[(num>>28u) & 0x0Fu];
    p_buffer[1u] = HexLookUp[(num>>24u) & 0x0Fu];
    p_buffer[2u] = HexLookUp[(num>>20u) & 0x0Fu];
    p_buffer[3u] = HexLookUp[(num>>16u) & 0x0Fu];
    p_buffer[4u] = HexLookUp[(num>>12u) & 0x0Fu];
    p_buffer[5u] = HexLookUp[(num>>8u) & 0x0Fu];
    p_buffer[6u] = HexLookUp[(num>>4u) & 0x0Fu];
    p_buffer[7u] = HexLookUp[num & 0x0Fu];
    p_buffer[8u] = '\0';
    return &p_buffer[8u];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_BufferToUpperCase converts all lower case ASCII characters
 * in a buffer into upper case (note that the source buffer is overwritten).
 * The buffer must be terminated with a carriage return or a NULL.
 *
 * @param   p_buffer    Pointer to buffer.
 * @retval  uint16_t    Length of the buffer.
 *
 */
// ----------------------------------------------------------------------------
uint16_t BUFFER_UTILS_BufferToUpperCase(char_t * const p_buffer)
{
    uint16_t Temp;
    uint16_t Offset = 0u;

    while ( (p_buffer[Offset] != '\r') && (p_buffer[Offset] != '\0') )
    {
        // Read next character into temporary variable - note that we have to
        // use a temporary variable here because MISRA won't allow us to do
        // 'arithmetic' on a type of plain char, but then Lint complains about
        // a suspicious cast.  This is safe, so the warning is disabled.
        //lint -e{571} -e{921}
        Temp = (uint16_t)p_buffer[Offset];

        // If character is lower case then convert to upper case and put back
        // in the buffer. Note that we disable the Lint warning for casting
        // back from uint16 to char - this is safe as we have checked the
        // range of values.
        if ( (Temp > 0x60u) && (Temp < 0x7bu) )
        {
            Temp -= 0x20u;
            p_buffer[Offset] = (char_t)Temp;     //lint !e921
        }
        Offset++;
    }

    // If we've finished and the delimiter was a CR then the length needs
    // to be corrected, because we need to count the CR as a valid character.
    if (p_buffer[Offset] == '\r')
    {
        Offset++;
    }

    // Returns the length of the buffer
    return Offset;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_StringToUint16 converts an ASCII string into a uint16_t.
 * The ASCII string should contain a 16 bit hex number.
 *
 * @note
 * The result is not touched if the conversion fails.
 *
 * @param   p_buffer    Pointer to buffer containing the string.
 * @param   puResult    Pointer to location to put unsigned result in.
 * @param   Radix       BUFFER_RADIX_HEX or BUFFER_RADIX_DEC
 * @retval  bool_t      Boolean - TRUE if converted OK, FALSE if some error.
 *
 */
// ----------------------------------------------------------------------------
bool_t BUFFER_UTILS_StringToUint16(const char_t * const p_buffer,
                                     uint16_t * const p_result_uint16,
                                     const buffer_radix_t radix)
{
    bool_t      bExtractedOk = TRUE;
    uint16_t    TempValue;
    uint16_t    Length;
    uint32_t    RunningValue = 0u;
    uint32_t    Multiplier = 0x000000001u;
    char_t      Character;

    // Get length of string - cast is for Lint compliance when using GCC.
    // This is safe because the buffer will never be more than 65535 chars.
    Length = (uint16_t)strlen(p_buffer);     //lint !e921

    // If string length is zero then it cannot be converted, so exit.
    if (Length == 0u)
    {
        bExtractedOk = FALSE;
    }
    // Otherwise convert the received string.
    else
    {
        // Loop around until we run out of characters - note that we
        // start from the end of the string, as this is the LSB.
        while (Length != 0u)
        {
            // Get next character and convert it into a hex value.
            Character = p_buffer[Length - 1u];
            TempValue = convert_single_char_to_hex(Character);

            // If the converted value is 0xFFFF then an invalid character
            // was processed, so abort.  If we're converting a decimal
            // value and a hex digit was received, also abort.
            if ( (TempValue == 0xFFFFu)
                    || ( (TempValue > 9u) && (radix == BUFFER_RADIX_DEC) ) )
            {
                bExtractedOk = FALSE;
                break;
            }

            // Add this 'digit' to the running value - multiplier will
            // be 0x0001, 0x0010, 0x0100, 0x1000
            // or 1, 10, 100, 1000, 10000 depending on which digit we're
            // converting and which radix we're using.
            RunningValue += (TempValue * Multiplier);

            // Decrement length counter for next character and adjust the
            // multiplier by shifting it up 4 bits.
            Length--;

            if (radix == BUFFER_RADIX_HEX)
            {
                Multiplier = Multiplier << 4;
            }
            else
            {
                Multiplier = Multiplier * 10u;
            }
        }

        // If the extracted value is bigger than 16 bits then fail.
        if (RunningValue > 0xFFFFu)
        {
            bExtractedOk = FALSE;
        }
    }

    // Only 'return' the converted value if it has been converted correctly.
    // The cast converts our 32 bit working value back into a 16 bit result.
    // We've already checked that the value <= 0xFFFF so this is safe.
    if (bExtractedOk)
    {
        *p_result_uint16 = (uint16_t)RunningValue;     //lint !e921
    }

    return bExtractedOk;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Uint16ToDecimal converts a 16 bit hex value into up to
 * 5 decimal ASCII digits and places them in a buffer.  Adds a NULL to the
 * end of the buffer, in case this is the last data to be written into the
 * buffer. Pads with leading zeros if the flag bIsZeroPaddingRequired is TRUE.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   num         Value to convert into ASCII.
 * @param   bIsZeroPaddingRequired  TRUE if padding required, FALSE if not.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_Uint16ToDecimal(char_t * const p_buffer,
                                       uint16_t num,
                                       const bool_t b_IsZeroPaddingRequired)
{
    uint16_t    PlaceCounter;
    bool_t      bFoundFirstCharacter = FALSE;
    uint16_t    Index = 0u;
    uint16_t    ComparisonValue = 9999u;
    uint16_t    SubtractionValue = 9000u;
    uint16_t    DigitCounter = 4u;

    // Loop round, converting the number of 10000's, 1000's, 100's and 10's
    while (DigitCounter != 0u)
    {
        PlaceCounter = 0u;

        // Count the number of 10000's, 1000's, 100's or 10's by subtracting
        // until the number is <= the comparison value.  Note that we subtract
        // one more than the comparison value, so we're subtracting 10000, 1000...
        while (num > ComparisonValue)
        {
            num -= (ComparisonValue + 1u);
            PlaceCounter++;
        }

        // Write the number of 10000's, 1000's, 100's or 10's into the buffer.
        // Check to make sure we're either padding with leading zeros, we have
        // a non-zero value/ or we've already found a non-zero digit to write.
        if ( b_IsZeroPaddingRequired || (PlaceCounter != 0u)
                || bFoundFirstCharacter )
        {
            p_buffer[Index] = HexLookUp[PlaceCounter];
            Index++;
            bFoundFirstCharacter = TRUE;
        }

        // Update the comparison value by subtracting the subtraction value,
        // and then update the subtraction value by dividing by 10 (to give
        // us 1000, 100 or 10).
        ComparisonValue -= SubtractionValue;
        SubtractionValue = SubtractionValue / 10u;

        DigitCounter--;
    }

    // Convert least significant digit - whatever is left.
    // Note that we always write this - even if it's zero because there must
    // be at least one digit displayed.
    // We disable the Lint warnings here for possible creation and possible
    // access of out of bounds pointer - given the program flow, this is OK.
    //lint -e{661} -e{662}
    p_buffer[Index] = HexLookUp[num];
    Index++;

    // Add NULL to the end of the buffer, just in case.
    p_buffer[Index] = '\0';

    return &p_buffer[Index];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Uint16ToFWDecimal converts a 16 bit hex value into a specified
 * number of decimal ASCII digits (FW = fixed width) and places them in a
 * buffer, padding with leading zeros to the specified number of digits.
 * Adds a NULL to the end of the buffer, in case this is the last data to be
 * written into the buffer.
 *
 * @param   p_buffer        Pointer to buffer to put characters in.
 * @param   num             Value to convert into ASCII.
 * @param   NumberOfDigits  Number of digits to display.
 * @retval  ptr             Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_Uint16ToFWDecimal(char_t * const p_buffer,
                                       const uint16_t num,
                                       const uint16_t NumberOfDigits)
{
    uint16_t        Index        = 0u;
    const char_t    Dashes[][6]  = {"", "-", "--", "---", "----", "-----"};
    const uint32_t  MaxValue[]   = {0u, 9u,  99u,  999u,  9999u,  65535u};
    bool_t          b_OutOfRange = FALSE;
    char_t          TempBuffer[6];

    // Safety check for zero digits.
    if (NumberOfDigits != 0u)
    {
        // Safety check for more than 5 digits - max value is 65535.
        if (NumberOfDigits > 5u)
        {
            Index = 5u;
            b_OutOfRange = TRUE;
        }
        else
        {
            Index = NumberOfDigits;

            // Check for the value not being able to be displayed in the
            // number of digits which has been specified.
            //lint -e{921} Cast to uint32_t to avoid wrap around during test.
            if ( (uint32_t)num > MaxValue[Index])
            {
                b_OutOfRange = TRUE;
            }
            else
            {
                // Convert the value, using a temporary buffer.
                // Note that we zero pad here (use TRUE).
                //lint -e{920} Discard the return value - don't need it.
                (void)BUFFER_UTILS_Uint16ToDecimal(&TempBuffer[0u],
                                                   num,
                                                   TRUE);

                // Copy the required number of digits of the result
                // into the 'real' buffer.
                strcpy(p_buffer, &TempBuffer[5u - NumberOfDigits]);
            }
        }

        // If out of range for any reason then just put the appropriate
        // number of dashes in the buffer (the same number as the number
        // of digits).
        if (b_OutOfRange)
        {
            strcpy(p_buffer, &Dashes[Index][0u]);
        }

    }

    // Add NULL to the end of the buffer, just in case.
    p_buffer[Index] = '\0';

    // Index will always refer to the correct position in the buffer.
    return &p_buffer[Index];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Uint32ToDecimal converts a 32 bit hex value into up to
 * 10 decimal ASCII digits and places them in a buffer.  Adds a NULL to the
 * end of the buffer, in case this is the last data to be written into the
 * buffer. Pads with leading zeros if the flag bIsZeroPaddingRequired is TRUE.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   num         Value to convert into ASCII.
 * @param   bIsZeroPaddingRequired  TRUE if padding required, FALSE if not.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_Uint32ToDecimal(char_t * const p_buffer,
                                       uint32_t num,
                                       const bool_t b_IsZeroPaddingRequired)
{
    uint16_t    PlaceCounter;
    bool_t      bFoundFirstCharacter = FALSE;
    uint16_t    Index = 0u;
    uint32_t    ComparisonValue  = 999999999u;
    uint32_t    SubtractionValue = 900000000u;
    uint16_t    DigitCounter     = 9u;

    // Loop round, converting the number of 10000's, 1000's, 100's and 10's
    while (DigitCounter != 0u)
    {
        PlaceCounter = 0u;

        // Count the number of 10000's, 1000's, 100's or 10's by subtracting
        // until the number is <= the comparison value.  Note that we subtract
        // one more than the comparison value, so we're subtracting 10000, 1000...
        while (num > ComparisonValue)
        {
            num -= (ComparisonValue + 1u);
            PlaceCounter++;
        }

        // Write the number of 10000's, 1000's, 100's or 10's into the buffer.
        // Check to make sure we're either padding with leading zeros, we have
        // a non-zero value/ or we've already found a non-zero digit to write.
        if ( b_IsZeroPaddingRequired || (PlaceCounter != 0u)
                || bFoundFirstCharacter )
        {
            p_buffer[Index] = HexLookUp[PlaceCounter];
            Index++;
            bFoundFirstCharacter = TRUE;
        }

        // Update the comparison value by subtracting the subtraction value,
        // and then update the subtraction value by dividing by 10 (to give
        // us 1000, 100 or 10).
        ComparisonValue -= SubtractionValue;
        SubtractionValue = SubtractionValue / 10u;

        DigitCounter--;
    }

    // Convert least significant digit - whatever is left.
    // Note that we always write this - even if it's zero because there must
    // be at least one digit displayed.
    // We disable the Lint warnings here for possible creation and possible
    // access of out of bounds pointer - given the program flow, this is OK.
    //lint -e{661} -e{662}
    p_buffer[Index] = HexLookUp[num];
    Index++;

    // Add NULL to the end of the buffer, just in case.
    p_buffer[Index] = '\0';

    return &p_buffer[Index];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_BackspaceRemoval removes any backspaces in the received buffer.
 * Note that the source buffer is overwritten, and the buffer must be
 * terminated with a carriage return or NULL.
 *
 * @param   p_buffer    Pointer to buffer.
 * @retval  uint16_t    Length of the buffer.
 *
 */
// ----------------------------------------------------------------------------
uint16_t BUFFER_UTILS_BackspaceRemoval(char_t * const p_buffer)
{
    uint16_t SourceOffset = 0u;
    uint16_t DestinationOffset = 0u;

    while ( (p_buffer[SourceOffset] != '\r') && (p_buffer[SourceOffset] != '\0') )
    {
        // If the character is not a backspace then move it into the correct
        // location in the buffer and increment the destination offset.
        // (Generally this is just overwriting the current location with the
        // same character, which seems inefficient but is probably easier than
        // testing to see if the offsets are the same.
        if (p_buffer[SourceOffset] != '\b')
        {
            p_buffer[DestinationOffset] = p_buffer[SourceOffset];
            DestinationOffset++;
        }
        // If the character is a backspace then we want to decrement the offset
        // to get rid of the previous character in the buffer (unless the
        // offset is already zero, so we've deleted everything).
        else
        {
            if (DestinationOffset != 0u)
            {
                DestinationOffset--;
            }
        }

        SourceOffset++;
    }

    // Copy final CR or NULL into buffer, and add extra NULL (saves testing
    // and only adding if last character was a CR).
    p_buffer[DestinationOffset] = p_buffer[SourceOffset];
    DestinationOffset++;
    p_buffer[DestinationOffset] = '\0';

    // Returns the length of the buffer
    return DestinationOffset;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Float32ToDecimal converts a 32 bit (IEEE-754 single precision)
 * float into ASCII digits and places them in a buffer.  This attempts to mimic
 * the behaviour of printf("%.?f", num), where ? is the precision, but without
 * using the dreaded \ non-portable \ dangerous stdio.h.
 * Also adds a NULL to the end of the buffer, in case this is the last data to
 * be written into the buffer.
 *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   fNumber     Value to convert into ASCII.
 * @param   Precision   Number of digits after the decimal point.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_Float32ToDecimal(char_t * const p_buffer,
                                        const float32_t number_float32,
                                        const uint16_t precision)
{
    uint16_t    Counter;
    float32_t   fMultiplier = 1.0f;
    float32_t   fWorkingValue;
    uint64_t    iWorkingValue;
    uint16_t    PlaceCounter;
    bool_t      bFoundFirstCharacter = FALSE;
    uint16_t    Index = 0u;
    uint64_t    iComparisonValue  = 9999999999999999999ULL;
    uint64_t    iSubtractionValue = 9000000000000000000ULL;
    uint16_t    DigitCounter = 19u;
    float32_t   fDecimalPointValue;
    int64_t     iZeroTestValue;

    // Calculate the multiplier to convert the floating point value
    // into a whole number, with the correct amount of precision,
    // and then convert the input value into the working value.
    Counter = precision;
    while (Counter != 0u)
    {
        fMultiplier = fMultiplier * 10.0f;
        Counter--;
    }
    fWorkingValue = number_float32 * fMultiplier;

    // Convert the working value into a 64 bit integer value, so we can check
    // to see if it's zero.  This has already been converted into a whole
    // number, so the cast is safe.
    iZeroTestValue = (int64_t)fWorkingValue;    //lint !e922

    // If number to convert is zero then just put zero, followed by
    // the required number of zeros for digits of precision in the buffer.
    if (iZeroTestValue == 0)
    {
        p_buffer[Index] = '0';
        Index++;

        // If we want some digits after the decimal point then add the
        // decimal point followed by the required number of zeros.
        if (precision != 0u)
        {
            p_buffer[Index] = '.';
            Index++;

            Counter = precision;
            while (Counter != 0u)
            {
                p_buffer[Index] = '0';
                Index++;
                Counter--;
            }
        }
    }
    // Otherwise number is non-zero, so convert to decimal.
    else
    {
        // If the value to convert if negative then put a  '-' in the
        // buffer and then convert the value into a positive number.
        if (fWorkingValue < 0.0f)
        {
            p_buffer[Index] = '-';
            Index++;
            fWorkingValue  = fWorkingValue * (-1.0f);
        }

        // If the working value is less than the multiplier then the
        // original value was zero point something, so put the zero
        // point something in the buffer first (the code below doesn't cope
        // with this case).  Do this check here so we've made the
        // working value positive for all cases.
        if (fWorkingValue < fMultiplier)
        {
            p_buffer[Index] = '0';
            Index++;
            p_buffer[Index] = '.';
            Index++;

            fDecimalPointValue = fMultiplier / 10.0f;

            while (fWorkingValue < fDecimalPointValue)
            {
                p_buffer[Index] = '0';
                Index++;
                fDecimalPointValue = fDecimalPointValue / 10.0f;
            }
        }

        // Convert the floating point working value into a 64 bit unsigned integer.
        // This 'should' be bigger than we'll ever need, but if you try and convert
        // a stupidly big value it will roll over and you'll get the wrong answer.
        iWorkingValue = (uint64_t)fWorkingValue;    //lint !e922

        // Loop round, converting each digit in turn.
        while (DigitCounter != 0u)
        {
            PlaceCounter = 0u;

            // Count number of x0000000's by subtracting until the number is <=
            // the comparison value.  Note that we subtract one more than the comparison
            // value, so we're subtracting 10000, 1000, etc...
            while (iWorkingValue > iComparisonValue)
            {
                iWorkingValue -= (iComparisonValue + 1ULL);
                PlaceCounter++;
            }

            // Write the number of x0000000's into the buffer.
            // Check to make sure we either have  a non-zero value or we've
            // already found a non-zero digit to write.
            if ( (PlaceCounter != 0u) || bFoundFirstCharacter )
            {
                p_buffer[Index] = HexLookUp[PlaceCounter];
                Index++;
                bFoundFirstCharacter = TRUE;

                // If we've reached the digit where the decimal point should be,
                // add it in.  Note that we won't go here for zero point something,
                // so it's OK that we've added it in by hand above.
                if (DigitCounter == precision)
                {
                    p_buffer[Index] = '.';
                    Index++;
                }
            }

            // Update the comparison value by subtracting the subtraction value,
            // and then update the subtraction value by dividing by 10.
            iComparisonValue -= iSubtractionValue;
            iSubtractionValue = iSubtractionValue / 10ULL;

            DigitCounter--;
        }

        // Convert least significant digit - whatever is left.
        // Note that we always write this - even if it's zero because there must
        // be at least one digit displayed.
        p_buffer[Index] = HexLookUp[iWorkingValue];
        Index++;
    }

    // Add NULL to the end of the buffer.
    p_buffer[Index] = '\0';

    return &p_buffer[Index];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Float32ToScientif converts a 32 bit (IEEE-754 single precision)
 * float into ASCII digits and places them in a buffer, using scientific notation.
  *
 * @param   p_buffer    Pointer to buffer to put characters in.
 * @param   fNumber     Value to convert into ASCII.
 * @param   Precision   Number of digits after the decimal point.
 * @retval  ptr         Pointer to location after the ones we've just used.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_Float32ToScientif(char_t * const p_buffer,
                                       const float32_t number_float32,
                                       const uint16_t precision)
{
    float32_t   workingValue = number_float32;
    float32_t   convertedValue;
    float32_t   finalConversion = 1.0f;
    char_t *    p_local;
    uint16_t    exponent = 0u;
    bool_t      b_exponentIsNegative;

    // Make the working value positive if it's less than zero.
    // This makes the exponent calculations easier to do - whether the
    // incoming value is positive or negative doesn't affect the exponent.
    if (workingValue < 0.0f)
    {
        workingValue *= (-1.0f);
    }

    // If the working value is >=1 we want to divide by 10
    // until we get a value in the range 1 <= value < 10.
    if (workingValue >= 1.0f)
    {
        b_exponentIsNegative = FALSE;

        while (workingValue >= 10.0f)
        {
            workingValue /= 10.0f;
            finalConversion *= 10.0f;
            ++exponent;
        }

        // Calculate the value to display - only do a single division
        // to avoid any risk of rounding.
        convertedValue = number_float32 / finalConversion;
    }
    // If the working value is < 1 we want to multiply by 10
    // until we get a value in the range 1 <= value < 10.
    else
    {
        // If the value is exactly zero then just treat this value
        // as if it's positive, don't try and do any multiplying.
        if (workingValue == 0.0f)
        {
            b_exponentIsNegative = FALSE;
        }
        else
        {
            b_exponentIsNegative = TRUE;

            while (workingValue < 1.0f)
            {
                workingValue *= 10.0f;
                finalConversion *= 10.0f;
                ++exponent;
            }
        }

        // Calculate the value to display - only do a single multiplication
        // to avoid any risk of rounding.
        convertedValue = number_float32 * finalConversion;
    }

    // Put the converted value in the buffer.
    // This function handles positive and negative values automatically.
    p_local = BUFFER_UTILS_Float32ToDecimal(p_buffer, convertedValue, precision);

    p_local[0u] = 'x';
    p_local[1u] = '1';
    p_local[2u] = '0';
    p_local[3u] = '^';

    // Now put the exponent in the buffer.
    if (b_exponentIsNegative)
    {
        p_local[4u] = '-';
        p_local = BUFFER_UTILS_Uint16ToDecimal(&p_local[5u], exponent, FALSE);
    }
    else
    {
        p_local = BUFFER_UTILS_Uint16ToDecimal(&p_local[4u], exponent, FALSE);
    }

    // Add NULL to the end of the buffer.
    *p_local = '\0';

    return p_local;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_StringToFloat32 converts an ASCII string into a float32_t.
 * This attempts to mimic the behaviour of sscanf(p_buffer, "%f", &pfResult),
 * but without using the dreaded \ non-portable \ dangerous stdio.h.
 *
 * @param   p_buffer    Pointer to buffer containing the string.
 * @param   pfResult    Pointer to location to put floating point result in.
 * @retval  bool_t      Boolean - TRUE if converted OK, FALSE if some error.
 *
 */
// ----------------------------------------------------------------------------
bool_t BUFFER_UTILS_StringToFloat32(const char_t * const p_buffer,
                                      float32_t * const p_result_float32)
{
    bool_t      bExtractedOk = TRUE;
    uint16_t    Length;
    uint16_t    TempValue;
    uint64_t    RunningValue = 0LLU;
    uint64_t    DigitMultiplier = 1LLU;
    char_t      Character;
    bool_t      bNumberIsNegative = FALSE;
    uint16_t    DigitCounter;
    float32_t   fTempResult;
    bool_t      bFoundDecimalPoint = FALSE;
    float32_t   fPrecisionMultiplier = 1.0f;

    // Get length of string - cast is for Lint compliance when using GCC.
    // This is safe because the buffer will never be more than 65535 chars.
    Length = (uint16_t)strlen(p_buffer);     //lint !e921

    // If string length is zero then it cannot be converted, so exit.
    if (Length == 0u)
    {
        bExtractedOk = FALSE;
    }
    // Otherwise convert the received string.
    else
    {
        // If the first character in the buffer is a minus then
        // set the negative flag and adjust the number of digits.
        if (p_buffer[0u] == '-')
        {
            bNumberIsNegative = TRUE;
            DigitCounter = Length - 1u;
        }
        // Otherwise just set the digit counter to be the string length.
        else
        {
            DigitCounter = Length;
        }

        // Loop around until we run out of characters - note that we
        // start from the end of the string, as this is the LSB.
        while (DigitCounter != 0u)
        {
            // Get next character from the input buffer.
            Character = p_buffer[Length - 1u];

            // If the character received is a decimal point, just set the
            // flag to say we've found the decimal point (unless there's
            // more than one, which is an error, so abort).
            if (Character == '.')
            {
                if (!bFoundDecimalPoint)
                {
                    bFoundDecimalPoint = TRUE;
                }
                else
                {
                    bExtractedOk = FALSE;
                }
            }
            // Otherwise convert the character, check it's valid, multiply
            // it up by the appropriate amount and add to the running value.
            else
            {
                TempValue = convert_single_char_to_hex(Character);

                // If the converted value is 0xFFFF or > 9 then an invalid
                // character was received, so abort.
                if ( (TempValue == 0xFFFFu) || (TempValue > 9u) )
                {
                    bExtractedOk = FALSE;
                }
                else
                {
                    // Add this 'digit' to the running value - multiplier will
                    // be 1, 10, 100, 1000, 10000 etc depending on which digit
                    // we're doing.
                    // We are working in 64 bit values here, hence the cast.
                    //lint -e{921}
                    RunningValue += ((uint64_t)TempValue * DigitMultiplier);

                    // Adjust the multiplier for the next digit.
                    DigitMultiplier = DigitMultiplier * 10LLU;

                    // If the decimal point hasn't been found already then keep
                    // adjusting the precision multiplier by dividing by 10.
                    // We do this as a multiplication for efficiency.
                    if (!bFoundDecimalPoint)
                    {
                        fPrecisionMultiplier = fPrecisionMultiplier * 0.1f;
                    }
                }
            }

            // Decrement length and digit counter for the next character.
            Length--;
            DigitCounter--;

            // If the ExtractedOK flag has been set to FALSE somewhere
            // then there has been an error, so abort.
            if (!bExtractedOk)
            {
                break;
            }
        }
    }

    // Only 'return' the converted value if it has been converted correctly.
    // If the number needs to be negative, multiply by minus one before
    // returning.  If there's a decimal point involved, multiply by the
    // precision multiplier before returning.
    if (bExtractedOk)
    {
        // Cast to convert from the uint64_t of the running value into a float.
        // This will be a whole number at this point, so will convert correctly.
        fTempResult = (float32_t)RunningValue;      //lint !e922

        if (bNumberIsNegative)
        {
            fTempResult = fTempResult * (-1.0f);
        }

        if (bFoundDecimalPoint)
        {
            fTempResult = fTempResult * fPrecisionMultiplier;
        }

        *p_result_float32 = fTempResult;
    }

    return bExtractedOk;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_nFloat32ToUint16 takes a normalised floating
 * point value (in the range +/- 1.0f) and convert to a 16 bit hex value
 * (4 ASCII digits).  It does this by multiplying the value by 32768.
 * Any out of range value is displayed as 0x8000.
 *
 * @param   p_buffer    Pointer to start of buffer to write to.
 * @param   fNumber     Floating point value to convert.
 * @retval  char_t*     Pointer to next free location in the buffer.
 *
 */
// ----------------------------------------------------------------------------
char_t* BUFFER_UTILS_nFloat32ToUint16(char_t * const p_buffer,
                                        const float32_t number_float32)
{
    uint16_t    Temp;
    float32_t   fTemp;

    // Check for value out of range - if it is, just put '8000' in the buffer.
    if ( (number_float32 >= 1.0f) || (number_float32 <= -1.0f) )
    {
        p_buffer[0u] = '8';
        p_buffer[1u] = '0';
        p_buffer[2u] = '0';
        p_buffer[3u] = '0';
        p_buffer[4u] = '\0';
    }
    // Otherwise multiply by 32768 and convert into 4 ASCII digits.
    else
    {
        fTemp = number_float32 * 32768.0f;

        // Convert the normalised value into an uint16_t.
        // Note the two casts - a single cast works OK under GCC but fails
        // when using the TI compiler.
        Temp = (uint16_t)(int16_t)fTemp;    //lint !e921 !e922

        // Discard the returning pointer - this always uses 4 locations.
        (void)BUFFER_UTILS_16BitsToHex(p_buffer, Temp);    //lint !e920
    }

    // Return the next location in the buffer - this is always [4u].
    return &p_buffer[4u];
}


// ----------------------------------------------------------------------------
/*!
 * BUFFER_UTILS_DataValueBufferPut uses the void* and data type which are
 * passed into the function to fetch necessary variable and put it in the buffer,
 * formatted as required.
 *
 * @param   p_buffer    Pointer to buffer to put the converted value in.
 * @param   p_variable  Void pointer to the variable to fetch.
 * @param   data_type   The data type of the variable to fetch.
 * @retval  char_t*     Pointer to next free location in the buffer.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{920} Ignoring the return values of functions - don't use them here.
char_t* BUFFER_UTILS_DataValueBufferPut(char_t * const p_buffer,
                                        const void * const p_variable,
                                        const data_types_t data_type)
{
    char_t *    returnPointer = NULL;
    uint16_t    length;
    float32_t   fValue;
    uint8_t     localBuffer[5];

    if ( (char_t*)NULL != p_buffer)
    {
        if ( (const void*)NULL == p_variable)
        {
            strcpy(p_buffer, "0x0000");
        }
        else
        {
            /*
             * Depending on the data type, convert the variable pointer to by
             * the void pointer into the correct type (cast and dereference)
             * and then put the value into the buffer.
             */
            if (data_type == DATA_TYPE_FLOAT32)
            {
                //lint -e{925} -e{9079} -e{9087} Pointer casting is OK.
                fValue = *(const float32_t*)p_variable;

                // Put value in the local buffer as IEEE754 32 bits, little-endian.
                (void)BUFFER_UTILS_Float32To8bitBuf(&localBuffer[0u], fValue);

                /// Convert the 32 bit little-endian value into 8 x ASCII digits.
                p_buffer[0u] = '0';
                p_buffer[1u] = 'x';
                (void)BUFFER_UTILS_8BitsToHex(&p_buffer[2u], localBuffer[3u]);
                (void)BUFFER_UTILS_8BitsToHex(&p_buffer[4u], localBuffer[2u]);
                (void)BUFFER_UTILS_8BitsToHex(&p_buffer[6u], localBuffer[1u]);
                (void)BUFFER_UTILS_8BitsToHex(&p_buffer[8u], localBuffer[0u]);
                p_buffer[10u] = ',';

                if ( (fValue > -1.0f) && (fValue < 1.0f) )
                {
                    // Put value in buffer using scientific notation, for small values.
                    (void)BUFFER_UTILS_Float32ToScientif(&p_buffer[11u], fValue, 6u);
                }
                else
                {
                    // Put value in buffer as a 'straight' float.
                    (void)BUFFER_UTILS_Float32ToDecimal(&p_buffer[11u], fValue, 4u);
                }
            }
            /* Display a uint16_t as decimal and hex. */
            else if (data_type == DATA_TYPE_UINT16)
            {
                //lint -e{925} -e{9079} -e{9087} Pointer casting is OK.
                (void)BUFFER_UTILS_Uint16ToDecimal(p_buffer,
                                                     *(const uint16_t*)p_variable, TRUE);
                p_buffer[5u] = ',';
                p_buffer[6u] = '0';
                p_buffer[7u] = 'x';

                //lint -e{925} -e{9079} -e{9087} Pointer casting is OK.
                (void)BUFFER_UTILS_16BitsToHex(&p_buffer[8u],
                                                *(const uint16_t*)p_variable);
            }
            else if (data_type == DATA_TYPE_BOOL)
            {
                /* Dereference the pointer to check if the flag is true or false. */
                //lint -e{925} -e{9079} -e{9087} Pointer casting is OK.
                if (*(const bool_t*)p_variable)
                {
                    strcpy(p_buffer, "TRUE");
                }
                else
                {
                    strcpy(p_buffer, "FALSE");
                }
            }
            /* All other data types are not currently supported. */
            else
            {
                strcpy(p_buffer, "data type not supported");
            }
        }

        //lint -e{921} Cast to uint16_t OK as length will always be < 65535.
        length        = (uint16_t)strlen(p_buffer);
        returnPointer = &p_buffer[length];
    }


    return returnPointer;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_8bitBufToFloat32 takes 4 x contiguous locations in an 8 bit
 * buffer, arranged LSB:MSB and extracts an IEEE-754 floating point value from
 * them.
 *
 * @note
 * We do a number of 'horrible' things in here - casting from a 32 bit value
 * to a float to get the floating point value, for example.  We use a local
 * 32 bit variable to ensure that this variable is aligned correctly on the
 * stack for using the pointer.  We have to do this as we can't use a union
 * to perform the conversion from bytes to float because the TI compiler
 * size of a byte messes everything up.
 *
 * @param   p_Buffer    Pointer to buffer to use.
 * @retval  float32_t   IEEE-754 floating point value.
 *
 */
// ----------------------------------------------------------------------------
float32_t BUFFER_UTILS_8bitBufToFloat32(const uint8_t * const p_Buffer)
{
    uint32_t    AlignedValue;

    // Get byte[0] and put it in bits [7:0] in the aligned value.
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    AlignedValue  = (uint32_t)p_Buffer[0u] & 0x000000FFu;

    // Get byte[1] and put it in bits [15:8] in the aligned value.
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    AlignedValue |= ((uint32_t)p_Buffer[1u] << 8u) & 0x0000FF00u;

    // Get byte[2] and put it in bits [23:16] in the aligned value.
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    AlignedValue |= ((uint32_t)p_Buffer[2u] << 16u) & 0x00FF0000u;

    // Get byte[3] and put it in bits [31:24] in the aligned value.
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    AlignedValue |= ((uint32_t)p_Buffer[3u] << 24u) & 0xFF000000u;

    // Return the floating point value using a dereferenced float pointer.
    //lint -e{740} -e{929} -e{9087} Pointer cast generates many warnings.
    return *(float32_t*)&AlignedValue;
}


// ----------------------------------------------------------------------------
/*!
 * BUFFER_UTILS_16bitBufToFloat32 takes 2 x contiguous locations in a 16 bit
 * buffer, arranged LSB:MSB and extracts an IEEE-754 floating point value from
 * them.
 *
 * @note
 * We do a number of 'horrible' things in here - casting from a 32 bit value
 * to a float to get the floating point value, for example.  We use a local
 * 32 bit variable to ensure that this variable is aligned correctly on the
 * stack for using the pointer.  We have to do this as we can't use a union
 * to perform the conversion from bytes to float because the TI compiler
 * size of a byte messes everything up.
 *
 * @param   p_Buffer    Pointer to buffer to use.
 * @retval  float32_t   IEEE-754 floating point value.
 *
 */
// ----------------------------------------------------------------------------
float32_t BUFFER_UTILS_16bitBufToFloat32(const uint16_t * const p_Buffer)
{
    uint32_t    AlignedValue;

    // Get LSB and put it in bits [15:0] in the aligned value.
    //lint -e{921} Cast from uint16_t to uint32_t before AND and assigning.
    AlignedValue  = (uint32_t)p_Buffer[0u] & 0x0000FFFFu;

    // Get MSB and put it in bits [31:16] in the aligned value.
    //lint -e{921} Cast from uint16_t to uint32_t before AND and assigning.
    AlignedValue |= ((uint32_t)p_Buffer[1u] << 16u) & 0xFFFF0000u;

    // Return the floating point value using a dereferenced float pointer.
    //lint -e{740} -e{929} -e{9087} Pointer cast generates many warnings.
    return *(float32_t*)&AlignedValue;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_8bitBufToUint16 takes 2 x contiguous locations in a buffer,
 * arranged LSB:MSB and extracts a uint16_t from them.
 *
 * @note
 * We mask off the most significant 8 bits because the cast cannot be
 * guaranteed to clear the top 8 bits, especially on the TI 28335 architecture.
 *
 * @param   p_Buffer    Pointer to buffer to use.
 * @retval  uint16_t    16 bit unsigned value.
 *
 */
// ----------------------------------------------------------------------------
uint16_t BUFFER_UTILS_8bitBufToUint16(const uint8_t * const p_Buffer)
{
    uint16_t    Result;

    // Get byte[0] and put it in bits [7:0] of the result (the LSB).
    //lint -e{921} Cast from uint8_t to uint16_t before AND and assigning.
    Result  = (uint16_t)p_Buffer[0u] & 0x00FFu;

    // Get byte[1] and put it in bits [15:8] of the result (the MSB).
    //lint -e{921} Cast from uint8_t to uint16_t before AND and assigning.
    Result |= ((uint16_t)p_Buffer[1u] << 8u) & 0xFF00u;

    return Result;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_8bitBufToUint16 takes 4 x contiguous locations in a buffer,
 * arranged LSB:MSB and extracts a uint32_t from them.
 *
 * @note
 * We mask off the most significant 8 bits because the cast cannot be
 * guaranteed to clear the top 8 bits, especially on the TI 28335 architecture.
 *
 * @param   p_Buffer    Pointer to buffer to use.
 * @retval  uint32_t    32 bit unsigned value.
 *
 */
// ----------------------------------------------------------------------------
uint32_t BUFFER_UTILS_8bitBufToUint32(const uint8_t * const p_Buffer)
{
    uint32_t    Result;

    // Get byte[0] and put it in bits [7:0] of the result (the LSB).
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    Result  = (uint32_t)p_Buffer[0u] & 0x000000FFu;

    // Get byte[1] and put it in bits [15:8] of the result.
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    Result |= ((uint32_t)p_Buffer[1u] << 8u) & 0x0000FF00u;

    // Get byte[2] and put it in bits [23:16] of the result.
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    Result |= ((uint32_t)p_Buffer[2u] << 16u) & 0x00FF0000u;

    // Get byte[3] and put it in bits [31:24] of the result (the MSB).
    //lint -e{921} Cast from uint8_t to uint32_t before AND and assigning.
    Result |= ((uint32_t)p_Buffer[3u] << 24u) & 0xFF000000u;

    return Result;
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Uint16To8bitBuf converts a 16 bit unsigned
 * value into two successive 'characters' (which are nominally 8 bits, stored
 * lsb then msb).
 *
 * @note
 * We mask off the most significant 8 bits because the cast cannot be
 * guaranteed to clear the top 8 bits, especially on the TI 28335 architecture.
 *
 * @param   pBuffer     Pointer to buffer to put data in, lsb then msb.
 * @param   Value       Value to convert into two successive 'characters'.
 * @retval  uint8_t*    Pointer to next free location in the buffer.
 *
 */
// ----------------------------------------------------------------------------
uint8_t* BUFFER_UTILS_Uint16To8bitBuf(uint8_t * const p_Buffer,
                                      const uint16_t Value)
{
    //lint -e{921} Cast from uint16_t to uint8_t before assigning.
    p_Buffer[0u] = (uint8_t)(Value & 0x00FFu);          // LSB

    //lint -e{921} Cast from uint16_t to uint8_t before assigning.
    p_Buffer[1u] = (uint8_t)((Value >> 8u) & 0x00FFu);  // MSB

    return &p_Buffer[2u];
}


// ----------------------------------------------------------------------------
/**
 * BUFFER_UTILS_Uint32To8bitBuf converts a 32 bit unsigned
 * value into four successive 'characters' (which are nominally 8 bits, stored
 * lsb then msb).
 *
 * @note
 * We mask off the most significant 8 bits because the cast cannot be
 * guaranteed to clear the top 8 bits, especially on the TI 28335 architecture.
 *
 * @param   pBuffer     Pointer to buffer to put data in, lsb then msb.
 * @param   Value       Value to convert into two successive 'characters'.
 * @retval  uint8_t*    Pointer to next free location in the buffer.
 *
 */
// ----------------------------------------------------------------------------
//lint -e{921} Cast from uint32_t to uint8_t before assigning is OK.
uint8_t* BUFFER_UTILS_Uint32To8bitBuf(uint8_t * const p_Buffer,
                                      const uint32_t Value)
{
    p_Buffer[0u] = (uint8_t)(Value & 0x000000FFu);          // LSB
    p_Buffer[1u] = (uint8_t)((Value >> 8u) & 0x000000FFu);
    p_Buffer[2u] = (uint8_t)((Value >> 16u) & 0x000000FFu);
    p_Buffer[3u] = (uint8_t)((Value >> 24u) & 0x000000FFu); // MSB

    return &p_Buffer[4u];
}


// ----------------------------------------------------------------------------
/*!
 * BUFFER_UTILS_Float32To8bitBuf puts a standard IEEE754 floating point value
 * (which is represented using 32 bits) into an 8 bit buffer, in little endian
 * format (LSB:MSB).
 *
 * @note
 * We are taking the 32 bit representation of the float and putting it in the
 * buffer as 4 x 8 bit values, in IEEE754 format, so we cast to a uint32_t*.
 * For example, 5.2 is represented as 0x40a66666, so we put 0x6666 (LSWORD)
 * and 0x40a6 (MSWORD) in the buffer.
 * (see www.h-schmidt.net/FloatConverter/IEEE754.html).
 *
 * @note
 * We mask off the unused bits in the 32 bit value before the shift,
 * because the cast to uint8_t cannot be guaranteed to clear the top 8 bits,
 * especially on the TI 28335 architecture.
 *
 * @param   p_Buffer    Pointer to buffer to put the result in.
 * @param   Value       Floating point value to put in the buffer.
 * @retval  uint8_t*    Pointer to next free location in the buffer.
 *
 */
// ----------------------------------------------------------------------------
uint8_t* BUFFER_UTILS_Float32To8bitBuf(uint8_t * const p_Buffer,
                                       const float32_t Value)
{
    //lint -e{740} -e{929} -e{9087} Pointer casting is nasty, but OK.
    const uint32_t* const p_Number = (const uint32_t*)&Value;

    //lint -e{921} Cast from uint32_t to uint8_t
    p_Buffer[0u] = (uint8_t)(*p_Number & 0x000000FFu);              // LSB

    //lint -e{921} Cast from uint32_t to uint8_t
    p_Buffer[1u] = (uint8_t)((*p_Number & 0x0000FF00u) >> 8u);

    //lint -e{921} Cast from uint32_t to uint8_t
    p_Buffer[2u] = (uint8_t)((*p_Number & 0x00FF0000u) >> 16u);

    //lint -e{921} Cast from uint32_t to uint8_t
    p_Buffer[3u] = (uint8_t)((*p_Number & 0x0FF000000u) >> 24u);    // MSB

    // A 32 bit float will always occupy 4 x 8 bit words in the buffer.
    return &p_Buffer[4u];
}


// ----------------------------------------------------------------------------
/*!
 * BUFFER_UTILS_Float32To16bitBuf puts a standard IEEE754 floating point value
 * (which is represented using 32 bits) into an 16 bit buffer, in little endian
 * format (LSB:MSB).
 *
 * @note
 * We are taking the 32 bit representation of the float and putting it in the
 * buffer as 2 x 8 bit values, in IEEE754 format, so we cast to a uint32_t*.
 * For example, 5.2 is represented as 0x40a66666, so we put 0x6666 (LSWORD)
 * and 0x40a6 (MSWORD) in the buffer.
 * (see www.h-schmidt.net/FloatConverter/IEEE754.html).
 *
 * @param   p_Buffer    Pointer to buffer to put the result in.
 * @param   Value       Floating point value to put in the buffer.
 * @retval  uint16_t*   Pointer to next free location in the buffer.
 *
 */
// ----------------------------------------------------------------------------
uint16_t* BUFFER_UTILS_Float32To16bitBuf(uint16_t * const p_Buffer,
                                         const float32_t Value)
{
    //lint -e{740} -e{929} -e{9087} Pointer casting is nasty, but OK.
    const uint32_t* const p_Number = (const uint32_t*)&Value;

    //lint -e{921} Cast from uint32_t to uint16_t
    p_Buffer[0u] = (uint16_t)(*p_Number & 0x0000FFFFu);             // LSWORD

    //lint -e{921} Cast from uint32_t to uint16_t
    p_Buffer[1u] = (uint16_t)((*p_Number & 0xFFFF0000u) >> 16u);    // MSWORD

    // A 32 bit float will always occupy 2 x 16 bit words in the buffer.
    return &p_Buffer[2u];
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE BELOW HERE - ONLY ACCESSIBLE BY THIS MODULE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/**
 * convert_single_char_to_hex converts a single ASCII character 0-9, A-F into
 * a hex value 0-9, A-F.  This does a reverse lookup of the HexLookUp table,
 * so it is not particularly efficient, but it is effective and simple.
 *
 * @param   Character   Character to convert.
 * @retval  uint16_t    Return value - contains value or 0xFFFF if no match.
 *
 */
// ----------------------------------------------------------------------------
static uint16_t convert_single_char_to_hex(const char_t character)
{
    uint16_t    Counter;

    // Loop round checking each character in turn against the hex lookup table.
    for (Counter = 0u; Counter < 16u; Counter++)
    {
        // Jump out if the character matches - counter is the hex value.
        if (HexLookUp[Counter] == character)
        {
            break;
        }
    }

    // If we've fallen off the end of the table then fail, so return 0xFFFF.
    if (Counter == 16u)
    {
        Counter = 0xFFFFu;
    }

    return Counter;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
