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

#include "MofWriter.h"
#include "CIMError.h"
#include "PropertyAccessor.h"

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

// Required property list.

static const char* _requiredProperties[] =
{
    "OwningEntity",
    "MessageID",
    "Message",
    "PerceivedSeverity",
    "ProbableCause",
    "CIMStatusCode",
};

static const size_t _numRequiredProperties =
    sizeof(_requiredProperties) / sizeof(_requiredProperties[0]);

/*
    Uint16 ErrorType;
    String OtherErrorType;
    String OwningEntity;
    String MessageID;
    String Message;
    Array<String> MessageArguments;
    Uint16 PerceivedSeverity;
    Uint16 ProbableCause;
    String ProbableCauseDescription;
    Array<String> RecommendedActions;
    String ErrorSource;
    Uint16 ErrorSourceFormat;
    String OtherErrorSourceFormat;
    Uint32 CIMStatusCode;
    String CIMStatusCodeDescription;
*/

struct TableEntry
{
    int enumTag;
    const char* keyword;
};

static TableEntry _ErrorTypeTable[] =
{
    { 0, "Unknown" },
    { 1, "Other" },
    { 2, "Communications Error" },
    { 3, "Quality of Service Error" },
    { 4, "Software Error" },
    { 5, "Hardware Error" },
    { 6, "Environmental Error" },
    { 7, "Security Error" },
    { 8, "Oversubscription Error" },
    { 9, "Unavailable Resource Error" },
    { 10, "Unsupported Operation Error" }
};

static TableEntry _PerceivedSeverityTable[] =
{
    { 0, "Unknown" },
    { 1, "Unused 1" },
    { 2, "Low" },
    { 3, "Medium" },
    { 4, "High" },
    { 5, "Fatal" }
};

static TableEntry _ProbableCauseTable[] =
{
    { 0, "Unknown" },
    { 1, "Other" },
    { 2, "Adapter/Card Error" },
    { 3, "Application Subsystem Failure" },
    { 4, "Bandwidth Reduced" },
    { 5, "Connection Establishment Error" },
    { 6, "Communications Protocol Error" },
    { 7, "Communications Subsystem Failure" },
    { 8, "Configuration/Customization Error" },
    { 9, "Congestion" },
    { 10, "Corrupt Data" },
    { 11, "CPU Cycles Limit Exceeded" },
    { 12, "Dataset/Modem Error" },
    { 13, "Degraded Signal" },
    { 14, "DTE-DCE Interface Error" },
    { 15, "Enclosure Door Open" },
    { 16, "Equipment Malfunction" },
    { 17, "Excessive Vibration" },
    { 18, "File Format Error" },
    { 19, "Fire Detected" },
    { 20, "Flood Detected" },
    { 21, "Framing Error" },
    { 22, "HVAC Problem" },
    { 23, "Humidity Unacceptable" },
    { 24, "I/O Device Error" },
    { 25, "Input Device Error" },
    { 26, "LAN Error" },
    { 27, "Non-Toxic Leak Detected" },
    { 28, "Local Node Transmission Error" },
    { 29, "Loss of Frame" },
    { 30, "Loss of Signal" },
    { 31, "Material Supply Exhausted" },
    { 32, "Multiplexer Problem" },
    { 33, "Out of Memory" },
    { 34, "Output Device Error" },
    { 35, "Performance Degraded" },
    { 36, "Power Problem" },
    { 37, "Pressure Unacceptable" },
    { 38, "Processor Problem (Internal Machine Error)" },
    { 39, "Pump Failure" },
    { 40, "Queue Size Exceeded" },
    { 41, "Receive Failure" },
    { 42, "Receiver Failure" },
    { 43, "Remote Node Transmission Error" },
    { 44, "Resource at or Nearing Capacity" },
    { 45, "Response Time Excessive" },
    { 46, "Retransmission Rate Excessive" },
    { 47, "Software Error" },
    { 48, "Software Program Abnormally Terminated" },
    { 49, "Software Program Error (Incorrect Results)" },
    { 50, "Storage Capacity Problem" },
    { 51, "Temperature Unacceptable" },
    { 52, "Threshold Crossed" },
    { 53, "Timing Problem" },
    { 54, "Toxic Leak Detected" },
    { 55, "Transmit Failure" },
    { 56, "Transmitter Failure" },
    { 57, "Underlying Resource Unavailable" },
    { 58, "Version Mismatch" },
    { 59, "Previous Alert Cleared" },
    { 60, "Login Attempts Failed" },
    { 61, "Software Virus Detected" },
    { 62, "Hardware Security Breached" },
    { 63, "Denial of Service Detected" },
    { 64, "Security Credential Mismatch" },
    { 65, "Unauthorized Access" },
    { 66, "Alarm Received" },
    { 67, "Loss of Pointer" },
    { 68, "Payload Mismatch" },
    { 69, "Transmission Error" },
    { 70, "Excessive Error Rate" },
    { 71, "Trace Problem" },
    { 72, "Element Unavailable" },
    { 73, "Element Missing" },
    { 74, "Loss of Multi Frame" },
    { 75, "Broadcast Channel Failure" },
    { 76, "Invalid Message Received" },
    { 77, "Routing Failure" },
    { 78, "Backplane Failure" },
    { 79, "Identifier Duplication" },
    { 80, "Protection Path Failure" },
    { 81, "Sync Loss or Mismatch" },
    { 82, "Terminal Problem" },
    { 83, "Real Time Clock Failure" },
    { 84, "Antenna Failure" },
    { 85, "Battery Charging Failure" },
    { 86, "Disk Failure" },
    { 87, "Frequency Hopping Failure" },
    { 88, "Loss of Redundancy" },
    { 89, "Power Supply Failure" },
    { 90, "Signal Quality Problem" },
    { 91, "Battery Discharging" },
    { 92, "Battery Failure" },
    { 93, "Commercial Power Problem" },
    { 94, "Fan Failure" },
    { 95, "Engine Failure" },
    { 96, "Sensor Failure" },
    { 97, "Fuse Failure" },
    { 98, "Generator Failure" },
    { 99, "Low Battery" },
    { 100, "Low Fuel" },
    { 101, "Low Water" },
    { 102, "Explosive Gas" },
    { 103, "High Winds" },
    { 104, "Ice Buildup" },
    { 105, "Smoke" },
    { 106, "Memory Mismatch" },
    { 107, "Out of CPU Cycles" },
    { 108, "Software Environment Problem" },
    { 109, "Software Download Failure" },
    { 110, "Element Reinitialized" },
    { 111, "Timeout" },
    { 112, "Logging Problems" },
    { 113, "Leak Detected" },
    { 114, "Protection Mechanism Failure" },
    { 115, "Protecting Resource Failure" },
    { 116, "Database Inconsistency" },
    { 117, "Authentication Failure" },
    { 118, "Breach of Confidentiality" },
    { 119, "Cable Tamper" },
    { 120, "Delayed Information" },
    { 121, "Duplicate Information" },
    { 122, "Information Missing" },
    { 123, "Information Modification" },
    { 124, "Information Out of Sequence" },
    { 125, "Key Expired" },
    { 126, "Non-Repudiation Failure" },
    { 127, "Out of Hours Activity" },
    { 128, "Out of Service" },
    { 129, "Procedural Error" },
    { 130, "Unexpected Information" },
};

static TableEntry _ErrorSourceFormatTable[] =
{
    { 0, "Unknown" },
    { 1, "Other" },
    { 2, "CIMObjectHandle" },
};

static TableEntry _CIMStatusCodeTable[] =
{
    { 1, "CIM_ERR_FAILED" },
    { 2, "CIM_ERR_ACCESS_DENIED" },
    { 3, "CIM_ERR_INVALID_NAMESPACE" },
    { 4, "CIM_ERR_INVALID_PARAMETER" },
    { 5, "CIM_ERR_INVALID_CLASS" },
    { 6, "CIM_ERR_NOT_FOUND" },
    { 7, "CIM_ERR_NOT_SUPPORTED" },
    { 8, "CIM_ERR_CLASS_HAS_CHILDREN" },
    { 9, "CIM_ERR_CLASS_HAS_INSTANCES" },
    { 10, "CIM_ERR_INVALID_SUPERCLASS" },
    { 11, "CIM_ERR_ALREADY_EXISTS" },
    { 12, "CIM_ERR_NO_SUCH_PROPERTY" },
    { 13, "CIM_ERR_TYPE_MISMATCH" },
    { 14, "CIM_ERR_QUERY_LANGUAGE_NOT_SUPPORTED" },
    { 15, "CIM_ERR_INVALID_QUERY" },
    { 16, "CIM_ERR_METHOD_NOT_AVAILABLE" },
    { 17, "CIM_ERR_METHOD_NOT_FOUND" },
    { 18, "CIM_ERR_UNEXPECTED_RESPONSE" },
    { 19, "CIM_ERR_INVALID_RESPONSE_DESTINATION" },
    { 20, "CIM_ERR_NAMESPACE_NOT_EMPTY" },
};

CIMError::CIMError() : _inst("CIM_Error")
{
    _inst.addProperty(CIMProperty(
        "ErrorType", CIMValue(CIMTYPE_UINT16, false)));
    _inst.addProperty(CIMProperty(
        "OtherErrorType", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "OwningEntity", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "MessageID", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "Message", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "MessageArguments", CIMValue(CIMTYPE_STRING, true)));
    _inst.addProperty(CIMProperty(
        "PerceivedSeverity", CIMValue(CIMTYPE_UINT16, false)));
    _inst.addProperty(CIMProperty(
        "ProbableCause", CIMValue(CIMTYPE_UINT16, false)));
    _inst.addProperty(CIMProperty(
        "ProbableCauseDescription", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "RecommendedActions", CIMValue(CIMTYPE_STRING, true)));
    _inst.addProperty(CIMProperty(
        "ErrorSource", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "ErrorSourceFormat", CIMValue(CIMTYPE_UINT16, false)));
    _inst.addProperty(CIMProperty(
        "OtherErrorSourceFormat", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "CIMStatusCode", CIMValue(CIMTYPE_UINT32, false)));
    _inst.addProperty(CIMProperty(
        "CIMStatusCodeDescription", CIMValue(CIMTYPE_STRING, false)));
}

CIMError::CIMError(const String& owningEntity,
                   const String& messageID,
                   const String& message,
                   const PerceivedSeverityEnum& perceivedSeverity,
                   const ProbableCauseEnum& probableCause,
                   const CIMStatusCodeEnum& cimStatusCode)
: _inst("CIM_Error")
{
    _inst.addProperty(CIMProperty(
        "ErrorType", CIMValue(CIMTYPE_UINT16, false)));
    _inst.addProperty(CIMProperty(
        "OtherErrorType", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "OwningEntity", CIMValue(owningEntity)));
    _inst.addProperty(CIMProperty(
        "MessageID", CIMValue(messageID)));
    _inst.addProperty(CIMProperty(
        "Message", CIMValue(message)));
    _inst.addProperty(CIMProperty(
        "MessageArguments", CIMValue(CIMTYPE_STRING, true)));
    _inst.addProperty(CIMProperty(
        "PerceivedSeverity", CIMValue(Uint16(perceivedSeverity))));
    _inst.addProperty(CIMProperty(
        "ProbableCause", CIMValue(Uint16(probableCause))));
    _inst.addProperty(CIMProperty(
        "ProbableCauseDescription", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "RecommendedActions", CIMValue(CIMTYPE_STRING, true)));
    _inst.addProperty(CIMProperty(
        "ErrorSource", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "ErrorSourceFormat", CIMValue(CIMTYPE_UINT16, false)));
    _inst.addProperty(CIMProperty(
        "OtherErrorSourceFormat", CIMValue(CIMTYPE_STRING, false)));
    _inst.addProperty(CIMProperty(
        "CIMStatusCode", CIMValue(Uint32(cimStatusCode))));
    _inst.addProperty(CIMProperty(
        "CIMStatusCodeDescription", CIMValue(CIMTYPE_STRING, false)));
}

CIMError::CIMError(const CIMError& x) : _inst(x._inst)
{
}

CIMError::~CIMError()
{
}

bool CIMError::getErrorType(ErrorTypeEnum& value) const
{
    Uint16 t;
    bool nullStat = Get(_inst, "ErrorType", t);
    value = ErrorTypeEnum(t);
    return nullStat;
}

void CIMError::setErrorType(ErrorTypeEnum value, bool null)
{
    Set(_inst, "ErrorType", Uint16(value), null);
}

bool CIMError::getOtherErrorType(String& value) const
{
    return Get(_inst, "OtherErrorType", value);
}

void CIMError::setOtherErrorType(const String& value, bool null)
{
    Set(_inst, "OtherErrorType", value, null);
}

bool CIMError::getOwningEntity(String& value) const
{
    return Get(_inst, "OwningEntity", value);
}

void CIMError::setOwningEntity(const String& value, bool null)
{
    Set(_inst, "OwningEntity", value, null);
}

bool CIMError::getMessageID(String& value) const
{
   return Get(_inst, "MessageID", value);
}

void CIMError::setMessageID(const String& value, bool null)
{
    Set(_inst, "MessageID", value, null);
}

bool CIMError::getMessage(String& value) const
{
    return Get(_inst, "Message", value);
}

void CIMError::setMessage(const String& value, bool null)
{
    Set(_inst, "Message", value, null);
}

bool CIMError::getMessageArguments(Array<String>&  value) const
{
    return Get(_inst, "MessageArguments", value);
}

void CIMError::setMessageArguments(const Array<String>& value, bool null)
{
    Set(_inst, "MessageArguments", value, null);
}

bool CIMError::getPerceivedSeverity(
    PerceivedSeverityEnum& value) const
{
    Uint16 t;
    bool nullStat = Get(_inst, "PerceivedSeverity", t);
    value = PerceivedSeverityEnum(t);
    return nullStat;
}

void CIMError::setPerceivedSeverity(
    PerceivedSeverityEnum value, bool null)
{
    Set(_inst, "PerceivedSeverity", Uint16(value), null);
}

bool CIMError::getProbableCause(ProbableCauseEnum& value) const
{
    Uint16 t;
    bool nullStat = Get(_inst, "ProbableCause", t);
    value = ProbableCauseEnum(t);
    return nullStat;
}

void CIMError::setProbableCause(ProbableCauseEnum value, bool null)
{
    Set(_inst, "ProbableCause", (Uint16)value, null);
}

bool CIMError::getProbableCauseDescription(String& value) const
{
    return Get(_inst, "ProbableCauseDescription", value);
}

void CIMError::setProbableCauseDescription(const String& value, bool null)
{
    Set(_inst, "ProbableCauseDescription", value, null);
}

bool CIMError::getRecommendedActions(Array<String>& value) const
{
    return Get(_inst, "RecommendedActions", value);
}

void CIMError::setRecommendedActions(const Array<String>& value, bool null)
{
    Set(_inst, "RecommendedActions", value, null);
}

bool CIMError::getErrorSource(String& value) const
{
    return Get(_inst, "ErrorSource", value);
}

void CIMError::setErrorSource(const String& value, bool null)
{
    Set(_inst, "ErrorSource", value, null);
}

bool CIMError::getErrorSourceFormat(
    ErrorSourceFormatEnum& value) const
{
    Uint16 t;
    bool nullStat = Get(_inst, "ErrorSourceFormat", t);
    value = ErrorSourceFormatEnum(t);
    return nullStat;
}

void CIMError::setErrorSourceFormat(ErrorSourceFormatEnum value, bool null)
{
    Set(_inst, "ErrorSourceFormat", Uint16(value), null);
}

bool CIMError::getOtherErrorSourceFormat(String& value) const
{
    return Get(_inst, "OtherErrorSourceFormat", value);
}

void CIMError::setOtherErrorSourceFormat(const String& value, bool null)
{
    Set(_inst, "OtherErrorSourceFormat", value, null);
}

bool CIMError::getCIMStatusCode(CIMStatusCodeEnum& value) const
{
    Uint32 t;
    bool nullStat = Get(_inst, "CIMStatusCode", t);
    value = CIMStatusCodeEnum(t);
    return nullStat;
}

void CIMError::setCIMStatusCode(CIMStatusCodeEnum value, bool null)
{
    Set(_inst, "CIMStatusCode", Uint32(value), null);
}

bool CIMError::getCIMStatusCodeDescription(String& value) const
{
    return Get(_inst, "CIMStatusCodeDescription", value);
}

void CIMError::setCIMStatusCodeDescription(const String& value, bool null)
{
    Set(_inst, "CIMStatusCodeDescription", value, null);
}

const CIMInstance& CIMError::getInstance() const
{
    return _inst;
}

template<class T>
void _Check(const String& name, CIMConstProperty& p, T* tag)
{
    if (p.getName() == name)
    {
        if (IsArray(tag) != p.isArray() || GetType(tag) != p.getType())
            throw CIMException(CIM_ERR_TYPE_MISMATCH, name);
    }
}

void CIMError::setInstance(const CIMInstance& instance)
{
    for (Uint32 i = 0; i < instance.getPropertyCount(); i++)
    {
        CIMConstProperty p = instance.getProperty(i);

        _Check("ErrorType", p, (Uint16*)0);
        _Check("OtherErrorType", p, (String*)0);
        _Check("OwningEntity", p, (String*)0);
        _Check("MessageID", p, (String*)0);
        _Check("Message", p, (String*)0);
        _Check("MessageArguments", p, (Array<String>*)0);
        _Check("PerceivedSeverity", p, (Uint16*)0);
        _Check("ProbableCause", p, (Uint16*)0);
        _Check("ProbableCauseDescription", p, (String*)0);
        _Check("RecommendedActions", p, (Array<String>*)0);
        _Check("ErrorSource", p, (String*)0);
        _Check("ErrorSourceFormat", p, (Uint16*)0);
        _Check("OtherErrorSourceFormat", p, (String*)0);
        _Check("CIMStatusCode", p, (Uint32*)0);
        _Check("CIMStatusCodeDescription", p, (String*)0);
    }

    // Verify that the instance contains all of the required properties.

    for (Uint32 i = 0; i < _numRequiredProperties; i++)
    {
        // Does inst have this property?

        Uint32 pos = instance.findProperty(_requiredProperties[i]);

        if (pos == PEG_NOT_FOUND)
        {
            char buffer[80];
            sprintf(buffer, "required property does not exist: %s",
                _requiredProperties[i]);
            throw CIMException(CIM_ERR_NO_SUCH_PROPERTY, buffer);
        }
        // is required property non-null?
        CIMConstProperty p = instance.getProperty(pos);
        CIMValue v = p.getValue();
        if (v.isNull())
        {
            char buffer[80];
            sprintf(buffer, "required property MUST NOT be Null: %s",
                _requiredProperties[i]);
            throw CIMException(CIM_ERR_FAILED, buffer);
        }
    }
    _inst = instance;
}

void CIMError::print() const
{
    Buffer buf;
    MofWriter::appendInstanceElement(buf, _inst);
    printf("%.*s\n", int(buf.size()), buf.getData());
}

PEGASUS_NAMESPACE_END
