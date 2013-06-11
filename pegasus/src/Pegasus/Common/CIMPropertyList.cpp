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

#include "CIMPropertyListRep.h"
#include "CIMPropertyList.h"

PEGASUS_NAMESPACE_BEGIN

CIMPropertyList::CIMPropertyList()
{
    _rep = new CIMPropertyListRep();
    _rep->isNull = true;
}

CIMPropertyList::CIMPropertyList(const CIMPropertyList& x)
{
    _rep = new CIMPropertyListRep();
    _rep->propertyNames = x._rep->propertyNames;
    _rep->isNull = x._rep->isNull;
}

CIMPropertyList::CIMPropertyList(const Array<CIMName>& propertyNames)
{
    _rep = new CIMPropertyListRep();

    // ATTN: the following code is inefficient and problematic. besides
    // adding overhead to check for null property names, it has the
    // disadvantage of returning an error if only 1 of n properties are null
    // without informing the caller of which one. this is mainly a problem
    // with this object's interface. it should be more like CIMQualifierList,
    // which has a add() method that would validate one at a time.

    // ensure names are not null
    for (Uint32 i = 0, n = propertyNames.size(); i < n; i++)
    {
        if (propertyNames[i].isNull())
        {
            throw UninitializedObjectException();
        }
    }

    _rep->propertyNames = propertyNames;
    _rep->isNull = false;
}

CIMPropertyList::~CIMPropertyList()
{
    delete _rep;
}

void CIMPropertyList::set(const Array<CIMName>& propertyNames)
{
    // ATTN: the following code is inefficient and problematic. besides
    // adding overhead to check for null property names, it has the
    // disadvantage of returning an error if only 1 of n properties are null
    // without informing the caller of which one. this is mainly a problem
    // with this object's interface. it should be more like CIMQualifierList,
    // which has a add() method that would validate one at a time.

    // ensure names are not null
    for (Uint32 i = 0, n = propertyNames.size(); i < n; i++)
    {
        if (propertyNames[i].isNull())
        {
            throw UninitializedObjectException();
        }
    }

    _rep->propertyNames = propertyNames;
    _rep->isNull = false;
}

CIMPropertyList& CIMPropertyList::operator=(const CIMPropertyList& x)
{
    if (&x != this)
    {
        _rep->propertyNames = x._rep->propertyNames;
        _rep->isNull = x._rep->isNull;
    }

    return *this;
}

void CIMPropertyList::clear()
{
    _rep->propertyNames.clear();
    _rep->isNull = true;
}

Boolean CIMPropertyList::isNull() const
{
    return _rep->isNull;
}

Uint32 CIMPropertyList::size() const
{
    return _rep->propertyNames.size();
}

const CIMName& CIMPropertyList::operator[](Uint32 index) const
{
    return _rep->propertyNames[index];
}

Array<CIMName> CIMPropertyList::getPropertyNameArray() const
{
    return _rep->propertyNames;
}

PEGASUS_NAMESPACE_END
