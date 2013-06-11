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

//%////////////////////////////////////////////////////////////////////////////

#ifndef Pegasus_reg_table_h
#define Pegasus_reg_table_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/ArrayInter.h>
#include <Pegasus/Common/HashTable.h>
#include <Pegasus/Common/System.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/MessageQueueService.h>
#include <Pegasus/Server/Linkage.h>
#include <Pegasus/Common/AutoPtr.h>

PEGASUS_NAMESPACE_BEGIN

class RegTableRecord
{
public:
    RegTableRecord(
        const CIMName& className,
        const CIMNamespaceName& namespaceName,
        const String& providerName,
        Uint32 serviceId);
    ~RegTableRecord();

    CIMName className;
    CIMNamespaceName namespaceName;
    String providerName;
    Uint32 serviceId;
private:
    RegTableRecord(const RegTableRecord&);
    RegTableRecord& operator=(const RegTableRecord&);
};

class PEGASUS_SERVER_LINKAGE DynamicRoutingTable
{
public:
    ~DynamicRoutingTable();

    static DynamicRoutingTable* getRoutingTable();

    // get a single service that can route this spec.
    Boolean getRouting(
        const CIMName& className,
        const CIMNamespaceName& namespaceName,
        String& provider,
        Uint32 &serviceId) const;

    void insertRecord(
        const CIMName& className,
        const CIMNamespaceName& namespaceName,
        const String& provider,
        Uint32 serviceId);
#ifdef PEGASUS_DEBUG
    void dumpRegTable();
#endif
private:
    DynamicRoutingTable();
    DynamicRoutingTable(const DynamicRoutingTable& table);
    DynamicRoutingTable& operator=(const DynamicRoutingTable& table);

    String _getRoutingKey(
        const CIMName& className,
        const CIMNamespaceName& namespaceName) const;

    String _getWildRoutingKey(
        const CIMName& className) const;

    typedef HashTable<String, RegTableRecord*,
        EqualNoCaseFunc, HashFunc<String> > RoutingTable;
    RoutingTable _routingTable;

    static AutoPtr<DynamicRoutingTable> _this;
};

PEGASUS_NAMESPACE_END

#endif // Pegasus_reg_table_h
