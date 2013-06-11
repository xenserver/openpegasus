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

#ifndef Pegasus_StatisticalData_h
#define Pegasus_StatisticalData_h

#include <Pegasus/Common/Config.h>
#include <iostream>
#include <cstring>
#include <Pegasus/Common/Linkage.h>
#include <Pegasus/Common/InternalException.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/CIMProperty.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/CIMDateTime.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Common/Mutex.h>
#include <Pegasus/Common/Time.h>
#include <Pegasus/Common/TimeValue.h>

PEGASUS_NAMESPACE_BEGIN

#ifndef PEGASUS_DISABLE_PERFINST

#define STAT_GETSTARTTIME \
Uint64 serverStartTimeMicroseconds = \
    TimeValue::getCurrentTime().toMicroseconds();

#define STAT_SERVERSTART \
request->setServerStartTime(serverStartTimeMicroseconds);

#define STAT_SERVEREND \
response->endServer();\

#define STAT_BYTESSENT \
Uint16 statType = (response->getType() >= CIM_GET_CLASS_RESPONSE_MESSAGE) ? \
    response->getType() - CIM_GET_CLASS_RESPONSE_MESSAGE : \
    response->getType() - 1; \
StatisticalData::current()->addToValue( \
    message.size(), statType, StatisticalData::PEGASUS_STATDATA_BYTES_SENT);


#define STAT_SERVEREND_ERROR \
response->endServer();

/*
    The request size value must be stored (requSize) and passed to the
    StatisticalData object at the end of processing other wise it will be
    the ONLY vlaue that is passed to the client which reports the current
    state of the object, not the pevious (one command ago) state
*/

#define STAT_BYTESREAD \
Uint16 statType = (request->getType() >= CIM_GET_CLASS_RESPONSE_MESSAGE) ? \
    request->getType() - CIM_GET_CLASS_RESPONSE_MESSAGE : \
    request->getType() - 1; \
StatisticalData::current()->requSize = contentLength;

#else
#define STAT_GETSTARTTIME
#define STAT_SERVERSTART
#define STAT_SERVEREND
#define STAT_SERVEREND_ERROR
#define STAT_BYTESREAD
#define STAT_BYTESSENT
#endif

class PEGASUS_COMMON_LINKAGE StatProviderTimeMeasurement
{
public:
    StatProviderTimeMeasurement(CIMMessage* message)
        : _message(message)
    {
#ifndef PEGASUS_DISABLE_PERFINST
        _startTimeMicroseconds = TimeValue::getCurrentTime().toMicroseconds();
#endif
    }

    ~StatProviderTimeMeasurement()
    {
#ifndef PEGASUS_DISABLE_PERFINST
        _message->setProviderTime(
            TimeValue::getCurrentTime().toMicroseconds() -
                _startTimeMicroseconds);
#endif
    }

private:
    StatProviderTimeMeasurement();
    StatProviderTimeMeasurement(const StatProviderTimeMeasurement&);
    StatProviderTimeMeasurement& operator=(const StatProviderTimeMeasurement&);

    CIMMessage* _message;
    Uint64 _startTimeMicroseconds;
};

class PEGASUS_COMMON_LINKAGE StatisticalData
{
public:
    enum StatRequestType
    {
        GET_CLASS,
        GET_INSTANCE,
        INDICATION_DELIVERY,
        DELETE_CLASS,
        DELETE_INSTANCE,
        CREATE_CLASS,
        CREATE_INSTANCE,
        MODIFY_CLASS,
        MODIFY_INSTANCE,
        ENUMERATE_CLASSES,
        ENUMERATE_CLASS_NAMES,
        ENUMERATE_INSTANCES,
        ENUMERATE_INSTANCE_NAMES,
        EXEC_QUERY,
        ASSOCIATORS,
        ASSOCIATOR_NAMES,
        REFERENCES,
        REFERENCE_NAMES,
        GET_PROPERTY,
        SET_PROPERTY,
        GET_QUALIFIER,
        SET_QUALIFIER,
        DELETE_QUALIFIER,
        ENUMERATE_QUALIFIERS,
        INVOKE_METHOD,
        NUMBER_OF_TYPES
    };

    enum StatDataType
    {
        PEGASUS_STATDATA_SERVER,
        PEGASUS_STATDATA_PROVIDER,
        PEGASUS_STATDATA_BYTES_SENT,
        PEGASUS_STATDATA_BYTES_READ
    };

    static const Uint32 length;
    static StatisticalData* current();

    StatisticalData();

    timeval timestamp;

    Sint64 numCalls[NUMBER_OF_TYPES];
    Sint64 cimomTime[NUMBER_OF_TYPES];
    Sint64 providerTime[NUMBER_OF_TYPES];
    Sint64 responseSize[NUMBER_OF_TYPES];
    Sint64 requestSize[NUMBER_OF_TYPES];
    Sint64 requSize;    //tempory storage for requestSize vlaue
    Boolean copyGSD;
    static StatisticalData* cur;
    void addToValue(Sint64 value, Uint16 type, Uint32 t);
    static String requestName[];
    void setCopyGSD(Boolean flag);

protected:
    Mutex _mutex;
};


PEGASUS_NAMESPACE_END
#endif

