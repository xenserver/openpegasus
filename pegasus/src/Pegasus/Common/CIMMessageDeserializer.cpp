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

#include <Pegasus/Common/XmlReader.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Common/System.h>

#include "CIMMessageDeserializer.h"

PEGASUS_NAMESPACE_BEGIN

CIMMessage* CIMMessageDeserializer::deserialize(char* buffer)
{
    if (buffer[0] == 0)
    {
        // No message to deserialize
        return 0;
    }

    XmlParser parser(buffer);
    XmlEntry entry;

    CIMMessage* message = 0;
    String messageID;
    String typeString;
    MessageType type;
    CIMValue genericValue;
#ifndef PEGASUS_DISABLE_PERFINST
    Uint64 serverStartTimeMicroseconds;
    Uint64 providerTimeMicroseconds;
#endif
    Boolean isComplete;
    Uint32 index;
    OperationContext operationContext;

    XmlReader::expectStartTag(parser, entry, "PGMESSAGE");

    Boolean found = entry.getAttributeValue("ID", messageID);
    PEGASUS_ASSERT(found);

    found = entry.getAttributeValue("TYPE", typeString);
    PEGASUS_ASSERT(found);
    type = MessageType(atoi(typeString.getCString()));

#ifndef PEGASUS_DISABLE_PERFINST
    // Deserialize the performance statistics data

    XmlReader::getValueElement(parser, CIMTYPE_UINT64, genericValue);
    genericValue.get(serverStartTimeMicroseconds);
    XmlReader::getValueElement(parser, CIMTYPE_UINT64, genericValue);
    genericValue.get(providerTimeMicroseconds);
#endif

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(isComplete);

    XmlReader::getValueElement(parser, CIMTYPE_UINT32, genericValue);
    genericValue.get(index);

    _deserializeOperationContext(parser, operationContext);

    if (XmlReader::testStartTag(parser, entry, "PGREQ"))
    {
        message = _deserializeCIMRequestMessage(parser, type);
        XmlReader::expectEndTag(parser, "PGREQ");
    }
    else
    {
        // CIMResponseMessage is the only other CIMMessage type defined
        found = XmlReader::testStartTag(parser, entry, "PGRESP");
        PEGASUS_ASSERT(found);
        message = _deserializeCIMResponseMessage(parser, type);
        XmlReader::expectEndTag(parser, "PGRESP");
    }

    XmlReader::expectEndTag(parser, "PGMESSAGE");

    message->messageId = messageID;
#ifndef PEGASUS_DISABLE_PERFINST
    message->setServerStartTime(serverStartTimeMicroseconds);
    message->setProviderTime(providerTimeMicroseconds);
#endif
    message->setComplete(isComplete);
    message->setIndex(index);
    message->operationContext = operationContext;

    return message;
}

//
// _deserializeCIMRequestMessage
//
CIMRequestMessage* CIMMessageDeserializer::_deserializeCIMRequestMessage(
    XmlParser& parser,
    MessageType type)
{
    CIMRequestMessage* message = 0;
    XmlEntry entry;
    QueueIdStack queueIdStack;

    _deserializeQueueIdStack(parser, queueIdStack);

    if (XmlReader::testStartTag(parser, entry, "PGOPREQ"))
    {
        CIMOperationRequestMessage* cimOpReqMessage = 0;
        CIMValue genericValue;
        String authType;
        String userName;
        CIMNamespaceName nameSpace;
        CIMName className;
        Uint32 providerType;

        _deserializeUserInfo(parser, authType, userName);
        _deserializeCIMNamespaceName(parser, nameSpace);
        _deserializeCIMName(parser, className);

        // Decode cimMessage->providerType
        XmlReader::getValueElement(parser, CIMTYPE_UINT32, genericValue);
        genericValue.get(providerType);

        switch (type)
        {
        // A provider cannot implement these operation types, so the
        // serialization of these messages is not implemented.
        //case CIM_GET_CLASS_REQUEST_MESSAGE:
        //case CIM_DELETE_CLASS_REQUEST_MESSAGE:
        //case CIM_CREATE_CLASS_REQUEST_MESSAGE:
        //case CIM_MODIFY_CLASS_REQUEST_MESSAGE:
        //case CIM_ENUMERATE_CLASSES_REQUEST_MESSAGE:
        //case CIM_ENUMERATE_CLASS_NAMES_REQUEST_MESSAGE:
        //case CIM_GET_QUALIFIER_REQUEST_MESSAGE:
        //case CIM_SET_QUALIFIER_REQUEST_MESSAGE:
        //case CIM_DELETE_QUALIFIER_REQUEST_MESSAGE:
        //case CIM_ENUMERATE_QUALIFIERS_REQUEST_MESSAGE:

        // Instance operations
        case CIM_GET_INSTANCE_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMGetInstanceRequestMessage(parser);
            break;
        case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMDeleteInstanceRequestMessage(parser);
            break;
        case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMCreateInstanceRequestMessage(parser);
            break;
        case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMModifyInstanceRequestMessage(parser);
            break;
        case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMEnumerateInstancesRequestMessage(parser);
            break;
        case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMEnumerateInstanceNamesRequestMessage(parser);
            break;
        case CIM_EXEC_QUERY_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMExecQueryRequestMessage(parser);
            break;

        // Property operations
        case CIM_GET_PROPERTY_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMGetPropertyRequestMessage(parser);
            break;
        case CIM_SET_PROPERTY_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMSetPropertyRequestMessage(parser);
            break;

        // Association operations
        case CIM_ASSOCIATORS_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMAssociatorsRequestMessage(parser);
            break;
        case CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMAssociatorNamesRequestMessage(parser);
            break;
        case CIM_REFERENCES_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMReferencesRequestMessage(parser);
            break;
        case CIM_REFERENCE_NAMES_REQUEST_MESSAGE:
            cimOpReqMessage =
                _deserializeCIMReferenceNamesRequestMessage(parser);
            break;

        // Method operations
        case CIM_INVOKE_METHOD_REQUEST_MESSAGE:
            cimOpReqMessage = _deserializeCIMInvokeMethodRequestMessage(parser);
            break;

        default:
            // Unexpected message type
            PEGASUS_ASSERT(0);
            break;
        }
        PEGASUS_ASSERT(cimOpReqMessage != 0);

        XmlReader::expectEndTag(parser, "PGOPREQ");

        cimOpReqMessage->authType = authType;
        cimOpReqMessage->userName = userName;
        cimOpReqMessage->nameSpace = CIMNamespaceName(nameSpace);
        cimOpReqMessage->className = className;
        cimOpReqMessage->providerType = providerType;

        message = cimOpReqMessage;
    }
    else if (XmlReader::testStartTag(parser, entry, "PGINDREQ"))
    {
        String authType;
        String userName;

        _deserializeUserInfo(parser, authType, userName);

        CIMIndicationRequestMessage* cimIndReqMessage = 0;
        switch (type)
        {
        case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
            cimIndReqMessage =
                _deserializeCIMCreateSubscriptionRequestMessage(parser);
            break;
        case CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE:
            cimIndReqMessage =
                _deserializeCIMModifySubscriptionRequestMessage(parser);
            break;
        case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
            cimIndReqMessage =
                _deserializeCIMDeleteSubscriptionRequestMessage(parser);
            break;
        default:
            // Unexpected message type
            PEGASUS_ASSERT(0);
            break;
        }
        PEGASUS_ASSERT(cimIndReqMessage != 0);

        XmlReader::expectEndTag(parser, "PGINDREQ");

        cimIndReqMessage->authType = authType;
        cimIndReqMessage->userName = userName;

        message = cimIndReqMessage;
    }
    else    // Other message types
    {
        XmlReader::expectStartTag(parser, entry, "PGOTHERREQ");

        switch (type)
        {
        case CIM_EXPORT_INDICATION_REQUEST_MESSAGE:
            message = _deserializeCIMExportIndicationRequestMessage(parser);
            break;
        case CIM_PROCESS_INDICATION_REQUEST_MESSAGE:
            message = _deserializeCIMProcessIndicationRequestMessage(parser);
            break;
        //case CIM_NOTIFY_PROVIDER_REGISTRATION_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //message =
            //  _deserializeCIMNotifyProviderRegistrationRequestMessage(parser);
            break;
        //case CIM_NOTIFY_PROVIDER_TERMINATION_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //message =
            //  _deserializeCIMNotifyProviderTerminationRequestMessage(parser);
            break;
        //case CIM_HANDLE_INDICATION_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //message = _deserializeCIMHandleIndicationRequestMessage(parser);
            break;
        case CIM_DISABLE_MODULE_REQUEST_MESSAGE:
            message = _deserializeCIMDisableModuleRequestMessage(parser);
            break;
        case CIM_ENABLE_MODULE_REQUEST_MESSAGE:
            message = _deserializeCIMEnableModuleRequestMessage(parser);
            break;
        //case CIM_NOTIFY_PROVIDER_ENABLE_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //message =
            //    _deserializeCIMNotifyProviderEnableRequestMessage(parser);
            break;
        //case CIM_NOTIFY_PROVIDER_FAIL_REQUEST_MESSAGE:
            // ATTN: No need to deserialize this yet
            //message = _deserializeCIMNotifyProviderFailRequestMessage(parser);
            //break;
        case CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE:
            message = _deserializeCIMStopAllProvidersRequestMessage(parser);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_REQUEST_MESSAGE:
            message =
                _deserializeCIMInitializeProviderAgentRequestMessage(parser);
            break;
        case CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE:
            message = _deserializeCIMNotifyConfigChangeRequestMessage(parser);
            break;
        case CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE:
            message =
                _deserializeCIMSubscriptionInitCompleteRequestMessage
                    (parser);
            break;
        case CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE:
            message =
                _deserializeCIMIndicationServiceDisabledRequestMessage
                    (parser);
            break;
        case PROVAGT_GET_SCMOCLASS_REQUEST_MESSAGE:
            message = _deserializeProvAgtGetScmoClassRequestMessage(parser);
            break;
        default:
            // Unexpected message type
            PEGASUS_ASSERT(0);
            break;
        }
        PEGASUS_ASSERT(message != 0);

        XmlReader::expectEndTag(parser, "PGOTHERREQ");
    }

    message->queueIds = queueIdStack;

    return message;
}

//
// _deserializeCIMResponseMessage
//
CIMResponseMessage* CIMMessageDeserializer::_deserializeCIMResponseMessage(
    XmlParser& parser,
    MessageType type)
{
    CIMResponseMessage* message = 0;
    QueueIdStack queueIdStack;
    CIMException cimException;

    _deserializeQueueIdStack(parser, queueIdStack);
    _deserializeCIMException(parser, cimException);

    switch (type)
    {
        //
        // CIM Operation Response Messages
        //

        // A provider cannot implement these operation types, so the
        // serialization of these messages is not implemented.
        //case CIM_GET_CLASS_RESPONSE_MESSAGE:
        //case CIM_DELETE_CLASS_RESPONSE_MESSAGE:
        //case CIM_CREATE_CLASS_RESPONSE_MESSAGE:
        //case CIM_MODIFY_CLASS_RESPONSE_MESSAGE:
        //case CIM_ENUMERATE_CLASSES_RESPONSE_MESSAGE:
        //case CIM_ENUMERATE_CLASS_NAMES_RESPONSE_MESSAGE:
        //case CIM_GET_QUALIFIER_RESPONSE_MESSAGE:
        //case CIM_SET_QUALIFIER_RESPONSE_MESSAGE:
        //case CIM_DELETE_QUALIFIER_RESPONSE_MESSAGE:
        //case CIM_ENUMERATE_QUALIFIERS_RESPONSE_MESSAGE:

        // Instance operations
        case CIM_GET_INSTANCE_RESPONSE_MESSAGE:
            message = _deserializeCIMGetInstanceResponseMessage(parser);
            break;
        case CIM_DELETE_INSTANCE_RESPONSE_MESSAGE:
            message = _deserializeCIMDeleteInstanceResponseMessage(parser);
            break;
        case CIM_CREATE_INSTANCE_RESPONSE_MESSAGE:
            message = _deserializeCIMCreateInstanceResponseMessage(parser);
            break;
        case CIM_MODIFY_INSTANCE_RESPONSE_MESSAGE:
            message = _deserializeCIMModifyInstanceResponseMessage(parser);
            break;
        case CIM_ENUMERATE_INSTANCES_RESPONSE_MESSAGE:
            message = _deserializeCIMEnumerateInstancesResponseMessage(parser);
            break;
        case CIM_ENUMERATE_INSTANCE_NAMES_RESPONSE_MESSAGE:
            message =
                _deserializeCIMEnumerateInstanceNamesResponseMessage(parser);
            break;
        case CIM_EXEC_QUERY_RESPONSE_MESSAGE:
            message = _deserializeCIMExecQueryResponseMessage(parser);
            break;

        // Property operations
        case CIM_GET_PROPERTY_RESPONSE_MESSAGE:
            message = _deserializeCIMGetPropertyResponseMessage(parser);
            break;
        case CIM_SET_PROPERTY_RESPONSE_MESSAGE:
            message = _deserializeCIMSetPropertyResponseMessage(parser);
            break;

        // Association operations
        case CIM_ASSOCIATORS_RESPONSE_MESSAGE:
            message = _deserializeCIMAssociatorsResponseMessage(parser);
            break;
        case CIM_ASSOCIATOR_NAMES_RESPONSE_MESSAGE:
            message = _deserializeCIMAssociatorNamesResponseMessage(parser);
            break;
        case CIM_REFERENCES_RESPONSE_MESSAGE:
            message = _deserializeCIMReferencesResponseMessage(parser);
            break;
        case CIM_REFERENCE_NAMES_RESPONSE_MESSAGE:
            message = _deserializeCIMReferenceNamesResponseMessage(parser);
            break;

        // Method operations
        case CIM_INVOKE_METHOD_RESPONSE_MESSAGE:
            message = _deserializeCIMInvokeMethodResponseMessage(parser);
            break;

        //
        // CIM Indication Response Messages
        //

        case CIM_CREATE_SUBSCRIPTION_RESPONSE_MESSAGE:
            message = _deserializeCIMCreateSubscriptionResponseMessage(parser);
            break;
        case CIM_MODIFY_SUBSCRIPTION_RESPONSE_MESSAGE:
            message = _deserializeCIMModifySubscriptionResponseMessage(parser);
            break;
        case CIM_DELETE_SUBSCRIPTION_RESPONSE_MESSAGE:
            message = _deserializeCIMDeleteSubscriptionResponseMessage(parser);
            break;

        //
        // Other CIM Response Messages
        //

        case CIM_EXPORT_INDICATION_RESPONSE_MESSAGE:
            message = _deserializeCIMExportIndicationResponseMessage(parser);
            break;
        case CIM_PROCESS_INDICATION_RESPONSE_MESSAGE:
            message = _deserializeCIMProcessIndicationResponseMessage(parser);
            break;
        //case CIM_NOTIFY_PROVIDER_REGISTRATION_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //message =
            //    _deserializeCIMNotifyProviderRegistrationResponseMessage(
            //        parser);
            //break;
        //case CIM_NOTIFY_PROVIDER_TERMINATION_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //message =
            //  _deserializeCIMNotifyProviderTerminationResponseMessage(parser);
            //break;
        //case CIM_HANDLE_INDICATION_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //message = _deserializeCIMHandleIndicationResponseMessage(parser);
            //break;
        case CIM_DISABLE_MODULE_RESPONSE_MESSAGE:
            message = _deserializeCIMDisableModuleResponseMessage(parser);
            break;
        case CIM_ENABLE_MODULE_RESPONSE_MESSAGE:
            message = _deserializeCIMEnableModuleResponseMessage(parser);
            break;
        //case CIM_NOTIFY_PROVIDER_ENABLE_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //message =
            //    _deserializeCIMNotifyProviderEnableResponseMessage(parser);
            //break;
        //case CIM_NOTIFY_PROVIDER_FAIL_RESPONSE_MESSAGE:
            // ATTN: No need to deserialize this yet
            //message =
            //    _deserializeCIMNotifyProviderFailResponseMessage(parser);
            //break;
        case CIM_STOP_ALL_PROVIDERS_RESPONSE_MESSAGE:
            message = _deserializeCIMStopAllProvidersResponseMessage(parser);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_RESPONSE_MESSAGE:
            message =
                _deserializeCIMInitializeProviderAgentResponseMessage(parser);
            break;
        case CIM_NOTIFY_CONFIG_CHANGE_RESPONSE_MESSAGE:
            message = _deserializeCIMNotifyConfigChangeResponseMessage(parser);
            break;
        case CIM_SUBSCRIPTION_INIT_COMPLETE_RESPONSE_MESSAGE:
            message =
                _deserializeCIMSubscriptionInitCompleteResponseMessage
                    (parser);
            break;
        case CIM_INDICATION_SERVICE_DISABLED_RESPONSE_MESSAGE:
            message =
                _deserializeCIMIndicationServiceDisabledResponseMessage
                    (parser);
            break;
        case PROVAGT_GET_SCMOCLASS_RESPONSE_MESSAGE:
            message = _deserializeProvAgtGetScmoClassResponseMessage
                (parser);
            break;

        default:
            // Unexpected message type
            PEGASUS_ASSERT(0);
            break;
    }
    PEGASUS_ASSERT(message != 0);

    message->queueIds = queueIdStack;
    message->cimException = cimException;

    return message;
}


//
// Utility Methods
//

//
// _deserializeUserInfo consolidates decoding of these common message attributes
//
void CIMMessageDeserializer::_deserializeUserInfo(
    XmlParser& parser,
    String& authType,
    String& userName)
{
    CIMValue genericValue;

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(authType);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(userName);
}

//
// _deserializeQueueIdStack
//
void CIMMessageDeserializer::_deserializeQueueIdStack(
    XmlParser& parser,
    QueueIdStack& queueIdStack)
{
    // ATTN: Incoming queueIdStack is presumed to be empty

    XmlEntry entry;
    CIMValue genericValue;
    Uint32 genericUint32;
    Array<Uint32> items;

    XmlReader::expectStartTag(parser, entry, "PGQIDSTACK");
    while (XmlReader::getValueElement(parser, CIMTYPE_UINT32, genericValue))
    {
        genericValue.get(genericUint32);
        items.append(genericUint32);
    }
    XmlReader::expectEndTag(parser, "PGQIDSTACK");

    for (Uint32 i=items.size(); i>0; i--)
    {
        queueIdStack.push(items[i-1]);
    }
}

//
// _deserializeOperationContext
//
void CIMMessageDeserializer::_deserializeOperationContext(
    XmlParser& parser,
    OperationContext& operationContext)
{
    operationContext.clear();

    XmlEntry entry;
    CIMValue genericValue;
    String genericString;

    XmlReader::expectStartTag(parser, entry, "PGOC");

    if (XmlReader::testStartTag(parser, entry, "PGOCID"))
    {
        String userName;

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(userName);
        operationContext.insert(IdentityContainer(userName));
        XmlReader::expectEndTag(parser, "PGOCID");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCSI"))
    {
        CIMInstance subscriptionInstance;
        _deserializeCIMInstance(parser, subscriptionInstance);
        operationContext.insert(
            SubscriptionInstanceContainer(subscriptionInstance));
        XmlReader::expectEndTag(parser, "PGOCSI");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCSFC"))
    {
        String filterCondition;
        String queryLanguage;

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(filterCondition);
        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(queryLanguage);
        operationContext.insert(
            SubscriptionFilterConditionContainer(
                filterCondition, queryLanguage));
        XmlReader::expectEndTag(parser, "PGOCSFC");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCSFQ"))
    {
        String filterQuery;
        String queryLanguage;
        CIMNamespaceName nameSpace;

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(filterQuery);
        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(queryLanguage);
        _deserializeCIMNamespaceName(parser, nameSpace);
        operationContext.insert(
            SubscriptionFilterQueryContainer(
                filterQuery, queryLanguage, nameSpace));
        XmlReader::expectEndTag(parser, "PGOCSFQ");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCSIN"))
    {
        Array<CIMObjectPath> subscriptionInstanceNames;
        CIMObjectPath genericObjectPath;

        while (_deserializeCIMObjectPath(parser, genericObjectPath))
        {
            subscriptionInstanceNames.append(genericObjectPath);
        }
        operationContext.insert(
            SubscriptionInstanceNamesContainer(subscriptionInstanceNames));
        XmlReader::expectEndTag(parser, "PGOCSIN");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCTO"))
    {
        Uint32 timeout;

        XmlReader::getValueElement(parser, CIMTYPE_UINT32, genericValue);
        genericValue.get(timeout);
        operationContext.insert(TimeoutContainer(timeout));
        XmlReader::expectEndTag(parser, "PGOCTO");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCALL"))
    {
        AcceptLanguageList acceptLanguages;

        _deserializeAcceptLanguageList(parser, acceptLanguages);
        operationContext.insert(AcceptLanguageListContainer(acceptLanguages));
        XmlReader::expectEndTag(parser, "PGOCALL");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCCLL"))
    {
        ContentLanguageList contentLanguages;

        _deserializeContentLanguageList(parser, contentLanguages);
        operationContext.insert(
            ContentLanguageListContainer(contentLanguages));
        XmlReader::expectEndTag(parser, "PGOCCLL");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCSTO"))
    {
        String snmpTrapOid;

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(snmpTrapOid);
        operationContext.insert(SnmpTrapOidContainer(snmpTrapOid));
        XmlReader::expectEndTag(parser, "PGOCSTO");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCL"))
    {
        String languageId;

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(languageId);
        operationContext.insert(LocaleContainer(languageId));
        XmlReader::expectEndTag(parser, "PGOCL");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCPI"))
    {
        CIMInstance module;
        CIMInstance provider;
        Boolean isRemoteNameSpace;
        String remoteInfo;
        String provMgrPath;

        _deserializeCIMInstance(parser, module);
        _deserializeCIMInstance(parser, provider);

        XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
        genericValue.get(isRemoteNameSpace);

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(remoteInfo);

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(provMgrPath);

        ProviderIdContainer pidc = ProviderIdContainer(module,
                                                      provider,
                                                      isRemoteNameSpace,
                                                      remoteInfo);
        pidc.setProvMgrPath(provMgrPath);
        operationContext.insert(pidc);
        XmlReader::expectEndTag(parser, "PGOCPI");
    }

    if (XmlReader::testStartTag(parser, entry, "PGOCCCD"))
    {
        CIMClass cimClass;

        XmlReader::getClassElement(parser, cimClass);
        operationContext.insert(CachedClassDefinitionContainer(cimClass));
        XmlReader::expectEndTag(parser, "PGOCCCD");
    }

    XmlReader::expectEndTag(parser, "PGOC");
}

//
// _deserializeContentLanguageList
//
void CIMMessageDeserializer::_deserializeContentLanguageList(
    XmlParser& parser,
    ContentLanguageList& contentLanguages)
{
    contentLanguages.clear();

    XmlEntry entry;
    CIMValue genericValue;
    String genericString;

    XmlReader::expectStartTag(parser, entry, "PGCONTLANGS");
    while (XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue))
    {
        genericValue.get(genericString);
        contentLanguages.append(LanguageTag(genericString));
    }
    XmlReader::expectEndTag(parser, "PGCONTLANGS");
}

//
// _deserializeAcceptLanguageList
//
void CIMMessageDeserializer::_deserializeAcceptLanguageList(
    XmlParser& parser,
    AcceptLanguageList& acceptLanguages)
{
    acceptLanguages.clear();

    XmlEntry entry;
    CIMValue genericValue;
    String genericString;
    Real32 genericReal32;

    XmlReader::expectStartTag(parser, entry, "PGACCLANGS");
    while (XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue))
    {
        genericValue.get(genericString);
        XmlReader::getValueElement(parser, CIMTYPE_REAL32, genericValue);
        genericValue.get(genericReal32);
        acceptLanguages.insert(LanguageTag(genericString), genericReal32);
    }
    XmlReader::expectEndTag(parser, "PGACCLANGS");
}

//
// _deserializeCIMException
//
void CIMMessageDeserializer::_deserializeCIMException(
    XmlParser& parser,
    CIMException& cimException)
{
    XmlEntry entry;
    CIMValue genericValue;
    Uint32 statusCode;
    String message;
    String cimMessage;
    String file;
    Uint32 line;
    ContentLanguageList contentLanguages;

    XmlReader::expectStartTag(parser, entry, "PGCIMEXC");

    XmlReader::getValueElement(parser, CIMTYPE_UINT32, genericValue);
    genericValue.get(statusCode);
    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(message);
    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(cimMessage);
    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(file);
    XmlReader::getValueElement(parser, CIMTYPE_UINT32, genericValue);
    genericValue.get(line);
    _deserializeContentLanguageList(parser, contentLanguages);

    XmlReader::expectEndTag(parser, "PGCIMEXC");

    TraceableCIMException e = TraceableCIMException(
        contentLanguages,
        CIMStatusCode(statusCode),
        message,
        file,
        line);
    e.setCIMMessage(cimMessage);
    cimException = e;
}

//
// _deserializeCIMPropertyList
//
void CIMMessageDeserializer::_deserializeCIMPropertyList(
    XmlParser& parser,
    CIMPropertyList& propertyList)
{
    const char* name;
    CIMValue genericValue;
    Boolean emptyTag;

    propertyList.clear();
    XmlReader::getIParamValueTag(parser, name, emptyTag);
    PEGASUS_ASSERT(!emptyTag);
    PEGASUS_ASSERT(System::strcasecmp(name, "PropertyList") == 0);
    if (XmlReader::getValueArrayElement(parser, CIMTYPE_STRING, genericValue))
    {
        Array<String> propertyListArray;
        genericValue.get(propertyListArray);
        Array<CIMName> cimNameArray;
        for (Uint32 i = 0; i < propertyListArray.size(); i++)
        {
            cimNameArray.append(propertyListArray[i]);
        }
        propertyList.set(cimNameArray);
    }
    XmlReader::expectEndTag(parser, "IPARAMVALUE");
}

//
// _deserializeCIMObjectPath
//
Boolean CIMMessageDeserializer::_deserializeCIMObjectPath(
    XmlParser& parser,
    CIMObjectPath& cimObjectPath)
{
    XmlEntry entry;

    if (!XmlReader::testStartTag(parser, entry, "PGPATH"))
    {
        return false;
    }

    // VALUE.REFERENCE element is absent when the object is uninitialized.
    // In this case, XmlReader::getValueReferenceElement returns "false" and
    // leaves cimObjectPath untouched.
    if (!XmlReader::getValueReferenceElement(parser, cimObjectPath))
    {
        cimObjectPath = CIMObjectPath();
    }

    XmlReader::expectEndTag(parser, "PGPATH");

    return true;
}

//
// _deserializeCIMInstance
//
Boolean CIMMessageDeserializer::_deserializeCIMInstance(
    XmlParser& parser,
    CIMInstance& cimInstance)
{
    XmlEntry entry;

    if (!XmlReader::testStartTag(parser, entry, "PGINST"))
    {
        return false;
    }

    // INSTANCE element is absent when the object is uninitialized.
    // In this case, XmlReader::getInstanceElement returns "false" and
    // leaves cimInstance untouched.
    if (XmlReader::getInstanceElement(parser, cimInstance))
    {
        CIMObjectPath path;
        _deserializeCIMObjectPath(parser, path);
        cimInstance.setPath(path);
    }
    else
    {
        cimInstance = CIMInstance();
    }

    XmlReader::expectEndTag(parser, "PGINST");

    return true;
}

//
// _deserializeCIMNamespaceName
//
void CIMMessageDeserializer::_deserializeCIMNamespaceName(
    XmlParser& parser,
    CIMNamespaceName& cimNamespaceName)
{
    CIMValue genericValue;
    String genericString;

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(genericString);
    if (genericString.size() > 0)
    {
        cimNamespaceName = CIMNamespaceName(genericString);
    }
}

//
// _deserializeCIMName
//
Boolean CIMMessageDeserializer::_deserializeCIMName(
    XmlParser& parser,
    CIMName& cimName)
{
    CIMValue genericValue;
    String genericString;

    if (!XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue))
    {
        return false;
    }

    genericValue.get(genericString);
    if (genericString.size() > 0)
    {
        cimName = CIMName(genericString);
    }
    else
    {
        cimName = CIMName();
    }

    return true;
}

//
// _deserializeCIMObject
//
Boolean CIMMessageDeserializer::_deserializeCIMObject(
    XmlParser& parser,
    CIMObject& object)
{
    XmlEntry entry;

    if (!XmlReader::testStartTag(parser, entry, "PGOBJ"))
    {
        return false;
    }

    CIMInstance cimInstance;
    CIMClass cimClass;
    CIMObjectPath path;

    // INSTANCE or CLASS element is absent when the object is uninitialized
    if (XmlReader::getInstanceElement(parser, cimInstance))
    {
        _deserializeCIMObjectPath(parser, path);
        cimInstance.setPath(path);
        object = CIMObject(cimInstance);
    }
    else if (XmlReader::getClassElement(parser, cimClass))
    {
        _deserializeCIMObjectPath(parser, path);
        cimClass.setPath(path);
        object = CIMObject(cimClass);
    }
    else
    {
        // Uninitialized object
        object = CIMObject();
    }

    XmlReader::expectEndTag(parser, "PGOBJ");

    return true;
}

//
// _deserializeCIMParamValue
//
Boolean CIMMessageDeserializer::_deserializeCIMParamValue(
    XmlParser& parser,
    CIMParamValue& paramValue)
{
    XmlEntry entry;

    if (XmlReader::getParamValueElement(parser, paramValue))
    {
        return true;
    }

    if (XmlReader::testStartTagOrEmptyTag(parser, entry, "PGNULLPARAMVALUE"))
    {
        // The parameter value is null; set the correct type

        CIMValue genericValue;
        CIMType type;
        String name;
        Boolean isArray;

        Boolean found = XmlReader::getCimTypeAttribute(
            parser.getLine(),
            entry,
            type,
            "PGNULLPARAMVALUE",
            "PARAMTYPE",
            false);
        PEGASUS_ASSERT(found);

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(name);

        XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
        genericValue.get(isArray);

        XmlReader::expectEndTag(parser, "PGNULLPARAMVALUE");

        paramValue = CIMParamValue(name, CIMValue(type, isArray), true);
        return true;
    }

    return false;
}


//
//
// Response Messages
//
//

//
//
// CIMOperationRequestMessages
//
//

//
// _deserializeCIMGetInstanceRequestMessage
//
CIMGetInstanceRequestMessage*
CIMMessageDeserializer::_deserializeCIMGetInstanceRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMObjectPath instanceName;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    _deserializeCIMObjectPath(parser, instanceName);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeQualifiers);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeClassOrigin);

    _deserializeCIMPropertyList(parser, propertyList);

    CIMGetInstanceRequestMessage* message =
        new CIMGetInstanceRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            instanceName,
            includeQualifiers,
            includeClassOrigin,
            propertyList,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMDeleteInstanceRequestMessage
//
CIMDeleteInstanceRequestMessage*
CIMMessageDeserializer::_deserializeCIMDeleteInstanceRequestMessage(
    XmlParser& parser)
{
    CIMObjectPath instanceName;

    _deserializeCIMObjectPath(parser, instanceName);

    CIMDeleteInstanceRequestMessage* message =
        new CIMDeleteInstanceRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            instanceName,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMCreateInstanceRequestMessage
//
CIMCreateInstanceRequestMessage*
CIMMessageDeserializer::_deserializeCIMCreateInstanceRequestMessage(
    XmlParser& parser)
{
    CIMInstance newInstance;

    _deserializeCIMInstance(parser, newInstance);

    CIMCreateInstanceRequestMessage* message =
        new CIMCreateInstanceRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            newInstance,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMModifyInstanceRequestMessage
//
CIMModifyInstanceRequestMessage*
CIMMessageDeserializer::_deserializeCIMModifyInstanceRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMInstance modifiedInstance;
    Boolean includeQualifiers;
    CIMPropertyList propertyList;

    _deserializeCIMInstance(parser, modifiedInstance);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeQualifiers);

    _deserializeCIMPropertyList(parser, propertyList);

    CIMModifyInstanceRequestMessage* message =
        new CIMModifyInstanceRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            modifiedInstance,
            includeQualifiers,
            propertyList,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMEnumerateInstancesRequestMessage
//
CIMEnumerateInstancesRequestMessage*
CIMMessageDeserializer::_deserializeCIMEnumerateInstancesRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMObjectPath instanceName;
    Boolean deepInheritance;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(deepInheritance);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeQualifiers);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeClassOrigin);

    _deserializeCIMPropertyList(parser, propertyList);

    CIMEnumerateInstancesRequestMessage* message =
        new CIMEnumerateInstancesRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            CIMName(),             // className
            deepInheritance,
            includeQualifiers,
            includeClassOrigin,
            propertyList,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMEnumerateInstanceNamesRequestMessage
//
CIMEnumerateInstanceNamesRequestMessage*
CIMMessageDeserializer::_deserializeCIMEnumerateInstanceNamesRequestMessage(
    XmlParser& parser)
{
    CIMEnumerateInstanceNamesRequestMessage* message =
        new CIMEnumerateInstanceNamesRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            CIMName(),             // className
            QueueIdStack());

    return message;
}

//
// _deserializeCIMExecQueryRequestMessage
//
CIMExecQueryRequestMessage*
CIMMessageDeserializer::_deserializeCIMExecQueryRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    String queryLanguage;
    String query;

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(queryLanguage);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(query);

    CIMExecQueryRequestMessage* message =
        new CIMExecQueryRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            queryLanguage,
            query,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMAssociatorsRequestMessage
//
CIMAssociatorsRequestMessage*
CIMMessageDeserializer::_deserializeCIMAssociatorsRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMObjectPath objectName;
    CIMName assocClass;
    CIMName resultClass;
    String role;
    String resultRole;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    _deserializeCIMObjectPath(parser, objectName);
    _deserializeCIMName(parser, assocClass);
    _deserializeCIMName(parser, resultClass);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(role);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(resultRole);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeQualifiers);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeClassOrigin);

    _deserializeCIMPropertyList(parser, propertyList);

    CIMAssociatorsRequestMessage* message =
        new CIMAssociatorsRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            objectName,
            assocClass,
            resultClass,
            role,
            resultRole,
            includeQualifiers,
            includeClassOrigin,
            propertyList,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMAssociatorNamesRequestMessage
//
CIMAssociatorNamesRequestMessage*
CIMMessageDeserializer::_deserializeCIMAssociatorNamesRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMObjectPath objectName;
    CIMName assocClass;
    CIMName resultClass;
    String role;
    String resultRole;

    _deserializeCIMObjectPath(parser, objectName);
    _deserializeCIMName(parser, assocClass);
    _deserializeCIMName(parser, resultClass);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(role);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(resultRole);

    CIMAssociatorNamesRequestMessage* message =
        new CIMAssociatorNamesRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            objectName,
            assocClass,
            resultClass,
            role,
            resultRole,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMReferencesRequestMessage
//
CIMReferencesRequestMessage*
CIMMessageDeserializer::_deserializeCIMReferencesRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMObjectPath objectName;
    CIMName resultClass;
    String role;
    Boolean includeQualifiers;
    Boolean includeClassOrigin;
    CIMPropertyList propertyList;

    _deserializeCIMObjectPath(parser, objectName);
    _deserializeCIMName(parser, resultClass);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(role);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeQualifiers);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(includeClassOrigin);

    _deserializeCIMPropertyList(parser, propertyList);

    CIMReferencesRequestMessage* message =
        new CIMReferencesRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            objectName,
            resultClass,
            role,
            includeQualifiers,
            includeClassOrigin,
            propertyList,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMReferenceNamesRequestMessage
//
CIMReferenceNamesRequestMessage*
CIMMessageDeserializer::_deserializeCIMReferenceNamesRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    CIMObjectPath objectName;
    CIMName resultClass;
    String role;

    _deserializeCIMObjectPath(parser, objectName);
    _deserializeCIMName(parser, resultClass);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(role);

    CIMReferenceNamesRequestMessage* message =
        new CIMReferenceNamesRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            objectName,
            resultClass,
            role,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMGetPropertyRequestMessage
//
CIMGetPropertyRequestMessage*
CIMMessageDeserializer::_deserializeCIMGetPropertyRequestMessage(
    XmlParser& parser)
{
    CIMObjectPath instanceName;
    CIMName propertyName;

    _deserializeCIMObjectPath(parser, instanceName);
    _deserializeCIMName(parser, propertyName);

    CIMGetPropertyRequestMessage* message =
        new CIMGetPropertyRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            instanceName,
            propertyName,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMSetPropertyRequestMessage
//
CIMSetPropertyRequestMessage*
CIMMessageDeserializer::_deserializeCIMSetPropertyRequestMessage(
    XmlParser& parser)
{
    CIMObjectPath instanceName;
    CIMParamValue newValue;

    _deserializeCIMObjectPath(parser, instanceName);

    _deserializeCIMParamValue(parser, newValue);

    CIMSetPropertyRequestMessage* message =
        new CIMSetPropertyRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            instanceName,
            newValue.getParameterName(),
            newValue.getValue(),
            QueueIdStack());

    return message;
}

//
// _deserializeCIMInvokeMethodRequestMessage
//
CIMInvokeMethodRequestMessage*
CIMMessageDeserializer::_deserializeCIMInvokeMethodRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMParamValue genericParamValue;
    CIMObjectPath instanceName;
    CIMName methodName;
    Array<CIMParamValue> inParameters;

    _deserializeCIMObjectPath(parser, instanceName);
    _deserializeCIMName(parser, methodName);

    // Get inParameter array
    XmlReader::expectStartTag(parser, entry, "PGPARAMS");
    while (_deserializeCIMParamValue(parser, genericParamValue))
    {
        inParameters.append(genericParamValue);
    }
    XmlReader::expectEndTag(parser, "PGPARAMS");

    CIMInvokeMethodRequestMessage* message =
        new CIMInvokeMethodRequestMessage(
            String::EMPTY,         // messageId
            CIMNamespaceName(),    // nameSpace
            instanceName,
            methodName,
            inParameters,
            QueueIdStack());

    return message;
}


//
//
// CIMIndicationRequestMessages
//
//

//
// _deserializeCIMCreateSubscriptionRequestMessage
//
CIMCreateSubscriptionRequestMessage*
CIMMessageDeserializer::_deserializeCIMCreateSubscriptionRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMValue genericValue;
    CIMName genericName;
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array<CIMName> classNames;
    CIMPropertyList propertyList;
    Uint16 repeatNotificationPolicy;
    String query;

    _deserializeCIMNamespaceName(parser, nameSpace);
    _deserializeCIMInstance(parser, subscriptionInstance);

    // Get classNames array
    XmlReader::expectStartTag(parser, entry, "PGNAMEARRAY");
    while (_deserializeCIMName(parser, genericName))
    {
        classNames.append(genericName);
    }
    XmlReader::expectEndTag(parser, "PGNAMEARRAY");

    _deserializeCIMPropertyList(parser, propertyList);

    // Decode repeatNotificationPolicy
    XmlReader::getValueElement(parser, CIMTYPE_UINT16, genericValue);
    genericValue.get(repeatNotificationPolicy);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(query);

    CIMCreateSubscriptionRequestMessage* message =
        new CIMCreateSubscriptionRequestMessage(
            String::EMPTY,         // messageId
            nameSpace,
            subscriptionInstance,
            classNames,
            propertyList,
            repeatNotificationPolicy,
            query,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMModifySubscriptionRequestMessage
//
CIMModifySubscriptionRequestMessage*
CIMMessageDeserializer::_deserializeCIMModifySubscriptionRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMValue genericValue;
    CIMName genericName;
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array<CIMName> classNames;
    CIMPropertyList propertyList;
    Uint16 repeatNotificationPolicy;
    String query;

    _deserializeCIMNamespaceName(parser, nameSpace);
    _deserializeCIMInstance(parser, subscriptionInstance);

    // Get classNames array
    XmlReader::expectStartTag(parser, entry, "PGNAMEARRAY");
    while (_deserializeCIMName(parser, genericName))
    {
        classNames.append(genericName);
    }
    XmlReader::expectEndTag(parser, "PGNAMEARRAY");

    _deserializeCIMPropertyList(parser, propertyList);

    // Decode repeatNotificationPolicy
    XmlReader::getValueElement(parser, CIMTYPE_UINT16, genericValue);
    genericValue.get(repeatNotificationPolicy);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(query);

    CIMModifySubscriptionRequestMessage* message =
        new CIMModifySubscriptionRequestMessage(
            String::EMPTY,         // messageId
            nameSpace,
            subscriptionInstance,
            classNames,
            propertyList,
            repeatNotificationPolicy,
            query,
            QueueIdStack());

    return message;
}

//
// _deserializeCIMDeleteSubscriptionRequestMessage
//
CIMDeleteSubscriptionRequestMessage*
CIMMessageDeserializer::_deserializeCIMDeleteSubscriptionRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMName genericName;
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array<CIMName> classNames;

    _deserializeCIMNamespaceName(parser, nameSpace);
    _deserializeCIMInstance(parser, subscriptionInstance);

    // Get classNames array
    XmlReader::expectStartTag(parser, entry, "PGNAMEARRAY");
    while (_deserializeCIMName(parser, genericName))
    {
        classNames.append(genericName);
    }
    XmlReader::expectEndTag(parser, "PGNAMEARRAY");

    CIMDeleteSubscriptionRequestMessage* message =
        new CIMDeleteSubscriptionRequestMessage(
            String::EMPTY,         // messageId
            nameSpace,
            subscriptionInstance,
            classNames,
            QueueIdStack());

    return message;
}


//
//
// Other CIMRequestMessages
//
//

//
// _deserializeCIMExportIndicationRequestMessage
//
CIMExportIndicationRequestMessage*
CIMMessageDeserializer::_deserializeCIMExportIndicationRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    String authType;
    String userName;
    String destinationPath;
    CIMInstance indicationInstance;

    _deserializeUserInfo(parser, authType, userName);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(destinationPath);

    _deserializeCIMInstance(parser, indicationInstance);

    CIMExportIndicationRequestMessage* message =
        new CIMExportIndicationRequestMessage(
            String::EMPTY,         // messageId
            destinationPath,
            indicationInstance,
            QueueIdStack(),        // queueIds
            authType,
            userName);

    return message;
}

//
// _deserializeCIMProcessIndicationRequestMessage
//
CIMProcessIndicationRequestMessage*
CIMMessageDeserializer::_deserializeCIMProcessIndicationRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObjectPath genericObjectPath;
    CIMNamespaceName nameSpace;
    CIMInstance indicationInstance;
    Array<CIMObjectPath> subscriptionInstanceNames;
    CIMInstance provider;

    _deserializeCIMNamespaceName(parser, nameSpace);
    _deserializeCIMInstance(parser, indicationInstance);

    // Get subscriptionInstanceNames array
    XmlReader::expectStartTag(parser, entry, "PGPATHARRAY");
    while (_deserializeCIMObjectPath(parser, genericObjectPath))
    {
        subscriptionInstanceNames.append(genericObjectPath);
    }
    XmlReader::expectEndTag(parser, "PGPATHARRAY");

    _deserializeCIMInstance(parser, provider);

    CIMProcessIndicationRequestMessage* message =
        new CIMProcessIndicationRequestMessage(
            String::EMPTY,         // messageId
            nameSpace,
            indicationInstance,
            subscriptionInstanceNames,
            provider,
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMDisableModuleRequestMessage
//
CIMDisableModuleRequestMessage*
CIMMessageDeserializer::_deserializeCIMDisableModuleRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMValue genericValue;
    CIMInstance genericInstance;
    Boolean genericBoolean;
    String authType;
    String userName;
    CIMInstance providerModule;
    Array<CIMInstance> providers;
    Boolean disableProviderOnly;
    Array<Boolean> indicationProviders;

    _deserializeUserInfo(parser, authType, userName);

    _deserializeCIMInstance(parser, providerModule);

    // Get providers array
    XmlReader::expectStartTag(parser, entry, "PGINSTARRAY");
    while (_deserializeCIMInstance(parser, genericInstance))
    {
        providers.append(genericInstance);
    }
    XmlReader::expectEndTag(parser, "PGINSTARRAY");

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(disableProviderOnly);

    // Get indicationProviders array
    XmlReader::expectStartTag(parser, entry, "PGBOOLARRAY");
    while (XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue))
    {
        genericValue.get(genericBoolean);
        indicationProviders.append(genericBoolean);
    }
    XmlReader::expectEndTag(parser, "PGBOOLARRAY");

    CIMDisableModuleRequestMessage* message =
        new CIMDisableModuleRequestMessage(
            String::EMPTY,         // messageId
            providerModule,
            providers,
            disableProviderOnly,
            indicationProviders,
            QueueIdStack(),        // queueIds
            authType,
            userName);

    return message;
}

//
// _deserializeCIMEnableModuleRequestMessage
//
CIMEnableModuleRequestMessage*
CIMMessageDeserializer::_deserializeCIMEnableModuleRequestMessage(
    XmlParser& parser)
{
    String authType;
    String userName;
    CIMInstance providerModule;

    _deserializeUserInfo(parser, authType, userName);

    _deserializeCIMInstance(parser, providerModule);

    CIMEnableModuleRequestMessage* message =
        new CIMEnableModuleRequestMessage(
            String::EMPTY,         // messageId
            providerModule,
            QueueIdStack(),        // queueIds
            authType,
            userName);

    return message;
}

//
// _deserializeCIMStopAllProvidersRequestMessage
//
CIMStopAllProvidersRequestMessage*
CIMMessageDeserializer::_deserializeCIMStopAllProvidersRequestMessage(
    XmlParser& parser)
{
    CIMStopAllProvidersRequestMessage* message =
        new CIMStopAllProvidersRequestMessage(
            String::EMPTY,         // messageId
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMInitializeProviderAgentRequestMessage
//
CIMInitializeProviderAgentRequestMessage*
CIMMessageDeserializer::_deserializeCIMInitializeProviderAgentRequestMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMValue genericValue;
    String pegasusHome;
    Array<Pair<String, String> > configProperties;
    Boolean bindVerbose;
    Boolean subscriptionInitComplete;

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(pegasusHome);

    // Get configProperties array
    XmlReader::expectStartTag(parser, entry, "PGCONFARRAY");
    while (XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue))
    {
        String propertyName;
        String propertyValue;

        genericValue.get(propertyName);

        XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
        genericValue.get(propertyValue);

        configProperties.append(
            Pair<String, String>(propertyName, propertyValue));
    }
    XmlReader::expectEndTag(parser, "PGCONFARRAY");

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(bindVerbose);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(subscriptionInitComplete);

    CIMInitializeProviderAgentRequestMessage* message =
        new CIMInitializeProviderAgentRequestMessage(
            String::EMPTY,         // messageId
            pegasusHome,
            configProperties,
            bindVerbose,
            subscriptionInitComplete,
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMNotifyConfigChangeRequestMessage
//
CIMNotifyConfigChangeRequestMessage*
CIMMessageDeserializer::_deserializeCIMNotifyConfigChangeRequestMessage(
    XmlParser& parser)
{
    CIMValue genericValue;
    String propertyName;
    String newPropertyValue;
    Boolean currentValueModified;

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(propertyName);

    XmlReader::getValueElement(parser, CIMTYPE_STRING, genericValue);
    genericValue.get(newPropertyValue);

    XmlReader::getValueElement(parser, CIMTYPE_BOOLEAN, genericValue);
    genericValue.get(currentValueModified);

    CIMNotifyConfigChangeRequestMessage* message =
        new CIMNotifyConfigChangeRequestMessage(
            String::EMPTY,         // messageId
            propertyName,
            newPropertyValue,
            currentValueModified,
            QueueIdStack());        // queueIds

    return message;
}

//
// _deserializeCIMSubscriptionInitCompleteRequestMessage
//
CIMSubscriptionInitCompleteRequestMessage*
CIMMessageDeserializer::_deserializeCIMSubscriptionInitCompleteRequestMessage(
    XmlParser& parser)
{
    CIMSubscriptionInitCompleteRequestMessage* message =
        new CIMSubscriptionInitCompleteRequestMessage(
            String::EMPTY,         // messageId
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMIndicationServiceDisabledRequestMessage
//
CIMIndicationServiceDisabledRequestMessage*
CIMMessageDeserializer::_deserializeCIMIndicationServiceDisabledRequestMessage(
    XmlParser& parser)
{
    CIMIndicationServiceDisabledRequestMessage* message =
        new CIMIndicationServiceDisabledRequestMessage(
            String(),         // messageId
            String::EMPTY,         // messageId
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeProvAgtGetScmoClassRequestMessage
//
ProvAgtGetScmoClassRequestMessage*
CIMMessageDeserializer::_deserializeProvAgtGetScmoClassRequestMessage(
    XmlParser& parser)
{
    CIMNamespaceName nameSpace;
    CIMName className;


    _deserializeCIMNamespaceName(parser,nameSpace);

    _deserializeCIMName(parser, className);


    ProvAgtGetScmoClassRequestMessage* message =
        new ProvAgtGetScmoClassRequestMessage(
            String::EMPTY,         // messageId
            nameSpace,
            className,
            QueueIdStack());       // queueIds

    return message;
}

//
//
// Response Messages
//
//

//
//
// CIM Operation Response Messages
//
//

//
// _deserializeCIMGetInstanceResponseMessage
//
CIMGetInstanceResponseMessage*
CIMMessageDeserializer::_deserializeCIMGetInstanceResponseMessage(
    XmlParser& parser)
{
    CIMInstance cimInstance;

    _deserializeCIMInstance(parser, cimInstance);

    CIMGetInstanceResponseMessage* message =
        new CIMGetInstanceResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    message->getResponseData().setInstance(cimInstance);

    return message;
}

//
// _deserializeCIMDeleteInstanceResponseMessage
//
CIMDeleteInstanceResponseMessage*
CIMMessageDeserializer::_deserializeCIMDeleteInstanceResponseMessage(
    XmlParser& parser)
{
    CIMDeleteInstanceResponseMessage* message =
        new CIMDeleteInstanceResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMCreateInstanceResponseMessage
//
CIMCreateInstanceResponseMessage*
CIMMessageDeserializer::_deserializeCIMCreateInstanceResponseMessage(
    XmlParser& parser)
{
    CIMObjectPath instanceName;

    _deserializeCIMObjectPath(parser, instanceName);

    CIMCreateInstanceResponseMessage* message =
        new CIMCreateInstanceResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack(),        // queueIds
            instanceName);

    return message;
}

//
// _deserializeCIMModifyInstanceResponseMessage
//
CIMModifyInstanceResponseMessage*
CIMMessageDeserializer::_deserializeCIMModifyInstanceResponseMessage(
    XmlParser& parser)
{
    CIMModifyInstanceResponseMessage* message =
        new CIMModifyInstanceResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMEnumerateInstancesResponseMessage
//
CIMEnumerateInstancesResponseMessage*
CIMMessageDeserializer::_deserializeCIMEnumerateInstancesResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMInstance genericInstance;
    Array<CIMInstance> cimNamedInstances;

    // Get cimNamedInstances array
    XmlReader::expectStartTag(parser, entry, "PGINSTARRAY");
    while (_deserializeCIMInstance(parser, genericInstance))
    {
        cimNamedInstances.append(genericInstance);
    }
    XmlReader::expectEndTag(parser, "PGINSTARRAY");

    CIMEnumerateInstancesResponseMessage* message =
        new CIMEnumerateInstancesResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setInstances(cimNamedInstances);

    return message;
}

//
// _deserializeCIMEnumerateInstanceNamesResponseMessage
//
CIMEnumerateInstanceNamesResponseMessage*
CIMMessageDeserializer::_deserializeCIMEnumerateInstanceNamesResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObjectPath genericObjectPath;
    Array<CIMObjectPath> instanceNames;

    // Get instanceNames array
    XmlReader::expectStartTag(parser, entry, "PGPATHARRAY");
    while (_deserializeCIMObjectPath(parser, genericObjectPath))
    {
        instanceNames.append(genericObjectPath);
    }
    XmlReader::expectEndTag(parser, "PGPATHARRAY");

    CIMEnumerateInstanceNamesResponseMessage* message =
        new CIMEnumerateInstanceNamesResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setInstanceNames(instanceNames);

    return message;
}

//
// _deserializeCIMExecQueryResponseMessage
//
CIMExecQueryResponseMessage*
CIMMessageDeserializer::_deserializeCIMExecQueryResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObject genericObject;
    Array<CIMObject> cimObjects;

    // Get cimObjects array
    XmlReader::expectStartTag(parser, entry, "PGOBJARRAY");
    while (_deserializeCIMObject(parser, genericObject))
    {
        cimObjects.append(genericObject);
    }
    XmlReader::expectEndTag(parser, "PGOBJARRAY");

    CIMExecQueryResponseMessage* message =
        new CIMExecQueryResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setObjects(cimObjects);

    return message;
}

//
// _deserializeCIMAssociatorsResponseMessage
//
CIMAssociatorsResponseMessage*
CIMMessageDeserializer::_deserializeCIMAssociatorsResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObject genericObject;
    Array<CIMObject> cimObjects;

    // Get cimObjects array
    XmlReader::expectStartTag(parser, entry, "PGOBJARRAY");
    while (_deserializeCIMObject(parser, genericObject))
    {
        cimObjects.append(genericObject);
    }
    XmlReader::expectEndTag(parser, "PGOBJARRAY");

    CIMAssociatorsResponseMessage* message =
        new CIMAssociatorsResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setObjects(cimObjects);

    return message;
}

//
// _deserializeCIMAssociatorNamesResponseMessage
//
CIMAssociatorNamesResponseMessage*
CIMMessageDeserializer::_deserializeCIMAssociatorNamesResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObjectPath genericObjectPath;
    Array<CIMObjectPath> objectNames;

    // Get objectNames array
    XmlReader::expectStartTag(parser, entry, "PGPATHARRAY");
    while (_deserializeCIMObjectPath(parser, genericObjectPath))
    {
        objectNames.append(genericObjectPath);
    }
    XmlReader::expectEndTag(parser, "PGPATHARRAY");

    CIMAssociatorNamesResponseMessage* message =
        new CIMAssociatorNamesResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setInstanceNames(objectNames);

    return message;
}

//
// _deserializeCIMReferencesResponseMessage
//
CIMReferencesResponseMessage*
CIMMessageDeserializer::_deserializeCIMReferencesResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObject genericObject;
    Array<CIMObject> cimObjects;

    // Get cimObjects array
    XmlReader::expectStartTag(parser, entry, "PGOBJARRAY");
    while (_deserializeCIMObject(parser, genericObject))
    {
        cimObjects.append(genericObject);
    }
    XmlReader::expectEndTag(parser, "PGOBJARRAY");

    CIMReferencesResponseMessage* message =
        new CIMReferencesResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setObjects(cimObjects);

    return message;
}

//
// _deserializeCIMReferenceNamesResponseMessage
//
CIMReferenceNamesResponseMessage*
CIMMessageDeserializer::_deserializeCIMReferenceNamesResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMObjectPath genericObjectPath;
    Array<CIMObjectPath> objectNames;

    // Get objectNames array
    XmlReader::expectStartTag(parser, entry, "PGPATHARRAY");
    while (_deserializeCIMObjectPath(parser, genericObjectPath))
    {
        objectNames.append(genericObjectPath);
    }
    XmlReader::expectEndTag(parser, "PGPATHARRAY");

    CIMReferenceNamesResponseMessage* message =
        new CIMReferenceNamesResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());        // queueIds
    message->getResponseData().setInstanceNames(objectNames);

    return message;
}

//
// _deserializeCIMGetPropertyResponseMessage
//
CIMGetPropertyResponseMessage*
CIMMessageDeserializer::_deserializeCIMGetPropertyResponseMessage(
    XmlParser& parser)
{
    CIMParamValue value;

    _deserializeCIMParamValue(parser, value);

    CIMGetPropertyResponseMessage* message =
        new CIMGetPropertyResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack(),        // queueIds
            value.getValue());

    return message;
}

//
// _deserializeCIMSetPropertyResponseMessage
//
CIMSetPropertyResponseMessage*
CIMMessageDeserializer::_deserializeCIMSetPropertyResponseMessage(
    XmlParser& parser)
{
    CIMSetPropertyResponseMessage* message =
        new CIMSetPropertyResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMInvokeMethodResponseMessage
//
CIMInvokeMethodResponseMessage*
CIMMessageDeserializer::_deserializeCIMInvokeMethodResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMParamValue genericParamValue;
    CIMParamValue retValue;
    CIMName methodName;
    Array<CIMParamValue> outParameters;

    _deserializeCIMParamValue(parser, retValue);

    // Get outParameter array
    XmlReader::expectStartTag(parser, entry, "PGPARAMS");
    while (_deserializeCIMParamValue(parser, genericParamValue))
    {
        outParameters.append(genericParamValue);
    }
    XmlReader::expectEndTag(parser, "PGPARAMS");

    _deserializeCIMName(parser, methodName);

    CIMInvokeMethodResponseMessage* message =
        new CIMInvokeMethodResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack(),        // queueIds
            retValue.getValue(),
            outParameters,
            methodName);

    return message;
}


//
//
// CIM Indication Response Messages
//
//

//
// _deserializeCIMCreateSubscriptionResponseMessage
//
CIMCreateSubscriptionResponseMessage*
CIMMessageDeserializer::_deserializeCIMCreateSubscriptionResponseMessage(
    XmlParser& parser)
{
    CIMCreateSubscriptionResponseMessage* message =
        new CIMCreateSubscriptionResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMModifySubscriptionResponseMessage
//
CIMModifySubscriptionResponseMessage*
CIMMessageDeserializer::_deserializeCIMModifySubscriptionResponseMessage(
    XmlParser& parser)
{
    CIMModifySubscriptionResponseMessage* message =
        new CIMModifySubscriptionResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMDeleteSubscriptionResponseMessage
//
CIMDeleteSubscriptionResponseMessage*
CIMMessageDeserializer::_deserializeCIMDeleteSubscriptionResponseMessage(
    XmlParser& parser)
{
    CIMDeleteSubscriptionResponseMessage* message =
        new CIMDeleteSubscriptionResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}


//
//
// Other CIMResponseMessages
//
//

//
// _deserializeCIMExportIndicationResponseMessage
//
CIMExportIndicationResponseMessage*
CIMMessageDeserializer::_deserializeCIMExportIndicationResponseMessage(
    XmlParser& parser)
{
    CIMExportIndicationResponseMessage* message =
        new CIMExportIndicationResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMProcessIndicationResponseMessage
//
CIMProcessIndicationResponseMessage*
CIMMessageDeserializer::_deserializeCIMProcessIndicationResponseMessage(
    XmlParser& parser)
{
    CIMProcessIndicationResponseMessage* message =
        new CIMProcessIndicationResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMDisableModuleResponseMessage
//
CIMDisableModuleResponseMessage*
CIMMessageDeserializer::_deserializeCIMDisableModuleResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMValue genericValue;
    Uint16 genericUint16;
    Array<Uint16> operationalStatus;

    // Get operationalStatus array
    XmlReader::expectStartTag(parser, entry, "PGUINT16ARRAY");
    while (XmlReader::getValueElement(parser, CIMTYPE_UINT16, genericValue))
    {
        genericValue.get(genericUint16);
        operationalStatus.append(genericUint16);
    }
    XmlReader::expectEndTag(parser, "PGUINT16ARRAY");

    CIMDisableModuleResponseMessage* message =
        new CIMDisableModuleResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack(),        // queueIds
            operationalStatus);

    return message;
}

//
// _deserializeCIMEnableModuleResponseMessage
//
CIMEnableModuleResponseMessage*
CIMMessageDeserializer::_deserializeCIMEnableModuleResponseMessage(
    XmlParser& parser)
{
    XmlEntry entry;
    CIMValue genericValue;
    Uint16 genericUint16;
    Array<Uint16> operationalStatus;

    // Get operationalStatus array
    XmlReader::expectStartTag(parser, entry, "PGUINT16ARRAY");
    while (XmlReader::getValueElement(parser, CIMTYPE_UINT16, genericValue))
    {
        genericValue.get(genericUint16);
        operationalStatus.append(genericUint16);
    }
    XmlReader::expectEndTag(parser, "PGUINT16ARRAY");

    CIMEnableModuleResponseMessage* message =
        new CIMEnableModuleResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack(),        // queueIds
            operationalStatus);

    return message;
}

//
// _deserializeCIMStopAllProvidersResponseMessage
//
CIMStopAllProvidersResponseMessage*
CIMMessageDeserializer::_deserializeCIMStopAllProvidersResponseMessage(
    XmlParser& parser)
{
    CIMStopAllProvidersResponseMessage* message =
        new CIMStopAllProvidersResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMInitializeProviderAgentResponseMessage
//
CIMInitializeProviderAgentResponseMessage*
CIMMessageDeserializer::_deserializeCIMInitializeProviderAgentResponseMessage(
    XmlParser& parser)
{
    CIMInitializeProviderAgentResponseMessage* message =
        new CIMInitializeProviderAgentResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMNotifyConfigChangeResponseMessage
//
CIMNotifyConfigChangeResponseMessage*
CIMMessageDeserializer::_deserializeCIMNotifyConfigChangeResponseMessage(
    XmlParser& parser)
{
    CIMNotifyConfigChangeResponseMessage* message =
        new CIMNotifyConfigChangeResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMSubscriptionInitCompleteResponseMessage
//
CIMSubscriptionInitCompleteResponseMessage*
CIMMessageDeserializer::_deserializeCIMSubscriptionInitCompleteResponseMessage(
    XmlParser& parser)
{
    CIMSubscriptionInitCompleteResponseMessage* message =
        new CIMSubscriptionInitCompleteResponseMessage(
            String::EMPTY,         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeCIMIndicationServiceDisabledResponseMessage
//
CIMIndicationServiceDisabledResponseMessage*
CIMMessageDeserializer::_deserializeCIMIndicationServiceDisabledResponseMessage(
    XmlParser& parser)
{
    CIMIndicationServiceDisabledResponseMessage* message =
        new CIMIndicationServiceDisabledResponseMessage(
            String(),         // messageId
            CIMException(),        // cimException
            QueueIdStack());       // queueIds

    return message;
}

//
// _deserializeProvAgtGetScmoClassResponseMessage
//
ProvAgtGetScmoClassResponseMessage*
CIMMessageDeserializer::_deserializeProvAgtGetScmoClassResponseMessage(
    XmlParser& parser)
{
    CIMClass cimClass;
    ProvAgtGetScmoClassResponseMessage* message;
    CIMNamespaceName nameSpace;

   if (XmlReader::getClassElement(parser, cimClass))
   {
       _deserializeCIMNamespaceName(parser,nameSpace);

       message = new ProvAgtGetScmoClassResponseMessage(
           String(),         // messageId
           CIMException(),        // cimException
           QueueIdStack(),        // queueIds
           SCMOClass(
               cimClass,
               (const char*)nameSpace.getString().getCString()));   
    } else
    {
        message = new ProvAgtGetScmoClassResponseMessage(
            String(),         // messageId
            CIMException(),        // cimException
            QueueIdStack(),        // queueIds
            SCMOClass("",""));     // empty SCMOClass
    }


   return message;
}

PEGASUS_NAMESPACE_END
