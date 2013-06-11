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

#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Common/StatisticalData.h>
#include "CIMMessage.h"
#include "XmlWriter.h"

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

#define PEGASUS_ARRAY_T ArraySint8
# include "ArrayImpl.h"
#undef PEGASUS_ARRAY_T

void CIMResponseMessage::syncAttributes(const CIMRequestMessage* request)
{
    // Propagate request attributes to the response, as necessary
    setMask(request->getMask());
    setHttpMethod(request->getHttpMethod());
    setCloseConnect(request->getCloseConnect());
#ifndef PEGASUS_DISABLE_PERFINST
    setServerStartTime(request->getServerStartTime());
#endif
    binaryRequest = request->binaryRequest;
    binaryResponse = request->binaryResponse;
}

CIMResponseMessage* CIMGetClassRequestMessage::buildResponse() const
{
    AutoPtr<CIMGetClassResponseMessage> response(
        new CIMGetClassResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            CIMClass()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMGetInstanceRequestMessage::buildResponse() const
{
    AutoPtr<CIMGetInstanceResponseMessage> response(
        new CIMGetInstanceResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMExportIndicationRequestMessage::buildResponse() const
{
    AutoPtr<CIMExportIndicationResponseMessage> response(
        new CIMExportIndicationResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMDeleteClassRequestMessage::buildResponse() const
{
    AutoPtr<CIMDeleteClassResponseMessage> response(
        new CIMDeleteClassResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMDeleteInstanceRequestMessage::buildResponse() const
{
    AutoPtr<CIMDeleteInstanceResponseMessage> response(
        new CIMDeleteInstanceResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMCreateClassRequestMessage::buildResponse() const
{
    AutoPtr<CIMCreateClassResponseMessage> response(
        new CIMCreateClassResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMCreateInstanceRequestMessage::buildResponse() const
{
    AutoPtr<CIMCreateInstanceResponseMessage> response(
        new CIMCreateInstanceResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            CIMObjectPath()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMModifyClassRequestMessage::buildResponse() const
{
    AutoPtr<CIMModifyClassResponseMessage> response(
        new CIMModifyClassResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMModifyInstanceRequestMessage::buildResponse() const
{
    AutoPtr<CIMModifyInstanceResponseMessage> response(
        new CIMModifyInstanceResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMEnumerateClassesRequestMessage::buildResponse() const
{
    AutoPtr<CIMEnumerateClassesResponseMessage> response(
        new CIMEnumerateClassesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            Array<CIMClass>()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMEnumerateClassNamesRequestMessage::buildResponse() const
{
    AutoPtr<CIMEnumerateClassNamesResponseMessage> response(
        new CIMEnumerateClassNamesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            Array<CIMName>()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMEnumerateInstancesRequestMessage::buildResponse() const
{
    AutoPtr<CIMEnumerateInstancesResponseMessage> response(
        new CIMEnumerateInstancesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage*
    CIMEnumerateInstanceNamesRequestMessage::buildResponse() const
{
    AutoPtr<CIMEnumerateInstanceNamesResponseMessage> response(
        new CIMEnumerateInstanceNamesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMExecQueryRequestMessage::buildResponse() const
{
    AutoPtr<CIMExecQueryResponseMessage> response(
        new CIMExecQueryResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMAssociatorsRequestMessage::buildResponse() const
{
    AutoPtr<CIMAssociatorsResponseMessage> response(
        new CIMAssociatorsResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMAssociatorNamesRequestMessage::buildResponse() const
{
    AutoPtr<CIMAssociatorNamesResponseMessage> response(
        new CIMAssociatorNamesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMReferencesRequestMessage::buildResponse() const
{
    AutoPtr<CIMReferencesResponseMessage> response(
        new CIMReferencesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMReferenceNamesRequestMessage::buildResponse() const
{
    AutoPtr<CIMReferenceNamesResponseMessage> response(
        new CIMReferenceNamesResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMGetPropertyRequestMessage::buildResponse() const
{
    AutoPtr<CIMGetPropertyResponseMessage> response(
        new CIMGetPropertyResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            CIMValue()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMSetPropertyRequestMessage::buildResponse() const
{
    AutoPtr<CIMSetPropertyResponseMessage> response(
        new CIMSetPropertyResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMGetQualifierRequestMessage::buildResponse() const
{
    AutoPtr<CIMGetQualifierResponseMessage> response(
        new CIMGetQualifierResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            CIMQualifierDecl()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMSetQualifierRequestMessage::buildResponse() const
{
    AutoPtr<CIMSetQualifierResponseMessage> response(
        new CIMSetQualifierResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMDeleteQualifierRequestMessage::buildResponse() const
{
    AutoPtr<CIMDeleteQualifierResponseMessage> response(
        new CIMDeleteQualifierResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMEnumerateQualifiersRequestMessage::buildResponse() const
{
    AutoPtr<CIMEnumerateQualifiersResponseMessage> response(
        new CIMEnumerateQualifiersResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            Array<CIMQualifierDecl>()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMInvokeMethodRequestMessage::buildResponse() const
{
    AutoPtr<CIMInvokeMethodResponseMessage> response(
        new CIMInvokeMethodResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            CIMValue(),
            Array<CIMParamValue>(),
            methodName));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMProcessIndicationRequestMessage::buildResponse() const
{
    AutoPtr<CIMProcessIndicationResponseMessage> response(
        new CIMProcessIndicationResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage*
    CIMNotifyProviderRegistrationRequestMessage::buildResponse() const
{
    AutoPtr<CIMNotifyProviderRegistrationResponseMessage> response(
        new CIMNotifyProviderRegistrationResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage*
    CIMNotifyProviderTerminationRequestMessage::buildResponse() const
{
    AutoPtr<CIMNotifyProviderTerminationResponseMessage> response(
        new CIMNotifyProviderTerminationResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMHandleIndicationRequestMessage::buildResponse() const
{
    AutoPtr<CIMHandleIndicationResponseMessage> response(
        new CIMHandleIndicationResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMCreateSubscriptionRequestMessage::buildResponse() const
{
    AutoPtr<CIMCreateSubscriptionResponseMessage> response(
        new CIMCreateSubscriptionResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMModifySubscriptionRequestMessage::buildResponse() const
{
    AutoPtr<CIMModifySubscriptionResponseMessage> response(
        new CIMModifySubscriptionResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMDeleteSubscriptionRequestMessage::buildResponse() const
{
    AutoPtr<CIMDeleteSubscriptionResponseMessage> response(
        new CIMDeleteSubscriptionResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage*
    CIMSubscriptionInitCompleteRequestMessage::buildResponse() const
{
    AutoPtr<CIMSubscriptionInitCompleteResponseMessage> response(
        new CIMSubscriptionInitCompleteResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage*
    CIMIndicationServiceDisabledRequestMessage::buildResponse() const
{
    AutoPtr<CIMIndicationServiceDisabledResponseMessage> response(
        new CIMIndicationServiceDisabledResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMDisableModuleRequestMessage::buildResponse() const
{
    AutoPtr<CIMDisableModuleResponseMessage> response(
        new CIMDisableModuleResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            Array<Uint16>()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMEnableModuleRequestMessage::buildResponse() const
{
    AutoPtr<CIMEnableModuleResponseMessage> response(
        new CIMEnableModuleResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            Array<Uint16>()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMNotifyProviderEnableRequestMessage::buildResponse() const
{
    AutoPtr<CIMNotifyProviderEnableResponseMessage> response(
        new CIMNotifyProviderEnableResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMNotifyProviderFailRequestMessage::buildResponse() const
{
    AutoPtr<CIMNotifyProviderFailResponseMessage> response(
        new CIMNotifyProviderFailResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMStopAllProvidersRequestMessage::buildResponse() const
{
    AutoPtr<CIMStopAllProvidersResponseMessage> response(
        new CIMStopAllProvidersResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage*
    CIMInitializeProviderAgentRequestMessage::buildResponse() const
{
    AutoPtr<CIMInitializeProviderAgentResponseMessage> response(
        new CIMInitializeProviderAgentResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* CIMNotifyConfigChangeRequestMessage::buildResponse() const
{
    AutoPtr<CIMNotifyConfigChangeResponseMessage> response(
        new CIMNotifyConfigChangeResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop()));
    response->syncAttributes(this);
    return response.release();
}

CIMResponseMessage* ProvAgtGetScmoClassRequestMessage::buildResponse() const
{
    AutoPtr<ProvAgtGetScmoClassResponseMessage> response(
        new ProvAgtGetScmoClassResponseMessage(
            messageId,
            CIMException(),
            queueIds.copyAndPop(),
            SCMOClass("","")));
    response->syncAttributes(this);
    return response.release();
}

CIMMessage::CIMMessage(
    MessageType type,
    const String& messageId_)
    : Message(type),
      messageId(messageId_),
      _languageContextThreadId(Threads::self())
#ifndef PEGASUS_DISABLE_PERFINST
      ,_serverStartTimeMicroseconds(0),
      _providerTimeMicroseconds(0),
      _totalServerTimeMicroseconds(0)
#endif
{
    operationContext.insert(
        AcceptLanguageListContainer(AcceptLanguageList()));
    operationContext.insert(
        ContentLanguageListContainer(ContentLanguageList()));

    binaryRequest = false;
    binaryResponse = false;
}

#ifndef PEGASUS_DISABLE_PERFINST
void CIMMessage::endServer()
{
    PEGASUS_ASSERT(_serverStartTimeMicroseconds != 0);

    _totalServerTimeMicroseconds =
        TimeValue::getCurrentTime().toMicroseconds() -
            _serverStartTimeMicroseconds;

    Uint64 serverTimeMicroseconds =
        _totalServerTimeMicroseconds - _providerTimeMicroseconds;

    Uint16 statType = (Uint16)((getType() >= CIM_GET_CLASS_RESPONSE_MESSAGE) ?
        getType() - CIM_GET_CLASS_RESPONSE_MESSAGE : getType() - 1);

    StatisticalData::current()->addToValue(serverTimeMicroseconds, statType,
        StatisticalData::PEGASUS_STATDATA_SERVER);

    StatisticalData::current()->addToValue(_providerTimeMicroseconds, statType,
        StatisticalData::PEGASUS_STATDATA_PROVIDER);

    /* This adds the number of bytes read to the total.the request size
       value must be stored (requSize) and passed to the StatisticalData
       object at the end of processingm otherwise it will be the ONLY value
       that is passed to the client which reports the current state of the
       object, not the previous (one command ago) state */

    StatisticalData::current()->addToValue(
        StatisticalData::current()->requSize,
        statType,
        StatisticalData::PEGASUS_STATDATA_BYTES_READ);
}
#endif

CIMRequestMessage::CIMRequestMessage(
    MessageType type_,
    const String& messageId_,
    const QueueIdStack& queueIds_)
    : CIMMessage(type_, messageId_), queueIds(queueIds_)
{
}

CIMResponseMessage::CIMResponseMessage(
    MessageType type_,
    const String& messageId_,
    const CIMException& cimException_,
    const QueueIdStack& queueIds_)
    :
    CIMMessage(type_, messageId_),
    queueIds(queueIds_),
    cimException(cimException_)
{
}

CIMOperationRequestMessage::CIMOperationRequestMessage(
    MessageType type_,
    const String& messageId_,
    const QueueIdStack& queueIds_,
    const String& authType_,
    const String& userName_,
    const CIMNamespaceName& nameSpace_,
    const CIMName& className_,
    Uint32 providerType_)
    :
    CIMRequestMessage(type_, messageId_, queueIds_),
    authType(authType_),
    userName(userName_),
    nameSpace(nameSpace_),
    className(className_),
    providerType(providerType_)
{
}

PEGASUS_NAMESPACE_END
