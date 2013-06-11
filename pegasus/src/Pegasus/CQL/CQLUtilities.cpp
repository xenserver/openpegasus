//%LICENSE////////////////////////////////////////////////////////////////
//
// Licensed to The Open Group (TOG) under one or more contributor license
// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
// this work for additional information regarding copyright ownership.
// Each contributor licenses this file to you under the OpenPegasus Open
// Source License; you may not use this file except in compliance with the
// License.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////
//
//%/////////////////////////////////////////////////////////////////////////////

#include <Pegasus/CQL/CQLUtilities.h>

// Query includes
#include <Pegasus/Query/QueryCommon/QueryException.h>

// Pegasus Common includes
#include <Pegasus/Common/Tracer.h>

// standard includes
#include <errno.h>

// symbol defines
#define PEGASUS_SINT64_MIN (PEGASUS_SINT64_LITERAL(0x8000000000000000))
#define PEGASUS_UINT64_MAX PEGASUS_UINT64_LITERAL(0xFFFFFFFFFFFFFFFF)

// required for the windows compile
#ifndef _MSC_VER
#define _MSC_VER 0
#endif

PEGASUS_NAMESPACE_BEGIN

inline Uint8 _CQLUtilities_hexCharToNumeric(Char16 c)
{
    Uint8 n;

    if (isdigit(c))
        n = (c - '0');
    else if (isupper(c))
        n = (c - 'A' + 10);
    else // if (islower(c))
        n = (c - 'a' + 10);

    return n;
}

Uint64 CQLUtilities::stringToUint64(const String &stringNum)
{
    PEG_METHOD_ENTER(TRC_CQL,"CQLUtilities::stringToUint64()");

    Uint64 x = 0;
    const Char16* p = stringNum.getChar16Data();
    const Char16* pStart = p;

    if (String::equal(stringNum, String::EMPTY))
    {
        MessageLoaderParms mload("CQL.CQLUtilities.EMPTY_STRING",
            "Error converting string to $0.  String cannot be $1.",
            "Uint64", "empty");
        throw CQLRuntimeException(mload);
    }

    if (!p)
    {
        MessageLoaderParms mload("CQL.CQLUtilities.EMPTY_STRING",
            "Error converting string to $0.  String cannot be $1.",
            "Uint64", "NULL");
        throw CQLRuntimeException(mload);
    }

    // If the string is a real number, use stringToReal, then convert to
    // a Uint64
    if (isReal(stringNum))
    {
        // Note:  the cast will rip off any non-whole number precision.
        // CQL spec is silent on whether to round up, round down, throw an
        // error, or allow the platform to round as it sees fit.
        // We chose the latter for now.
        return (Uint64) stringToReal64(stringNum);
    }

    // There cannot be a negative '-' sign
    if (*p == '-')
    {
         MessageLoaderParms mload("CQL.CQLUtilities.INVALID_NEG",
             "Error converting string to $0.  String '$1' cannot begin"
                 " with '-'.",
             "Uint64",
             stringNum);
         throw CQLRuntimeException(mload);
    }
    if (*p == '+')
        p++;  // skip over the positive sign

    if (!((*p >= '0') && (*p <= '9')))
    {
         MessageLoaderParms mload("CQL.CQLUtilities.INVALID_NUM_FORMAT",
            "Error converting string to $0.  String '$1' is badly"
                " formed.",
            "Uint64",
            stringNum);
         throw CQLRuntimeException(mload);
    }

    // if hexidecimal
    if ( (*p == '0') && ((p[1] == 'x') || (p[1] == 'X')) )
    {
        // Convert a hexadecimal string

        // Skip over the "0x"
        p+=2;

        // At least one hexadecimal digit is required
        if (!*p)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.INVALID_HEX_FORMAT",
                "Error converting string to $0.  String '$1' needs a"
                    " hexadecimal digit character following '0x'",
                "Uint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }

        // Add on each digit, checking for overflow errors
        while (isascii(*p) && isxdigit(*p))
        {
            // Make sure we won't overflow when we multiply by 16
            if (x > PEGASUS_UINT64_MAX/16)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                    "Error converting string to $0.  String '$1' caused"
                        " an overflow.",
                    "Uint64",
                    stringNum);
                throw CQLRuntimeException(mload);
            }

            x = x << 4;

            // We can't overflow when we add the next digit
            Uint64 newDigit = Uint64(_CQLUtilities_hexCharToNumeric(*p++));
            if (PEGASUS_UINT64_MAX - x < newDigit)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                    "Error converting string to $0.  String '$1' caused"
                        " an overflow.",
                    "Uint64",
                    stringNum);
                throw CQLRuntimeException(mload);
            }

            x = x + newDigit;
        }

        // If we found a non-hexadecimal digit, report an error
        if (*p)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.INVALID_HEX_CHAR",
                "Error converting string to $0.  Character '$1' in"
                    " string '$2' is not a hexidecimal digit.",
                "Uint64",
                String(p, 1), stringNum);
            throw CQLRuntimeException(mload);
        }

        // return value from the hex string
        PEG_METHOD_EXIT();
        return x;
  }  // end if hexidecimal

    // if binary
    Uint32 endString = stringNum.size() - 1;
    if ( (pStart[endString] == 'b') || (pStart[endString] == 'B') )
    {
        // Add on each digit, checking for overflow errors
        while ((*p == '0') || (*p == '1'))
        {
            // Make sure we won't overflow when we multiply by 2
            if (x > PEGASUS_UINT64_MAX/2)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                    "Error converting string to $0.  String '$1' caused"
                        " an overflow.",
                    "Uint64",
                    stringNum);
                throw CQLRuntimeException(mload);
            }

            x = x << 1;

            // We can't overflow when we add the next digit
            Uint64 newDigit = 0;
            if (*p++ == '1')
            newDigit = 1;

            if (PEGASUS_UINT64_MAX - x < newDigit)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                    "Error converting string to $0.  String '$1' caused"
                        " an overflow.",
                    "Uint64",
                    stringNum);
                throw CQLRuntimeException(mload);
            }

            x = x + newDigit;
        }

        // If we found a non-binary digit before the terminating 'b', then
        // report an error
        if (*p && (p-pStart < (Sint32)endString || (*p != 'b' && *p != 'B')))
        {
            MessageLoaderParms mload("CQL.CQLUtilities.INVALID_BIN_CHAR",
                "Error converting string to $0.  Character '$1' in"
                  " string '$2' is not a binary digit.",
                "Uint64",
                String(p, 1), stringNum);
            throw CQLRuntimeException(mload);
    }

    // return value from the binary string
    PEG_METHOD_EXIT();
    return x;
    } // end if binary


  // Expect a positive decimal digit:

  // Add on each digit, checking for overflow errors
    while ((*p >= '0') && (*p <= '9'))
    {
        // Make sure we won't overflow when we multiply by 10
        if (x > PEGASUS_UINT64_MAX/10)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused an"
                    " overflow.",
                "Uint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }
        x = 10 * x;

        // Make sure we won't overflow when we add the next digit
        Uint64 newDigit = (*p++ - '0');
        if (PEGASUS_UINT64_MAX - x < newDigit)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused an"
                    " overflow.",
                "Uint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }

    x = x + newDigit;
    }

    // If we found a non-decimal digit, report an error
    if (*p)
    {
        MessageLoaderParms mload("CQL.CQLUtilities.INVALID_DECIMAL_CHAR",
            "Error converting string to $0.  Character '$1' in"
                " string '$2' is not a decimal digit.",
            "Uint64",
            String(p, 1), stringNum);
        throw CQLRuntimeException(mload);
    }

    // return the value for the decimal string
    PEG_METHOD_EXIT();
    return x;
}

Sint64 CQLUtilities::stringToSint64(const String &stringNum)
{
    PEG_METHOD_ENTER(TRC_CQL,"CQLUtilities::stringToSint64()");

    Sint64 x = 0;
    Boolean invert = false;
    const Char16* p = stringNum.getChar16Data();
    const Char16* pStart = p;

    if (String::equal(stringNum, String::EMPTY))
    {
        MessageLoaderParms mload("CQL.CQLUtilities.EMPTY_STRING",
            "Error converting string to $0.  String cannot be $1.",
            "Sint64", "empty");
        throw CQLRuntimeException(mload);
    }

    if (!p)
    {
        MessageLoaderParms mload("CQL.CQLUtilities.EMPTY_STRING",
            "Error converting string to $0.  String cannot be $1.",
            "Sint64", "NULL");
        throw CQLRuntimeException(mload);
    }

    // If the string is a real number, use stringToReal, then convert to
    // a Sint64
    if (isReal(stringNum))
    {
        // Note:  the cast will rip off any non-whole number precision.
        // CQL spec is silent on whether to round up, round down, throw
        // an error, or allow the platform to round as it sees fit.
        // We chose the latter for now.
        return (Sint64) stringToReal64(stringNum);
    }

    // skip over the sign if there is one
    if (*p == '-')
    {
        invert = true;
        p++;
    }
    if (*p == '+')
        p++;

    if (!((*p >= '0') && (*p <= '9')))
    {
         MessageLoaderParms mload("CQL.CQLUtilities.INVALID_NUM_FORMAT",
            "Error converting string to $0.  String '$1' is badly"
                " formed.",
            "Uint64",
            stringNum);
        throw CQLRuntimeException(mload);
    }

    // ********************
    // Build the Sint64 as a negative number, regardless of the
    // eventual sign (negative numbers can be bigger than positive ones)
    // ********************

    // if hexidecimal
    if ( (*p == '0') && ((p[1] == 'x') || (p[1] == 'X')) )
    {
        // Convert a hexadecimal string

        // Skip over the "0x"
        p+=2;

        // At least one hexidecimal digit is required
        if (!*p)
        {
        MessageLoaderParms mload("CQL.CQLUtilities.INVALID_HEX_FORMAT",
            "Error converting string to $0.  String '$1' needs a"
                " hexadecimal digit character following '0x'.",
            "Sint64",
            stringNum);
        throw CQLRuntimeException(mload);
        }

        // Add on each digit, checking for overflow errors
        while (isascii(*p) && isxdigit(*p))
        {
            // Make sure we won't overflow when we multiply by 16
            if (x < PEGASUS_SINT64_MIN/16)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                    "Error converting string to $0.  String '$1' caused"
                        " an overflow.",
                    "Sint64",
                    stringNum);
                throw CQLRuntimeException(mload);
            }

            x = x << 4;

            // We can't overflow when we add the next digit
            Sint64 newDigit = Sint64(_CQLUtilities_hexCharToNumeric(*p++));
            if (PEGASUS_SINT64_MIN - x > -newDigit)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                   "Error converting string to $0.  String '$1' caused"
                       " an overflow.",
                   "Sint64",
                   stringNum);
                throw CQLRuntimeException(mload);
            }

            x = x - newDigit;
        }

        // If we found a non-hexidecimal digit, report an error
        if (*p)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.INVALID_HEX_CHAR",
                "Error converting string to $0.  Character '$1' in "
                     "string '$2' is not a hexidecimal digit.",
                "Sint64",
                String(p, 1), stringNum);
            throw CQLRuntimeException(mload);
        }

        // Return the integer to positive, if necessary, checking for an
        // overflow error
        if (!invert)
        {
            if (x == PEGASUS_SINT64_MIN)
            {
                MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                    "Error converting string to $0.  String '$1' caused"
                           " an overflow.",
                    "Sint64",
                    stringNum);
                throw CQLRuntimeException(mload);
            }
            x = -x;
        }

        // return value from the hex string
        PEG_METHOD_EXIT();
        return x;
    }  // end if hexidecimal

    // if binary
    Uint32 endString = stringNum.size() - 1;
    if ( (pStart[endString] == 'b') || (pStart[endString] == 'B') )
    {
    // Add on each digit, checking for overflow errors
    while ((*p == '0') || (*p == '1'))
    {
        // Make sure we won't overflow when we multiply by 2
        if (x < PEGASUS_SINT64_MIN/2)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused"
                       " an overflow.",
                "Sint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }

        x = x << 1;

        // We can't overflow when we add the next digit
        Sint64 newDigit = 0;
        if (*p++ == '1')
            newDigit = 1;
        if (PEGASUS_SINT64_MIN - x > -newDigit)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused an"
                       " overflow.",
                "Sint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }

        x = x - newDigit;
    }

    // If we found a non-binary digit before the terminating 'b', then
    // report an error
    if (*p && (p-pStart < (Sint32)endString || (*p != 'b' && *p != 'B')))
    {
        MessageLoaderParms mload("CQL.CQLUtilities.INVALID_BIN_CHAR",
            "Error converting string to $0.  Character '$1' in"
                 " string '$2' is not a binary digit.",
            "Sint64",
            String(p, 1), stringNum);
        throw CQLRuntimeException(mload);
    }

    // Return the integer to positive, if necessary, checking for an
    // overflow error
    if (!invert)
    {
        if (x == PEGASUS_SINT64_MIN)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused"
                     " an overflow.",
                "Sint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }
        x = -x;
    }

    // return value from the binary string
    PEG_METHOD_EXIT();
    return x;
    }  // end if binary

    // Expect a positive decimal digit:

    // Add on each digit, checking for overflow errors
    while ((*p >= '0') && (*p <= '9'))
    {
        // Make sure we won't overflow when we multiply by 10
        if (x < PEGASUS_SINT64_MIN/10)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused an"
                       " overflow.",
                "Sint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }
        x = 10 * x;

        // Make sure we won't overflow when we add the next digit
        Sint64 newDigit = (*p++ - '0');
        if (PEGASUS_SINT64_MIN - x > -newDigit)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused an"
                       " overflow.",
                "Sint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }

        x = x - newDigit;
    }

    // If we found a non-decimal digit, report an error
    if (*p)
    {
        MessageLoaderParms mload("CQL.CQLUtilities.INVALID_DECIMAL_CHAR",
            "Error converting string to $0.  Character '$1' in string"
                   " '$2' is not a decimal digit.",
            "Sint64",
            String(p, 1), stringNum);
        throw CQLRuntimeException(mload);
    }

    // Return the integer to positive, if necessary, checking for an
    // overflow error
    if (!invert)
    {
        if (x == PEGASUS_SINT64_MIN)
        {
            MessageLoaderParms mload("CQL.CQLUtilities.OVERFLOW",
                "Error converting string to $0.  String '$1' caused an"
                       " overflow.",
                "Sint64",
                stringNum);
            throw CQLRuntimeException(mload);
        }
        x = -x;
    }

    // return the value for the decimal string
    PEG_METHOD_EXIT();
    return x;
}

Real64 CQLUtilities::stringToReal64(const String &stringNum)
{
    PEG_METHOD_ENTER(TRC_CQL,"CQLUtilities::stringToReal64()");

    Real64 x = 0;
    const Char16* p = stringNum.getChar16Data();
    Boolean neg = false;
    const Char16* pStart = p;

    if (String::equal(stringNum, String::EMPTY))
    {
        MessageLoaderParms mload("CQL.CQLUtilities.EMPTY_STRING",
            "Error converting string to $0.  String cannot be $1.",
            "Real64", "empty");
        throw CQLRuntimeException(mload);
    }

    if (!p)
    {
        MessageLoaderParms mload("CQL.CQLUtilities.EMPTY_STRING",
            "Error converting string to $0.  String cannot be $1.",
            "Real64", "NULL");
        throw CQLRuntimeException(mload);
    }

    // Skip optional sign:

    if (*p == '+')
        p++;

    if (*p  == '-')
    {
        neg = true;
        p++;
    };

    // Check if it it is a binary or hex integer
    Uint32 endString = stringNum.size() - 1;
    if ((*p == '0' && (p[1] == 'x' || p[1] == 'X')) ||  // hex OR
        pStart[endString] == 'b' || pStart[endString] == 'B')  // binary
    {
        if (neg)
            x = stringToSint64(stringNum);
        else

// Check if the complier is MSVC 6, which does not support the conversion
//  operator from Uint64 to Real64
#if defined(PEGASUS_PLATFORM_WIN32_IX86_MSVC) && (_MSC_VER < 1300)
    {
        Uint64 num = stringToUint64(stringNum);
        Sint64 half = num / 2;
        x = half;
        x += half;
        if (num % 2)  // if odd, then add the lost remainder
            x += 1;
    }
#else
        x = stringToUint64(stringNum);
#endif
        PEG_METHOD_EXIT();
        return x;
    }

    // Skip optional first set of digits:

    while ((*p >= '0') && (*p <= '9'))
        p++;

    // Test if optional dot is there
    if (*p++ == '.')
    {
        // One or more digits required:
        if (!((*p >= '0') && (*p <= '9')))
        {
            MessageLoaderParms mload("CQL.CQLUtilities.INVALID_CHAR_POST_DOT",
                "Error converting string to Real64.  String '$0' must "
                    "have a digit character following the decimal point.",
                stringNum);
            throw CQLRuntimeException(mload);
        }
        p++;

        while ((*p >= '0') && (*p <= '9'))
            p++;

        // If there is an exponent now:
        if (*p)
        {
            // Test exponent:

            if (*p != 'e' && *p != 'E')
            {
                MessageLoaderParms mload("CQL.CQLUtilities.INVALID_REAL_CHAR",
                    "Error converting string to $0.  Character '$1' in "
                        "string '$2' is invalid.",
                    "Real64",
                    String(p, 1), stringNum);
                throw CQLRuntimeException(mload);
            }
            p++;

            // Skip optional sign:

            if (*p == '+' || *p  == '-')
                p++;

            // One or more digits required:
            if (!((*p >= '0') && (*p <= '9')))
            {
                MessageLoaderParms mload("CQL.CQLUtilities.INVALID_REAL_EXP",
                    "Error converting string to Real64.  String '$0'"
                        " has an exponent that is not well formed.  Character"
                        " '$1' is invalid.",
                    stringNum, String(p, 1));
                throw CQLRuntimeException(mload);
            }
            p++;

            while ((*p >= '0') && (*p <= '9'))
                p++;
        }
    } // end-if optional decimal point
    if (p - pStart < (Sint32) stringNum.size())
    {
        //  printf("This is char # %d\n", p - pStart);
        MessageLoaderParms mload("CQL.CQLUtilities.INVALID_DECIMAL_CHAR",
            "Error converting string to $0.  Character '$1' in string"
                   " '$2' is not a decimal digit.",
            "Real64",
            String(p-1, 1), stringNum);
        throw CQLRuntimeException(mload);
    }
    //
    // Do the conversion
    //
    char* end;
    errno = 0;
    CString temp = stringNum.getCString();
    x = strtod((const char *) temp, &end);
    if (*end || (errno == ERANGE))
    {
        MessageLoaderParms mload("CQL.CQLUtilities.CONVERSION_REAL_ERROR",
            "String '$0' was unable to be converted to a Real64."
                   "  It could be out of range.",
            stringNum);
        throw CQLRuntimeException(mload);
    }
    PEG_METHOD_EXIT();
    //  printf("String %s = %.16e\n", (const char *)stringNum.getCString(), x);
      return x;
}

String CQLUtilities::formatRealStringExponent(const String &realString)
{
    String newString(realString);
    Uint32 expIndex = PEG_NOT_FOUND;
    Uint32 index = newString.size() - 1;

    expIndex = newString.find('E');
    if (expIndex == PEG_NOT_FOUND)
        expIndex = newString.find('e');

    if (expIndex == PEG_NOT_FOUND)
        return newString;  // no exponent symbol, so just return

    // format the exponent
    index = expIndex + 1;  // start index at next character
    if (newString[index] == '+')
        newString.remove(index, 1);  // remove the '+' symbol

    if (newString[index] == '-')
        index++;   // skip the '-' exponent sign

    while (newString[index] == '0' && index < newString.size())
    {
        newString.remove(index, 1);
    }

    // If only an 'e' is left (only 0's behind it) then strip the 'e'
    if (index >= newString.size())
        newString.remove(expIndex, 1);

    return newString;
}

Boolean CQLUtilities::isReal(const String &numString)
{
    // If there is a decimal point, we consider it to be a real.
    if (numString.find('.') == PEG_NOT_FOUND)
        return false;
    return true;
}

PEGASUS_NAMESPACE_END
