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

#ifndef Pegasus_WsmResponse_h
#define Pegasus_WsmResponse_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Common/ContentLanguageList.h>
#include <Pegasus/WsmServer/WsmRequest.h>
#include <Pegasus/WsmServer/WsmFault.h>
#include <Pegasus/WsmServer/WsmInstance.h>
#include <Pegasus/WsmServer/WsmUtils.h>

PEGASUS_NAMESPACE_BEGIN

class WsmResponse
{
protected:

    WsmResponse(
        WsmOperationType type,
        const WsmRequest* request,
        const ContentLanguageList& contentLanguages)
        : _type(type),
          _messageId(WsmUtils::getMessageId()),
          _relatesTo(request->messageId),
          _queueId(request->queueId),
          _httpMethod(request->httpMethod),
          _httpCloseConnect(request->httpCloseConnect),
          _omitXMLProcessingInstruction(request->omitXMLProcessingInstruction),
          _maxEnvelopeSize(request->maxEnvelopeSize),
          _contentLanguages(contentLanguages)
    {
    }

    WsmResponse(
        WsmOperationType type,
        const String& relatesTo,
        Uint32 queueId,
        HttpMethod httpMethod,
        Boolean httpCloseConnect,
        Boolean omitXMLProcessingInstruction,
        const ContentLanguageList& contentLanguages)
        : _type(type),
          _messageId(WsmUtils::getMessageId()),
          _relatesTo(relatesTo),
          _queueId(queueId),
          _httpMethod(httpMethod),
          _httpCloseConnect(httpCloseConnect),
          _omitXMLProcessingInstruction(omitXMLProcessingInstruction),
          _maxEnvelopeSize(0),
          _contentLanguages(contentLanguages)
    {
    }

public:

    virtual ~WsmResponse()
    {
    }

    WsmOperationType getType() const
    {
        return _type;
    }

    String& getMessageId()
    {
        return _messageId;
    }

    String& getRelatesTo()
    {
        return _relatesTo;
    }

    Uint32 getQueueId() const
    {
        return _queueId;
    }

    HttpMethod getHttpMethod() const
    {
        return _httpMethod;
    }

    Boolean getHttpCloseConnect() const
    {
        return _httpCloseConnect;
    }

    Uint32 getMaxEnvelopeSize() const
    {
        return _maxEnvelopeSize;
    }

    ContentLanguageList& getContentLanguages()
    {
        return _contentLanguages;
    }

    Boolean getOmitXMLProcessingInstruction() const
    {
        return _omitXMLProcessingInstruction;
    }

private:

    WsmResponse(const WsmResponse&);
    WsmResponse& operator=(const WsmResponse&);

    WsmOperationType _type;
    String _messageId;
    String _relatesTo;
    Uint32 _queueId;
    HttpMethod _httpMethod;
    Boolean _httpCloseConnect;
    Boolean _omitXMLProcessingInstruction;
    Uint32 _maxEnvelopeSize;
    ContentLanguageList _contentLanguages;
};

class WsmFaultResponse : public WsmResponse
{
public:

    WsmFaultResponse(
        const String& relatesTo,
        Uint32 queueId,
        HttpMethod httpMethod,
        Boolean httpCloseConnect,
        Boolean omitXMLProcessingInstruction,
        const WsmFault& fault)
        : WsmResponse(
              WSM_FAULT,
              relatesTo,
              queueId,
              httpMethod,
              httpCloseConnect,
              omitXMLProcessingInstruction,
              fault.getReasonLanguage()),
          _fault(fault)
    {
    }

    WsmFaultResponse(
        const WsmRequest* request,
        const WsmFault& fault)
        : WsmResponse(
              WSM_FAULT,
              request,
              fault.getReasonLanguage()),
          _fault(fault)
    {
    }

    ~WsmFaultResponse()
    {
    }

    WsmFault& getFault()
    {
        return _fault;
    }

private:

    WsmFault _fault;
};

class SoapFaultResponse : public WsmResponse
{
public:

    SoapFaultResponse(
        const String& relatesTo,
        Uint32 queueId,
        HttpMethod httpMethod,
        Boolean httpCloseConnect,
        Boolean omitXMLProcessingInstruction,
        const SoapNotUnderstoodFault& fault)
        : WsmResponse(
              SOAP_FAULT,
              relatesTo,
              queueId,
              httpMethod,
              httpCloseConnect,
              omitXMLProcessingInstruction,
              fault.getMessageLanguage()),
          _fault(fault)
    {
    }

    ~SoapFaultResponse()
    {
    }

    SoapNotUnderstoodFault& getFault()
    {
        return _fault;
    }

private:

    SoapNotUnderstoodFault _fault;
};

class WxfGetResponse : public WsmResponse
{
public:

    WxfGetResponse(
        const WsmInstance& inst,
        const WxfGetRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_TRANSFER_GET,
              request,
              contentLanguages),
          _instance(inst),
          _resourceUri(request->epr.resourceUri)
    {
    }

    ~WxfGetResponse()
    {
    }

    WsmInstance& getInstance()
    {
        return _instance;
    }

    const String& getResourceUri() const
    {
        return _resourceUri;
    }

private:

    WsmInstance _instance;
    String _resourceUri;
};

class WxfPutResponse : public WsmResponse
{
public:

    WxfPutResponse(
        const WxfPutRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_TRANSFER_PUT,
              request,
              contentLanguages),
          _reference(request->epr),
          _requestedEPR(request->requestEpr)
    {
    }

    ~WxfPutResponse()
    {
    }

    WsmEndpointReference& getEPR()
    {
        return _reference;
    }

    Boolean getRequestedEPR()
    {
        return _requestedEPR;
    }

private:

    // The client can request the potentially updated EPR by specifying the
    // wsman:RequestEPR header.  CIM does not allow a ModifyInstance operation
    // to change key values, though, so this will always be the same as the
    // EPR in the request.
    WsmEndpointReference _reference;
    Boolean _requestedEPR;
    String _resourceUri;
};

class WxfCreateResponse : public WsmResponse
{
public:

    WxfCreateResponse(
        const WsmEndpointReference& ref,
        const WxfCreateRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_TRANSFER_CREATE,
              request,
              contentLanguages),
          _reference(ref)
    {
    }

    ~WxfCreateResponse()
    {
    }

    WsmEndpointReference& getEPR()
    {
        return _reference;
    }

private:

    WsmEndpointReference _reference;
};

class WxfDeleteResponse : public WsmResponse
{
public:

    WxfDeleteResponse(
        const WxfDeleteRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_TRANSFER_DELETE,
              request,
              contentLanguages)
    {
    }

    ~WxfDeleteResponse()
    {
    }
};

class WsenEnumerationData
{
public:

    WsenEnumerationData()
        : enumerationMode(WSEN_EM_UNKNOWN)
    {
    }
    WsenEnumerationData(const Array<WsmInstance>& inst,
        const Array<WsmEndpointReference> EPRs,
        WsmbPolymorphismMode pm, const String& uri)
        : instances(inst),
          eprs(EPRs),
          enumerationMode(WSEN_EM_OBJECT_AND_EPR),
          polymorphismMode(pm),
          classUri(uri)
    {
    }
    WsenEnumerationData(const Array<WsmInstance>& inst,
        WsmbPolymorphismMode pm, const String& uri)
        : instances(inst),
          enumerationMode(WSEN_EM_OBJECT),
          polymorphismMode(pm),
          classUri(uri)
    {
    }
    WsenEnumerationData(const Array<WsmEndpointReference>& epr)
        : eprs(epr),
          enumerationMode(WSEN_EM_EPR),
          polymorphismMode(WSMB_PM_UNKNOWN)
    {
    }
    WsenEnumerationData(const WsenEnumerationData& data)
        : instances(data.instances),
          eprs(data.eprs),
          enumerationMode(data.enumerationMode),
          polymorphismMode(data.polymorphismMode),
          classUri(data.classUri)
    {
    }
    Uint32 getSize()
    {
        if (enumerationMode == WSEN_EM_OBJECT ||
            enumerationMode == WSEN_EM_OBJECT_AND_EPR)
        {
            return instances.size();
        }
        else if (enumerationMode == WSEN_EM_EPR)
        {
            return eprs.size();
        }
        else
        {
            PEGASUS_ASSERT(0);
            return 0;
        }
    }
    void remove(Uint32 index, Uint32 size)
    {
        if (enumerationMode == WSEN_EM_OBJECT)
        {
            instances.remove(index, size);
        }
        else if (enumerationMode == WSEN_EM_EPR)
        {
            eprs.remove(index, size);
        }
        else if (enumerationMode == WSEN_EM_OBJECT_AND_EPR)
        {
            instances.remove(index, size);
            eprs.remove(index, size);
        }
        else
        {
            PEGASUS_ASSERT(0);
        }
    }
    void merge(const WsenEnumerationData& data)
    {
        if (enumerationMode == WSEN_EM_OBJECT)
        {
            instances.appendArray(data.instances);
        }
        else if (enumerationMode == WSEN_EM_EPR)
        {
            eprs.appendArray(data.eprs);
        }
        else if (enumerationMode == WSEN_EM_OBJECT_AND_EPR)
        {
            instances.appendArray(data.instances);
            eprs.appendArray(data.eprs);
        }
        else
        {
            PEGASUS_ASSERT(0);
        }
    }
    void split(WsenEnumerationData& data, Uint32 num)
    {
        data.enumerationMode = enumerationMode;
        data.polymorphismMode = polymorphismMode;
        data.classUri = classUri;

        if (enumerationMode == WSEN_EM_OBJECT)
        {
            Uint32 i;
            for (i = 0; i < num && i < instances.size(); i++)
            {
                data.instances.append(instances[i]);
            }

            if (i != 0)
            {
                instances.remove(0, i);
            }
        }
        else if (enumerationMode == WSEN_EM_EPR)
        {
            Uint32 i;
            for (i = 0; i < num && i < eprs.size(); i++)
            {
                data.eprs.append(eprs[i]);
            }
            if (i != 0)
            {
                eprs.remove(0, i);
            }
        }
        else if (enumerationMode == WSEN_EM_OBJECT_AND_EPR)
        {
            Uint32 i;
            for (i = 0; i < num && i < instances.size(); i++)
            {
                data.instances.append(instances[i]);
                data.eprs.append(eprs[i]);
            }

            if (i != 0)
            {
                instances.remove(0, i);
                eprs.remove(0, i);
            }
        }
        else
        {
            PEGASUS_ASSERT(0);
        }
    }

    Array<WsmInstance> instances;
    Array<WsmEndpointReference> eprs;
    WsenEnumerationMode enumerationMode;
    WsmbPolymorphismMode polymorphismMode;
    String classUri;
};

class WsenPullResponse : public WsmResponse
{
public:

    WsenPullResponse(
        const WsenEnumerationData& data,
        const WsenPullRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_ENUMERATION_PULL,
              request,
              contentLanguages),
          _enumerationContext((Uint64) -1),
          _isComplete(false),
          _enumerationData(data),
          _resourceUri(request->epr.resourceUri)
    {
    }
    ~WsenPullResponse()
    {
    }
    Array<WsmInstance>& getInstances()
    {
        return _enumerationData.instances;
    }
    Array<WsmEndpointReference>& getEPRs()
    {
        return _enumerationData.eprs;
    }
    WsenEnumerationData& getEnumerationData()
    {
        return _enumerationData;
    }
    Uint32 getSize()
    {
        return _enumerationData.getSize();
    }
    void remove(Uint32 index, Uint32 size)
    {
        _enumerationData.remove(index, size);
    }
    void setComplete()
    {
        _isComplete = true;
    }
    Boolean isComplete()
    {
        return _isComplete;
    }
    WsenEnumerationMode getEnumerationMode()
    {
        return _enumerationData.enumerationMode;
    }
    Uint64 getEnumerationContext()
    {
        return _enumerationContext;
    }
    void setEnumerationContext(Uint64 context)
    {
        _enumerationContext = context;
    }

    const String& getResourceUri() const
    {
        return _resourceUri;
    }

private:

    Uint64 _enumerationContext;
    Boolean _isComplete;
    WsenEnumerationData _enumerationData;
    String _resourceUri;
};

class WsenEnumerateResponse : public WsmResponse
{
public:

    WsenEnumerateResponse(
        const Array<WsmInstance>& inst,
        const Array<WsmEndpointReference>& epr,
        Uint32 itemCount,
        const WsenEnumerateRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_ENUMERATION_ENUMERATE,
              request,
              contentLanguages),
          _enumerationContext((Uint64) -1),
          _isComplete(false),
          _requestItemCount(request->requestItemCount),
          _itemCount(itemCount),
          _enumerationData(inst, epr, request->polymorphismMode,
              request->epr.resourceUri),
          _resourceUri(request->epr.resourceUri)
    {
        PEGASUS_ASSERT(request->enumerationMode == WSEN_EM_OBJECT ||
            request->enumerationMode == WSEN_EM_OBJECT_AND_EPR);
    }
    WsenEnumerateResponse(
        const Array<WsmInstance>& inst,
        Uint32 itemCount,
        const WsenEnumerateRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_ENUMERATION_ENUMERATE,
              request,
              contentLanguages),
          _enumerationContext((Uint64) -1),
          _isComplete(false),
          _requestItemCount(request->requestItemCount),
          _itemCount(itemCount),
          _enumerationData(inst, request->polymorphismMode,
              request->epr.resourceUri),
          _resourceUri(request->epr.resourceUri)
    {
        PEGASUS_ASSERT(request->enumerationMode == WSEN_EM_OBJECT);
    }
    WsenEnumerateResponse(
        const Array<WsmEndpointReference>& epr,
        Uint32 itemCount,
        const WsenEnumerateRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_ENUMERATION_ENUMERATE,
              request,
              contentLanguages),
          _enumerationContext((Uint64) -1),
          _isComplete(false),
          _requestItemCount(request->requestItemCount),
          _itemCount(itemCount),
          _enumerationData(epr),
          _resourceUri(request->epr.resourceUri)
    {
        PEGASUS_ASSERT(request->enumerationMode == WSEN_EM_EPR);
    }
    WsenEnumerateResponse(
        const WsenEnumerationData data,
        Uint32 itemCount,
        const WsenEnumerateRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_ENUMERATION_ENUMERATE,
              request,
              contentLanguages),
          _enumerationContext((Uint64) -1),
          _isComplete(false),
          _requestItemCount(request->requestItemCount),
          _itemCount(itemCount),
          _enumerationData(data),
          _resourceUri(request->epr.resourceUri)
    {
        PEGASUS_ASSERT(request->enumerationMode == data.enumerationMode);
    }
    ~WsenEnumerateResponse()
    {
    }
    Array<WsmInstance>& getInstances()
    {
        return _enumerationData.instances;
    }
    Array<WsmEndpointReference>& getEPRs()
    {
        return _enumerationData.eprs;
    }
    WsenEnumerationData& getEnumerationData()
    {
        return _enumerationData;
    }
    Uint32 getSize()
    {
        return _enumerationData.getSize();
    }
    void merge(WsenEnumerateResponse* response)
    {
        PEGASUS_ASSERT(_enumerationData.enumerationMode ==
            response->_enumerationData.enumerationMode);
        _enumerationData.merge(response->getEnumerationData());
    }
    void merge(WsenPullResponse* response)
    {
        PEGASUS_ASSERT(_enumerationData.enumerationMode ==
            response->getEnumerationData().enumerationMode);
        _enumerationData.merge(response->getEnumerationData());
    }
    void remove(Uint32 index, Uint32 size)
    {
        _enumerationData.remove(index, size);
    }
    void setComplete()
    {
        _isComplete = true;
    }
    Boolean isComplete()
    {
        return _isComplete;
    }
    Boolean requestedItemCount()
    {
        return _requestItemCount;
    }
    void setItemCount(Uint32 count)
    {
        _itemCount = count;
    }
    Uint32 getItemCount()
    {
        return _itemCount;
    }
    WsenEnumerationMode getEnumerationMode()
    {
        return _enumerationData.enumerationMode;
    }
    Uint64 getEnumerationContext()
    {
        return _enumerationContext;
    }
    void setEnumerationContext(Uint64 context)
    {
        _enumerationContext = context;
    }

    const String& getResourceUri() const
    {
        return _resourceUri;
    }

private:

    Uint64 _enumerationContext;
    Boolean _isComplete;
    Boolean _requestItemCount;
    Uint32 _itemCount;
    WsenEnumerationData _enumerationData;
    String _resourceUri;
};

class WsenReleaseResponse : public WsmResponse
{
public:

    WsenReleaseResponse(
        const WsenReleaseRequest* request,
        const ContentLanguageList& contentLanguages)
        : WsmResponse(
              WS_ENUMERATION_RELEASE,
              request,
              contentLanguages)
    {
    }

    ~WsenReleaseResponse()
    {
    }
};

class WsInvokeResponse : public WsmResponse
{
public:

    WsInvokeResponse(
        const String& nameSpace_,
        const String& className_,
        const String& methodName_,
        const WsmInstance& instance_,
        const WsInvokeRequest* request_,
        const ContentLanguageList& contentLanguages_)
        :
        WsmResponse(WS_INVOKE, request_, contentLanguages_),
        nameSpace(nameSpace_),
        className(className_),
        methodName(methodName_),
        instance(instance_),
        resourceUri(request_->epr.resourceUri)
    {
    }

    String nameSpace;
    String className;
    String methodName;
    WsmInstance instance;
    String resourceUri;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_WsmResponse_h */
