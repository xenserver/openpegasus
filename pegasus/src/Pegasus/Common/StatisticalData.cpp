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

#include "StatisticalData.h"
#include "Tracer.h"

PEGASUS_NAMESPACE_BEGIN


// The table on the right represents the mapping from the enumerated types
// in the CIM_CIMOMStatisticalDate class ValueMap versus the internal
// message type defined in Message.h. This conversion is performed by
// getOpType() in CIMOMStatDataProvider.cpp.
//

String StatisticalData::requestName[] =
{
                                    // Enumerated     ValueMap Value
                                    // value from     from class
                                    // internal       CIM_StatisticalData
                                    // message type
                                    // -------------- -------------------
  "GetClass",                       //     1           3
  "GetInstance",                    //     2           4
  "IndicationDelivery",             //     3           26
  "DeleteClass",                    //     4           5
  "DeleteInstance",                 //     5           6
  "CreateClass",                    //     6           7
  "CreateInstance",                 //     7           8
  "ModifyClass",                    //     8           9
  "ModifyInstance",                 //     9          10
  "EnumerateClasses",               //    10          11
  "EnumerateClassNames",            //    11          12
  "EnumerateInstances",             //    12          13
  "EnumerateInstanceNames",         //    13          14
  "ExecQuery",                      //    14          15
  "Associators",                    //    15          16
  "AssociatorNames",                //    16          17
  "References",                     //    17          18
  "ReferenceNames",                 //    18          19
  "GetProperty",                    //    19          20
  "SetProperty",                    //    20          21
  "GetQualifier",                   //    21          22
  "SetQualifier",                   //    22          23
  "DeleteQualifier",                //    23          24
  "EnumerateQualifiers",            //    24          25
  "InvokeMethod"                    //    25          Not Present
};

const Uint32 StatisticalData::length = NUMBER_OF_TYPES;

StatisticalData* StatisticalData::cur = NULL;

StatisticalData* StatisticalData::current()
{
    if (cur == NULL)
    {
        cur = new StatisticalData();
    }
    return cur;
}

StatisticalData::StatisticalData()
{
    copyGSD = 0;

    for (unsigned int i=0; i<StatisticalData::length; i++)
    {
        numCalls[i] = 0;
        cimomTime[i] = 0;
        providerTime[i] = 0;
        responseSize[i] = 0;
        requestSize[i] = 0;
    }
}

void StatisticalData::addToValue(Sint64 value, Uint16 type, Uint32 t)
{
    if (type >= NUMBER_OF_TYPES)
    {
         PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL2,
             "StatData: Statistical Data Discarded.  "
                 "Invalid Request Type =  %u", type));
         return;
    }

    if (copyGSD)
    {
        AutoMutex autoMut(_mutex);
        switch (t)
        {
            case PEGASUS_STATDATA_SERVER:
                numCalls[type] += 1;
                cimomTime[type] += value;
                PEG_TRACE((TRC_STATISTICAL_DATA, Tracer::LEVEL4,
                    "StatData: SERVER: %s(%d): count = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; value = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; total = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d",
                    (const char *)requestName[type].getCString(), type,
                    numCalls[type], value, cimomTime[type]));
                break;
            case PEGASUS_STATDATA_PROVIDER:
                providerTime[type] += value;
                PEG_TRACE((TRC_STATISTICAL_DATA, Tracer::LEVEL4,
                    "StatData: PROVIDER: %s(%d): count = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; value = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; total = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d",
                    (const char *)requestName[type].getCString(), type,
                    numCalls[type], value, providerTime[type]));
                break;
        case PEGASUS_STATDATA_BYTES_SENT:
                responseSize[type] += value;
                PEG_TRACE((TRC_STATISTICAL_DATA, Tracer::LEVEL4,
                    "StatData: BYTES_SENT: %s(%d): count = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; value = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; total = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d",
                    (const char *)requestName[type].getCString(), type,
                    numCalls[type], value, responseSize[type]));
                break;
        case PEGASUS_STATDATA_BYTES_READ:
                requestSize[type] += value;
                PEG_TRACE((TRC_STATISTICAL_DATA, Tracer::LEVEL4,
                    "StatData: BYTES_READ: %s(%d): count = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; value = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d; total = %"
                        PEGASUS_64BIT_CONVERSION_WIDTH "d",
                    (const char *)requestName[type].getCString(), type,
                    numCalls[type], value, requestSize[type]));
                break;
        }
    }
}

void StatisticalData::setCopyGSD(Boolean flag)
{
    copyGSD = flag;
}

PEGASUS_NAMESPACE_END
