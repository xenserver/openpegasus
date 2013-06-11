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
//%////////////////////////////////////////////////////////////////////////////

#include <Pegasus/Common/MessageQueue.h>
#include <Pegasus/Common/HashTable.h>
#include <Pegasus/Common/Sharable.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/Tracer.h>
#include "reg_table.h"

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

AutoPtr<DynamicRoutingTable> DynamicRoutingTable::_this;

RegTableRecord::RegTableRecord(
    const CIMName& className_,
    const CIMNamespaceName& namespaceName_,
    const String& providerName_,
    Uint32 serviceId_)
    : className(className_),
      namespaceName(namespaceName_),
      providerName(providerName_),
      serviceId(serviceId_)
{
}

RegTableRecord::~RegTableRecord()
{
}

DynamicRoutingTable::DynamicRoutingTable()
{
}

DynamicRoutingTable::~DynamicRoutingTable()
{
    for (RoutingTable::Iterator it = _routingTable.start(); it ; it++)
    {
        delete it.value();
    }
}

DynamicRoutingTable*  DynamicRoutingTable::getRoutingTable()
{
    if (!_this.get())
    {
        _this.reset(new DynamicRoutingTable());
    }
    return _this.get();
}

inline String DynamicRoutingTable::_getRoutingKey(
    const CIMName& className,
    const CIMNamespaceName& namespaceName) const
{
    //ATTN: We don't support wild class names.
    PEGASUS_ASSERT(!className.isNull());
    String key(namespaceName.getString());
    key.append(Char16(':'));
    key.append(className.getString());

    return key;
}

inline String DynamicRoutingTable::_getWildRoutingKey(
    const CIMName& className) const
{
    //ATTN: We don't support wild class names.
    PEGASUS_ASSERT(!className.isNull());
    String key(":");
    key.append(className.getString());

    return key;
}

Boolean DynamicRoutingTable::getRouting(
    const CIMName& className,
    const CIMNamespaceName& namespaceName,
    String& providerName,
    Uint32 &serviceId) const
{
    RegTableRecord* routing = 0;
    if (_routingTable.lookup(_getRoutingKey(className, namespaceName), routing)
        || _routingTable.lookup(_getWildRoutingKey(className), routing))
    {
        providerName= routing->providerName;
        serviceId = routing->serviceId;
        return true;
    }
    return false;
}

void DynamicRoutingTable::insertRecord(
    const CIMName& className,
    const CIMNamespaceName& namespaceName,
    const String& providerName,
    Uint32 serviceId)
{
    RegTableRecord *rec = new RegTableRecord(
        className, namespaceName, providerName, serviceId);
    String _routingKey = _getRoutingKey(className, namespaceName);
    Boolean done = _routingTable.insert(_routingKey, rec);
    PEGASUS_ASSERT(done);
}

#ifdef PEGASUS_DEBUG
void DynamicRoutingTable::dumpRegTable()
{
    PEGASUS_STD(cout) << "******** Dumping Reg Table ********" <<
        PEGASUS_STD(endl);

    for (RoutingTable::Iterator it = _routingTable.start(); it ; it++)
    {
        RegTableRecord *rec = it.value();
        PEGASUS_STD(cout) << "--------------------------------" <<
            PEGASUS_STD(endl);
        PEGASUS_STD(cout) << "Class name : " << rec->className.getString()
            << PEGASUS_STD(endl);
        PEGASUS_STD(cout) << "Namespace : " << rec->namespaceName.getString()
            << PEGASUS_STD(endl);
        PEGASUS_STD(cout) << "Provider name : " << rec->providerName
            << PEGASUS_STD(endl);
        PEGASUS_STD(cout) << "Service name : "
            << MessageQueue::lookup(rec->serviceId)->getQueueName()
            << PEGASUS_STD(endl);
        PEGASUS_STD(cout) << "---------------------------------" <<
            PEGASUS_STD(endl);
    }
}
#endif
PEGASUS_NAMESPACE_END
