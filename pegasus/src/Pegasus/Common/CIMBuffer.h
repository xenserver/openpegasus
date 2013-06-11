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
#ifndef Pegasus_CIMBuffer_h
#define Pegasus_CIMBuffer_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/CIMClass.h>
#include <Pegasus/Common/CIMQualifierList.h>
#include <Pegasus/Common/CIMQualifierDecl.h>
#include <Pegasus/Common/CIMParamValue.h>
#include <Pegasus/Common/Buffer.h>
#include <Pegasus/Common/CIMNameCast.h>
#include <Pegasus/Common/CIMDateTimeRep.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/StringRep.h>
#include <Pegasus/Common/SCMOInstance.h>

#define PEGASUS_USE_MAGIC

PEGASUS_NAMESPACE_BEGIN

#define BIN_TYPE_MARKER 0xFFFF0000
#define BIN_TYPE_MARKER_CPPD    ( BIN_TYPE_MARKER | (1 << 1) )
#define BIN_TYPE_MARKER_SCMO    ( BIN_TYPE_MARKER | (1 << 2) )


/** This class serializes/deserializes CIM objects (String, CIMInstance, etc.)
    to/from a binary message stream. Serialized objects have two main
    characteristics. (1) They are aligned on suitable boundaries so the basic
    types can be accessed in place without the risk of aligment faults. (2)
    They represents strings as UCS2 characters. Performance is an overriding
    design goal of this class. The CIMBuffer class is suitable for binary
    protocols since it sacrifices size for performance; Whereas the Packer
    class is more suitable for disk storage since it favors size over
    performance.

    CIMBuffer handles network byte ordering. It uses a "reader makes right"
    policy whereby the writing process sends data in his own endianess,
    which he comminicates to the reading process (using a mechanism defined
    outside of this class). The reader checks to see if that endianess is
    the same as his own. If so, the data is used as is. Otherwise, the
    reader calls CIMBuffer::setSwap(true) to cause subsequent get calls to
    swap data ordering.
*/
class PEGASUS_COMMON_LINKAGE CIMBuffer
{
public:

    CIMBuffer();

    CIMBuffer(size_t size);

    CIMBuffer(char* data, size_t size)
    {
        _data = data;
        _ptr = _data;
        _end = data + size;
        _swap = 0;
        _validate = 0;
    }

    ~CIMBuffer();

    void setSwap(bool x)
    {
        _swap = x ? 1 : 0;
    }

    void setValidate(bool x)
    {
        _validate = x ? 1 : 0;
    }

    bool more() const
    {
        return _ptr != _end;
    }

    void rewind()
    {
        _ptr = _data;
    }

    size_t capacity()
    {
        return _end - _data;
    }

    size_t size()
    {
        return _ptr - _data;
    }

    const char* getData() const
    {
        return _data;
    }

    const char* getPtr() const
    {
        return _ptr;
    }

    char* release()
    {
        char* data = _data;
        _data = 0;
        _ptr = 0;
        _end = 0;
        return data;
    }

    static size_t round(size_t size)
    {
        /* Round up to nearest multiple of 8 */
        return (size + 7) & ~7;
    }

    void putBoolean(Boolean x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Uint8*)_ptr) = x ? 1 : 0;
        _ptr += 8;
    }

    void putUint8(Uint8 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Uint8*)_ptr) = x;
        _ptr += 8;
    }

    void putSint8(Sint8 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Sint8*)_ptr) = x;
        _ptr += 8;
    }

    void putUint16(Uint16 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Uint16*)_ptr) = x;
        _ptr += 8;
    }

    void putSint16(Sint16 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Sint16*)_ptr) = x;
        _ptr += 8;
    }

    void putUint32(Uint32 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Uint32*)_ptr) = x;
        _ptr += 8;
    }

    void putSint32(Sint32 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Sint32*)_ptr) = x;
        _ptr += 8;
    }

    void putUint64(Uint64 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Uint64*)_ptr) = x;
        _ptr += 8;
    }

    void putSint64(Sint64 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Sint64*)_ptr) = x;
        _ptr += 8;
    }

    void putReal32(Real32 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Real32*)_ptr) = x;
        _ptr += 8;
    }

    void putReal64(Real64 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Real64*)_ptr) = x;
        _ptr += 8;
    }

    void putChar16(Char16 x)
    {
        if (_end - _ptr < 8)
            _grow(sizeof(x));

        *((Char16*)_ptr) = x;
        _ptr += 8;
    }

    void putBytes(const void* data, size_t size)
    {
        size_t r = round(size);

        if (_end - _ptr < ptrdiff_t(r))
            _grow(r);

        memcpy(_ptr, data, size);
        _ptr += r;
    }

    void putString(const String& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getChar16Data(), n * sizeof(Char16));
    }

    // This function needs to transform UTF-8 encoded data into UTF-16
    // data will be read as String later
    void putUTF8AsString(const char * x, size_t x_size)
    {
        if (0 == x_size || 0 == x)
        {
            putUint32(0);
            putBytes("", 0);
        }
        else
        {
            // Won't need more than 2*length of x bytes
            Uint16 * p = (Uint16*) malloc(x_size << 1);
            size_t utf8_error_index;
            size_t new_size = _convert(p, x, x_size, utf8_error_index);
            putUint32(new_size);
            putBytes(p, new_size << 1);
            free(p);
        }
    }

    void putDateTime(const CIMDateTime& x)
    {
        putUint64(x._rep->usec);
        putUint32(x._rep->utcOffset);
        putUint16(x._rep->sign);
        putUint16(x._rep->numWildcards);
    }

    void putBooleanA(const Array<Boolean>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        size_t r = round(n);

        if (_end - _ptr < ptrdiff_t(r))
            _grow(r);

        for (Uint32 i = 0; i < n; i++)
            _ptr[i] = x[i] ? 1 : 0;

        _ptr += r;
    }

    void putUint8A(const Array<Uint8>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Uint8));
    }

    void putSint8A(const Array<Sint8>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Sint8));
    }

    void putUint16A(const Array<Uint16>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Uint16));
    }

    void putSint16A(const Array<Sint16>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Sint16));
    }

    void putUint32A(const Array<Uint32>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Uint32));
    }

    void putSint32A(const Array<Sint32>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Sint32));
    }

    void putUint64A(const Array<Uint64>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Uint64));
    }

    void putSint64A(const Array<Sint64>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Sint64));
    }

    void putReal32A(const Array<Real32>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Real32));
    }

    void putReal64A(const Array<Real64>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Real64));
    }

    void putChar16A(const Array<Char16>& x)
    {
        Uint32 n = x.size();
        putUint32(n);
        putBytes(x.getData(), n * sizeof(Char16));
    }

    void putStringA(const Array<String>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putString(x[i]);
    }

    void putDateTimeA(const Array<CIMDateTime>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putDateTime(x[i]);
    }

    bool getBytes(void* data, size_t size)
    {
        size_t r = round(size);

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        memcpy(data, _ptr, size);
        _ptr += r;
        return true;
    }

    bool getBoolean(Boolean& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Uint8*)_ptr);
        _ptr += 8;
        return true;
    }

    bool getUint8(Uint8& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Uint8*)_ptr);
        _ptr += 8;
        return true;
    }

    bool getSint8(Sint8& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Sint8*)_ptr);
        _ptr += 8;
        return true;
    }

    bool getUint16(Uint16& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Uint16*)_ptr);

        if (_swap)
            x = _swapUint16(x);

        _ptr += 8;
        return true;
    }

    bool getSint16(Sint16& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Sint16*)_ptr);

        if (_swap)
            x = _swapSint16(x);

        _ptr += 8;
        return true;
    }

    bool getUint32(Uint32& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Uint32*)_ptr);

        if (_swap)
            x = _swapUint32(x);

        _ptr += 8;
        return true;
    }

    bool getSint32(Sint32& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Sint32*)_ptr);

        if (_swap)
            x = _swapSint32(x);

        _ptr += 8;
        return true;
    }

    bool getUint64(Uint64& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Uint64*)_ptr);

        if (_swap)
            x = _swapUint64(x);

        _ptr += 8;
        return true;
    }

    bool getSint64(Sint64& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Sint64*)_ptr);

        if (_swap)
            x = _swapSint64(x);

        _ptr += 8;
        return true;
    }

    bool getReal32(Real32& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Real32*)_ptr);

        if (_swap)
            x = _swapReal32(x);

        _ptr += 8;
        return true;
    }

    bool getReal64(Real64& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Real64*)_ptr);

        if (_swap)
            x = _swapReal64(x);

        _ptr += 8;
        return true;
    }

    bool getChar16(Char16& x)
    {
        if (_end - _ptr < 8)
            return false;

        x = *((Char16*)_ptr);

        if (_swap)
            x = _swapChar16(x);

        _ptr += 8;
        return true;
    }

    bool getString(String& x);

    bool getDateTime(CIMDateTime& x)
    {
        Uint64 usec;

        if (!getUint64(usec))
            return false;

        Uint32 utcOffset;

        if (!getUint32(utcOffset))
            return false;

        Uint16 sign;

        if (!getUint16(sign))
            return false;

        Uint16 numWildcards;

        if (!getUint16(numWildcards))
            return false;

        CIMDateTimeRep *rep = new CIMDateTimeRep;
        rep->usec = usec;
        rep->utcOffset = utcOffset;
        rep->sign = sign;
        rep->numWildcards = numWildcards;
        x = CIMDateTime(rep);
        return true;
    }

    bool getBooleanA(Array<Boolean>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n);

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            x.append(_ptr[i]);
        }

        _ptr += r;
        return true;
    }

    bool getUint8A(Array<Uint8>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Uint8));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Uint8*)_ptr, n);
        _ptr += r;
        return true;
    }

    bool getSint8A(Array<Sint8>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Sint8));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Sint8*)_ptr, n);
        _ptr += r;
        return true;
    }

    bool getUint16A(Array<Uint16>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Uint16));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Uint16*)_ptr, n);

        if (_swap)
            _swapUint16Data((Uint16*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getSint16A(Array<Sint16>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Sint16));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Sint16*)_ptr, n);

        if (_swap)
            _swapSint16Data((Sint16*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getUint32A(Array<Uint32>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Uint32));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Uint32*)_ptr, n);

        if (_swap)
            _swapUint32Data((Uint32*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getSint32A(Array<Sint32>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Sint32));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Sint32*)_ptr, n);

        if (_swap)
            _swapSint32Data((Sint32*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getUint64A(Array<Uint64>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Uint64));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Uint64*)_ptr, n);

        if (_swap)
            _swapUint64Data((Uint64*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getSint64A(Array<Sint64>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Sint64));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Sint64*)_ptr, n);

        if (_swap)
            _swapSint64Data((Sint64*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getReal32A(Array<Real32>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Real32));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Real32*)_ptr, n);

        if (_swap)
            _swapReal32Data((Real32*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getReal64A(Array<Real64>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Real64));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Real64*)_ptr, n);

        if (_swap)
            _swapReal64Data((Real64*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    bool getChar16A(Array<Char16>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Char16));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        x.append((const Char16*)_ptr, n);

        if (_swap)
            _swapChar16Data((Char16*)x.getData(), x.size());

        _ptr += r;
        return true;
    }

    // ATTENTION:
    // This method returns a reference to the data in the buffer rather
    // than a new copy. The data will only be valid through the lifetime
    // of the CIMBuffer.
    bool getFastChar16Array(Char16** x, Uint32& n)
    {
        if (!getUint32(n))
            return false;

        size_t r = round(n * sizeof(Char16));

        if (_end - _ptr < ptrdiff_t(r))
            return false;

        *x = (Char16*)_ptr;

        if (_swap)
            _swapChar16Data(*x, n);

        _ptr += r;
        return true;
    }

    bool getStringA(Array<String>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            String tmp;

            if (!getString(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    bool getDateTimeA(Array<CIMDateTime>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMDateTime tmp;

            if (!getDateTime(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putValue(const CIMValue& x);

    bool getValue(CIMValue& x);

    void putKeyBinding(const CIMKeyBinding& x);

    bool getKeyBinding(CIMKeyBinding& x);

    // To omit the host and namespace elements of the object path, set
    // includeHostAndNamespace to false. This is required for compatibility with
    // XML transmission of instances, which excludes these elements.
    void putObjectPath(
        const CIMObjectPath& x,
        bool includeHostAndNamespace = true,
        bool includeKeyBindings = true);

    bool getObjectPath(CIMObjectPath& x);

    void putQualifier(const CIMQualifier& x);

    bool getQualifier(CIMQualifier& x);

    void putQualifierList(const CIMQualifierList& x);

    bool getQualifierList(CIMQualifierList& x);

    void putQualifierDecl(const CIMQualifierDecl& x);

    bool getQualifierDecl(CIMQualifierDecl& x);

    void putProperty(const CIMProperty& x);

    bool getProperty(CIMProperty& x);

    void putInstance(
        const CIMInstance& x,
        bool includeHostAndNamespace = true,
        bool includeKeyBindings = true);

    bool getInstance(CIMInstance& x);

    void putClass(const CIMClass& x);

    bool getClass(CIMClass& x);

    void putParameter(const CIMParameter& x);

    bool getParameter(CIMParameter& x);

    void putMethod(const CIMMethod& x);

    bool getMethod(CIMMethod& x);

    void putPropertyList(const CIMPropertyList& x);

    bool getPropertyList(CIMPropertyList& x);

    void putObject(const CIMObject& x,
        bool includeHostAndNamespace = true,
        bool includeKeyBindings = true);

    bool getObject(CIMObject& x);

    void putParamValue(const CIMParamValue& x);

    bool getParamValue(CIMParamValue& x);

    void putName(const CIMName& x)
    {
        putString(x.getString());
    }

    void putNamespaceName(const CIMNamespaceName& x)
    {
        putString(x.getString());
    }

    bool getName(CIMName& x);

    bool getNamespaceName(CIMNamespaceName& x);

    void putNameA(const Array<CIMName>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putName(x[i]);
    }

    bool getNameA(Array<CIMName>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            String tmp;

            if (!getString(tmp))
                return false;

            x.append(CIMNameCast(tmp));
        }

        return true;
    }

    void putObjectPathA(
        const Array<CIMObjectPath>& x,
        bool includeHostAndNamespace = true)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putObjectPath(x[i], includeHostAndNamespace);
    }

    bool getObjectPathA(Array<CIMObjectPath>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMObjectPath tmp;

            if (!getObjectPath(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putSCMOClass(const SCMOClass& scmoClass);
    bool getSCMOClass(SCMOClass& scmoClass);

    void putInstanceA(
        const Array<CIMInstance>& x,
        bool includeHostAndNamespace = true,
        bool includeKeyBindings = true);

    void putSCMOInstanceA(Array<SCMOInstance>& x);

    bool getSCMOInstanceA(Array<SCMOInstance>& x);

    bool getInstanceA(Array<CIMInstance>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMInstance tmp;

            if (!getInstance(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putClassA(const Array<CIMClass>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putClass(x[i]);
    }

    bool getClassA(Array<CIMClass>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMClass tmp;

            if (!getClass(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putObjectA(
        const Array<CIMObject>& x,
        bool includeHostAndNamespace = true,
        bool includeKeyBindings = true)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putObject(x[i], includeHostAndNamespace, includeKeyBindings);
    }

    bool getObjectA(Array<CIMObject>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMObject tmp;

            if (!getObject(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putParamValueA(const Array<CIMParamValue>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putParamValue(x[i]);
    }

    bool getParamValueA(Array<CIMParamValue>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMParamValue tmp;

            if (!getParamValue(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putQualifierDeclA(const Array<CIMQualifierDecl>& x)
    {
        Uint32 n = x.size();
        putUint32(n);

        for (size_t i = 0; i < n; i++)
            putQualifierDecl(x[i]);
    }

    bool getQualifierDeclA(Array<CIMQualifierDecl>& x)
    {
        Uint32 n;

        if (!getUint32(n))
            return false;

        for (Uint32 i = 0; i < n; i++)
        {
            CIMQualifierDecl tmp;

            if (!getQualifierDecl(tmp))
                return false;

            x.append(tmp);
        }

        return true;
    }

    void putPresent(Boolean flag);

    bool getPresent(Boolean& flag);

    void putTypeMarker(Uint32 typeMarker)
    {
        putUint32(typeMarker);
    }

    bool getTypeMarker(Uint32& typeMarker)
    {
        return getUint32(typeMarker);
    }

private:

    void _create(size_t);

    void _grow(size_t size);

    void _putMagic(Uint32 magic)
    {
#if defined(PEGASUS_USE_MAGIC)
        putUint32(magic);
#endif
    }

    bool _testMagic(Uint32 magic)
    {
#if defined(PEGASUS_USE_MAGIC)
        Uint32 tmp;

        if (!getUint32(tmp))
            return false;

        return tmp == magic;
#else
        return true;
#endif
    }

    Uint16 _swapUint16(Uint16 x)
    {
        return (Uint16)(
            (((Uint16)(x) & 0x00ffU) << 8) |
            (((Uint16)(x) & 0xff00U) >> 8));
    }

    Sint16 _swapSint16(Sint16 x)
    {
        return Sint16(_swapUint16(Uint16(x)));
    }

    Char16 _swapChar16(Char16 x)
    {
        return Char16(_swapUint16(Uint16(x)));
    }

    Uint32 _swapUint32(Uint32 x)
    {
        return (Uint32)(
            (((Uint32)(x) & 0x000000ffUL) << 24) |
            (((Uint32)(x) & 0x0000ff00UL) <<  8) |
            (((Uint32)(x) & 0x00ff0000UL) >>  8) |
            (((Uint32)(x) & 0xff000000UL) >> 24));
    }

    Sint32 _swapSint32(Sint32 x)
    {
        return Sint32(_swapUint32(Uint32(x)));
    }

    void _swapBytes(Uint8& x, Uint8& y)
    {
        Uint8 t = x;
        x = y;
        y = t;
    }

    Uint64 _swapUint64(Uint64 x)
    {
        union
        {
            Uint64 x;
            Uint8 bytes[8];
        }
        u;

        u.x = x;
        _swapBytes(u.bytes[0], u.bytes[7]);
        _swapBytes(u.bytes[1], u.bytes[6]);
        _swapBytes(u.bytes[2], u.bytes[5]);
        _swapBytes(u.bytes[3], u.bytes[4]);
        return u.x;
    }

    Sint64 _swapSint64(Sint64 x)
    {
        return Sint64(_swapUint64(Uint64(x)));
    }

    Real32 _swapReal32(Real32 x)
    {
        return _swapUint32(*((Uint32*)(void*)&x));
    }

    Real64 _swapReal64(Real64 x)
    {
        return _swapSint64(*((Sint64*)(void*)&x));
    }

    void _swapUint16Data(Uint16* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapUint16(*p);
    }

    void _swapSint16Data(Sint16* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapSint16(*p);
    }

    void _swapUint32Data(Uint32* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapUint32(*p);
    }

    void _swapSint32Data(Sint32* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapSint32(*p);
    }

    void _swapUint64Data(Uint64* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapUint64(*p);
    }

    void _swapSint64Data(Sint64* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapSint64(*p);
    }

    void _swapReal32Data(Real32* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapReal32(*p);
    }

    void _swapReal64Data(Real64* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapReal64(*p);
    }

    void _swapChar16Data(Char16* p, Uint32 n)
    {
        for (; n--; p++)
            *p = _swapChar16(*p);
    }

    char* _data;
    char* _end;
    char* _ptr;
    // If non-zero, the endianess of reads is swapped (big-endian is changed
    // to little-endian and visa versa).

    int _swap;
    int _validate;
};


struct CIMBufferReleaser
{
    CIMBufferReleaser(CIMBuffer& buf) : _buf(buf)
    {
    }

    ~CIMBufferReleaser()
    {
        _buf.release();
    }

private:
    CIMBuffer& _buf;
};



PEGASUS_NAMESPACE_END

#endif /* Pegasus_CIMBuffer_h */
