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

#ifndef Pegasus_CIMResponseData_h
#define Pegasus_CIMResponseData_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/Linkage.h>
#include <Pegasus/Common/CIMBuffer.h>
#include <Pegasus/Common/SCMOClass.h>
#include <Pegasus/Common/SCMOInstance.h>
#include <Pegasus/Common/SCMODump.h>

PEGASUS_NAMESPACE_BEGIN

typedef Array<Sint8> ArraySint8;
#define PEGASUS_ARRAY_T ArraySint8
# include <Pegasus/Common/ArrayInter.h>
#undef PEGASUS_ARRAY_T


class PEGASUS_COMMON_LINKAGE CIMResponseData
{
public:

    enum ResponseDataEncoding {
        RESP_ENC_CIM = 1,
        RESP_ENC_BINARY = 2,
        RESP_ENC_XML = 4,
        RESP_ENC_SCMO = 8
    };

    enum ResponseDataContent {
        RESP_INSTNAMES = 1,
        RESP_INSTANCES = 2,
        RESP_INSTANCE = 3,
        RESP_OBJECTS = 4,
        RESP_OBJECTPATHS = 5
    };

    CIMResponseData(ResponseDataContent content):
        _encoding(0),_dataType(content)
    {};

    CIMResponseData(const CIMResponseData & x):
        _encoding(x._encoding),
        _dataType(x._dataType),
        _referencesData(x._referencesData),
        _instanceData(x._instanceData),
        _hostsData(x._hostsData),
        _nameSpacesData(x._nameSpacesData),
        _binaryData(x._binaryData),
        _defaultNamespace(x._defaultNamespace),
        _defaultHostname(x._defaultHostname),
        _instanceNames(x._instanceNames),
        _instances(x._instances),
        _objects(x._objects),
        _scmoInstances(x._scmoInstances)
    {};

    ~CIMResponseData()
    {
    }

    // C++ objects interface handling

    // Instance Names handling
    Array<CIMObjectPath>& getInstanceNames();

    void setInstanceNames(const Array<CIMObjectPath>& x)
    {
        _instanceNames=x;
        _encoding |= RESP_ENC_CIM;
    }

    // Instance handling
    CIMInstance& getInstance();

    void setInstance(const CIMInstance& x)
    {
        _instances.clear();
        _instances.append(x);
        _encoding |= RESP_ENC_CIM;
    }

    // Instances handling
    Array<CIMInstance>& getInstances();

    void setInstances(const Array<CIMInstance>& x)
    {
        _instances=x;
        _encoding |= RESP_ENC_CIM;
    }
    void appendInstance(const CIMInstance& x)
    {
        _instances.append(x);
        _encoding |= RESP_ENC_CIM;
    }

    // Objects handling
    Array<CIMObject>& getObjects();
    void setObjects(const Array<CIMObject>& x)
    {
        _objects=x;
        _encoding |= RESP_ENC_CIM;
    }
    void appendObject(const CIMObject& x)
    {
        _objects.append(x);
        _encoding |= RESP_ENC_CIM;
    }

    // SCMO representation, single instance stored as one element array
    // object paths are represented as SCMOInstance
    Array<SCMOInstance>& getSCMO();

    void setSCMO(const Array<SCMOInstance>& x);

    void appendSCMO(const Array<SCMOInstance>& x)
    {
        _scmoInstances.appendArray(x);
        _encoding |= RESP_ENC_SCMO;
    }

    // Binary data is just a data stream
    Array<Uint8>& getBinary();
    bool setBinary(CIMBuffer& in, bool hasLen=true);

    // Xml data is unformatted, no need to differentiate between instance
    // instances and object paths or objects
    bool setXml(CIMBuffer& in);

    // function used by OperationAggregator to aggregate response data in a
    // single ResponseData object
    void appendResponseData(const CIMResponseData & x);

    // Function used by CMPI layer to complete the namespace on all data held
    // Input (x) has to have a valid namespace
    void completeNamespace(const SCMOInstance * x);

    // Function primarily used by CIMOperationRequestDispatcher to complete
    // namespace and hostname on a,an,r and rn operations in the
    // OperationAggregator
    void completeHostNameAndNamespace(
        const String & hn,
        const CIMNamespaceName & ns);

    // Encoding responses

    // binary format used with Provider Agents and OP Clients
    void encodeBinaryResponse(CIMBuffer& out);
    // Xml format used with Provider Agents only
    void encodeInternalXmlResponse(CIMBuffer& out);
    // official Xml format(CIM over Http) used to communicate to clients
    void encodeXmlResponse(Buffer& out);

private:

    // helper functions to transform different formats into one-another
    // functions work on the internal data and calling of them should be
    // avoided

    void _resolveToCIM();
    void _resolveToSCMO();

    void _resolveBinary();

    void _resolveXmlToSCMO();
    void _resolveXmlToCIM();

    void _resolveSCMOToCIM();
    void _resolveCIMToSCMO();

    // Helper functions for this class only, do NOT externalize
    SCMOInstance _getSCMOFromCIMInstance(const CIMInstance& cimInst);
    SCMOInstance _getSCMOFromCIMObject(const CIMObject& cimObj);
    SCMOInstance _getSCMOFromCIMObjectPath(const CIMObjectPath& cimPath);
    SCMOClass* _getSCMOClass(const char* ns,const char* cls);

    // Bitflags in this integer will reflect what data representation types
    // are currently stored in this CIMResponseData object
    Uint32 _encoding;

    // Storing type of data in this enumeration
    ResponseDataContent _dataType;

    // unused arrays are represented by ArrayRepBase _empty_rep
    // which is a 16 byte large static shared across all of them
    // so, even though this object looks large, it holds just
    // 2 integer and 9 pointers

    // For XML encoding.
    Array<ArraySint8> _referencesData;
    Array<ArraySint8> _instanceData;
    Array<String> _hostsData;
    Array<CIMNamespaceName> _nameSpacesData;


    // For binary encoding.
    Array<Uint8> _binaryData;
    CIMNamespaceName _defaultNamespace;
    String _defaultHostname;

    // Default C++ encoding
    Array<CIMObjectPath> _instanceNames;
    Array<CIMInstance> _instances;
    Array<CIMObject> _objects;

    // SCMO encoding
    Array<SCMOInstance> _scmoInstances;

};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_CIMResponseData_h */
