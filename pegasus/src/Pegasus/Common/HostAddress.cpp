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

#include <Pegasus/Common/HostAddress.h>

PEGASUS_NAMESPACE_BEGIN

#if defined (PEGASUS_OS_TYPE_WINDOWS) || !defined (PEGASUS_ENABLE_IPV6)

/*
    Address conversion utility functions.
*/

/*
    Converts given "src" text address (Ex: 127.0.0.1) to equivalent binary form
    and stores in "dst"  buffer (Ex 0x7f000001). Returns 1 if given ipv4 address
    is valid or returns -1 if invalid. Returns value in network byte order.
*/

static int _inet_ptonv4(const char *src, void *dst)
{
    Boolean isValid = true;
    Uint16 octetValue[4] = {0};
     // Check for valid IPV4 address.
    for (Uint32 octet = 1, i = 0; octet <= 4; octet++)
    {
        int j = 0;
        if (!(isascii(src[i]) && isdigit(src[i])))
        {
            isValid = false;
            break;
        }
        while (isascii(src[i]) && isdigit(src[i]))
        {
            if (j == 3)
            {
                isValid = false;
                break;
            }
            octetValue[octet-1] = octetValue[octet-1]*10 + (src[i] - '0');
            i++;
            j++;
        }
        if (octetValue[octet-1] > 255)
        {
            isValid = false;
            break;
        }
        // Check for invalid character in IP address
        if ((octet != 4) && (src[i++] != '.'))
        {
            isValid = false;
            break;
        }
        // Check for the case where it's a valid host name that happens
        // to have 4 (or more) leading all-numeric host segments.
        if ((octet == 4) && (src[i] != ':') &&
            src[i] != char(0))
        {
            isValid = false;
            break;
        }
    }
    if (!isValid)
    {
        return 0;
    }

    // Return the value in network byte order.
    Uint32 value;
    value = octetValue[0];
    value = (value << 8) + octetValue[1];
    value = (value << 8) + octetValue[2];
    value = (value << 8) + octetValue[3];
    value = htonl(value);
    memcpy (dst, &value, sizeof(Uint32));

    return 1;
}

/*
     Converts given ipv6 text address (ex. ::1) to binary form and stroes
     in "dst" buffer (ex. 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1). Returns 1
     if src ipv6 address is valid or returns -1 if invalid. Returns value
     in network byte order.
*/
static int _inet_ptonv6(const char *src, void *dst)
{
    int ccIndex = -1;
    int sNumber = 0;
    Uint16 sValues[8] = {0};
    Boolean ipv4Mapped = false;

    while (*src && sNumber < 8)
    {
        if (*src == ':')
        {
            if (!*++src)
            {
                return 0;
            }
            if (*src == ':')
            {
                if (ccIndex != -1)
                {
                    return 0;
                }
                ccIndex = sNumber;
                if (!*++src)
                {
                    break;
                }
            }
        }
        if ((isalpha(*src) && tolower(*src) <= 'f') || isdigit(*src))
        {
            // Check for ipv4 compatible ipv6 or ipv4 mapped ipv6 addresses
            if(!strchr(src, ':') && strchr(src, '.'))
            {
                if ( _inet_ptonv4 (src, sValues + sNumber) != 1)
                {
                    return 0;
                }
                sNumber += 2;
                ipv4Mapped = true;
                break;
            }
            int chars = 0;
            while (*src && *src != ':')
            {
                if (chars++ == 4)
                {
                    return 0;
                }
                if (!((isalpha(*src) && tolower(*src) <= 'f') || isdigit(*src)))
                {
                    return 0;
                }
                sValues[sNumber] = sValues[sNumber] * 16 +
                    (isdigit(*src) ? *src - '0' : (tolower(*src) - 'a' + 10));
                ++src;
            }
            sValues[sNumber] = htons(sValues[sNumber]);
            ++sNumber;
        }
        else
        {
            return 0;
        }
    }

    if ((!ipv4Mapped &&*src) || (ccIndex == -1 && sNumber < 8) ||
        (ccIndex != -1 && sNumber == 8) )
    {
        return 0;
    }
    memset(dst, 0, PEGASUS_IN6_ADDR_SIZE);
    for (int i = 0, j = 0; i < 8 ; ++i)
    {
        if (ccIndex == i)
        {
            i += 7 - sNumber;
        }
        else
        {
            memcpy ((char*) dst + i * 2, sValues + j++ , 2);
        }
    }
    return 1;
}

/*
    Converts given ipv4 address in binary form to text form. Ex. 0x7f000001
    to 127.0.0.1.
*/
static const char *_inet_ntopv4(const void *src, char *dst, Uint32 size)
{

   Uint32 n;

   memset(dst, 0, size);
   memcpy(&n, src, sizeof (Uint32));
   n = ntohl(n);
   sprintf(dst, "%u.%u.%u.%u", n >> 24 & 0xFF ,
       n >> 16 & 0xFF, n >> 8 & 0xFF, n & 0xFF);

   return dst;
}

/*
    Converts given ipv6 address in binary form to text form. Ex.
    0000000000000001 to ::1.
*/
static const char *_inet_ntopv6(const void *src, char *dst, Uint32 size)
{

    Uint16 n[8];
    int ccIndex = -1;
    int maxZeroCnt = 0;
    int zeroCnt = 0;
    int index = 0;

    memcpy (n, src, PEGASUS_IN6_ADDR_SIZE);
    memset(dst, 0, size);
    for (int i = 0; i < 8 ; ++i)
    {
        if (n[i])
        {
            if (zeroCnt)
            {
                if (zeroCnt > maxZeroCnt)
                {
                    ccIndex = index;
                    maxZeroCnt = zeroCnt;
                }
                zeroCnt = index = 0;
            }
            n[i] = ntohs (n[i]);
        }
        else
        {
            if(!zeroCnt++)
            {
                if (ccIndex == -1)
                {
                    ccIndex = i;
                }
                index = i;
            }
        }
    }
    char tmp[50];
    *dst = 0;
    zeroCnt = 0;

    for (int i = 0; i < 8 ; ++i)
    {
        if (i == ccIndex)
        {
            sprintf(tmp, "::");
            while ( i < 8 && !n[i])
            {
                ++i;
                ++zeroCnt;
            }
            --i;
        }
        else
        {
            Boolean mapped = false;
            if (ccIndex == 0 && zeroCnt > 4)
            {
                // check for ipv4 mapped ipv6 and ipv4 compatible ipv6
                // addresses.
                if (zeroCnt == 5 && n[i] == 0xffff)
                {
                    strcat(dst,"ffff:");
                    mapped = true;
                }
                else if (zeroCnt == 6 && n[6])
                {
                    mapped = true;
                }
            }
            if (mapped)
            {
                Uint32 m;
                m = htons(n[7]);
                m = (m << 16) + htons(n[6]);
                HostAddress::convertBinaryToText(AF_INET, &m, tmp, 50);
                i += 2;
            }
            else
            {
                sprintf(tmp, i < 7 && ccIndex != i + 1 ? "%x:" : "%x", n[i]);
            }
        }
        strcat(dst,tmp);
    }

    return dst;
}
#endif  // defined (PEGASUS_OS_TYPE_WINDOWS) || !defined (PEGASUS_ENABLE_IPV6)

void HostAddress::_init()
{
    _hostAddrStr = String::EMPTY;
    _isValid = false;
    _addrType = AT_INVALID;
}

HostAddress::HostAddress()
{
    _init();
}

HostAddress::HostAddress(const String &addrStr)
{
    _init();
    _hostAddrStr = addrStr;
    _parseAddress();
}

HostAddress& HostAddress::operator =(const HostAddress &rhs)
{
    if (this != &rhs)
    {
        _hostAddrStr = rhs._hostAddrStr;
        _isValid = rhs._isValid;
        _addrType = rhs._addrType;
    }

    return *this;
}

HostAddress::HostAddress(const HostAddress &rhs)
{
    *this = rhs;
}

HostAddress::~HostAddress()
{
}

void HostAddress::setHostAddress(const String &addrStr)
{
    _init();
    _hostAddrStr = addrStr;
    _parseAddress();
}

Uint32 HostAddress::getAddressType()
{
    return _addrType;
}

Boolean HostAddress::isValid()
{
    return _isValid;
}

String HostAddress::getHost()
{
    return _hostAddrStr;
}

Boolean HostAddress::equal(int af, void *p1, void *p2)
{
    switch (af)
    {
        case AT_IPV6:
             return !memcmp(p1, p2, PEGASUS_IN6_ADDR_SIZE);
        case AT_IPV4:
             return !memcmp(p1, p2, sizeof(struct in_addr));
    }

    return false;
}

void HostAddress::_parseAddress()
{
    if (_hostAddrStr.size() == 0)
        return;

    if (isValidIPV4Address(_hostAddrStr))
    {
        _isValid = true;
        _addrType = AT_IPV4;
    }
    else if (isValidIPV6Address(_hostAddrStr))
    {
        _isValid = true;
        _addrType = AT_IPV6;
    }
    else if (isValidHostName(_hostAddrStr))
    {
        _isValid = true;
        _addrType = AT_HOSTNAME;
    }
}

Boolean HostAddress::isValidIPV6Address (const String &ipv6Address)
{
    const Uint16* p = (const Uint16*)ipv6Address.getChar16Data();
    int numColons = 0;

    while (*p)
    {
        if (*p > 127)
            return false;

        if (*p == ':')
            numColons++;

        p++;
    }

    // No need to check whether IPV6 if no colons found.

    if (numColons == 0)
        return false;

    CString addr = ipv6Address.getCString();
#ifdef PEGASUS_ENABLE_IPV6
    struct in6_addr iaddr;
#else
    char iaddr[PEGASUS_IN6_ADDR_SIZE];
#endif
    return  convertTextToBinary(AT_IPV6, (const char*)addr, (void*)&iaddr) == 1;
}

Boolean HostAddress::isValidIPV4Address (const String &ipv4Address)
{
    const Uint16* src = (const Uint16*)ipv4Address.getChar16Data();
    Uint16 octetValue[4] = {0};

    for (Uint32 octet = 1, i = 0; octet <= 4; octet++)
    {
        int j = 0;

        if (!(isascii(src[i]) && isdigit(src[i])))
            return false;

        while (isascii(src[i]) && isdigit(src[i]))
        {
            if (j == 3)
                return false;

            octetValue[octet-1] = octetValue[octet-1] * 10 + (src[i] - '0');
            i++;
            j++;
        }

        if (octetValue[octet-1] > 255)
            return false;

        if ((octet != 4) && (src[i++] != '.'))
            return false;

        if ((octet == 4) && (src[i] != ':') && src[i] != char(0))
            return false;
    }

    return true;
}

Boolean HostAddress::isValidHostName (const String &hostName_)
{
    const Uint16* hostName = (const Uint16*)hostName_.getChar16Data();

    Uint32 i = 0;
    Boolean expectHostSegment = true;
    Boolean hostSegmentIsNumeric;
    while (expectHostSegment)
    {
        expectHostSegment = false;
        hostSegmentIsNumeric = true; // assume all-numeric host segment
        if (!(isascii(hostName[i]) &&
            (isalnum(hostName[i]) || (hostName[i] == '_'))))
        {
            return false;
        }
        while (isascii(hostName[i]) &&
            (isalnum(hostName[i]) || (hostName[i] == '-') ||
                (hostName[i] == '_')))
        {
            // If a non-digit is encountered, set "all-numeric"
            // flag to false
            if (isalpha(hostName[i]) || (hostName[i] == '-') ||
                (hostName[i] == '_'))
            {
                hostSegmentIsNumeric = false;
            }
            i++;
        }
        if (hostName[i] == '.')
        {
            i++;
            expectHostSegment = true;
        }
    }
    // If the last Host Segment is all numeric, then return false.
    // RFC 1123 says "highest-level component label will be alphabetic".
    if (hostSegmentIsNumeric || hostName[i] != char(0))
    {
        return false;
    }

    return true;
}


int HostAddress::convertTextToBinary(int af, const char *src, void *dst)
{
#if defined (PEGASUS_OS_TYPE_WINDOWS) || !defined (PEGASUS_ENABLE_IPV6)
    if (af == AT_IPV4)
    {
        return _inet_ptonv4(src, dst);
    }
    else if(af == AT_IPV6)
    {
        return _inet_ptonv6(src, dst);
    }
    return -1; // Unsupported address family.
#else
    return ::inet_pton(af, src, dst);
#endif
}

const char * HostAddress::convertBinaryToText(int af, const void *src,
    char *dst, Uint32 size)
{
#if defined (PEGASUS_OS_TYPE_WINDOWS) || !defined (PEGASUS_ENABLE_IPV6)
    if (af == AT_IPV6)
    {
        return _inet_ntopv6(src, dst, size);
    }
    else if (af == AT_IPV4)
    {
        return _inet_ntopv4(src, dst, size);
    }
    return 0; // Unsupported address family.
#else
    return ::inet_ntop(af, src, dst, size);
#endif
}

PEGASUS_NAMESPACE_END
