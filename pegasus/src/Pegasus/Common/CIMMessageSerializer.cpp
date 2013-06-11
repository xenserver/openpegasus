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

#include <Pegasus/Common/XmlWriter.h>
#include <Pegasus/Common/OperationContextInternal.h>

#include "CIMMessageSerializer.h"

PEGASUS_NAMESPACE_BEGIN

void CIMMessageSerializer::serialize(Buffer& out, CIMMessage* cimMessage)
{
    if (cimMessage == 0)
    {
        // No message to serialize
        return;
    }

    // ATTN: Need to serialize the Message class members?

    XmlWriter::append(out, "<PGMESSAGE ID=\"");
    XmlWriter::append(out, cimMessage->messageId);
    XmlWriter::append(out, "\" TYPE=\"");
    XmlWriter::append(out, Uint32(cimMessage->getType()));
    XmlWriter::append(out, "\">");

#ifndef PEGASUS_DISABLE_PERFINST
    // Serialize the statistics data

    XmlWriter::appendValueElement(
        out, cimMessage->getServerStartTime());
    XmlWriter::appendValueElement(
        out, cimMessage->getProviderTime());
#endif

    XmlWriter::appendValueElement(out, cimMessage->isComplete());
    XmlWriter::appendValueElement(out, cimMessage->getIndex());

    _serializeOperationContext(out, cimMessage->operationContext);

    CIMRequestMessage* cimReqMessage;
    cimReqMessage = dynamic_cast<CIMRequestMessage*>(cimMessage);

    CIMResponseMessage* cimRespMessage;
    cimRespMessage = dynamic_cast<CIMResponseMessage*>(cimMessage);

    if (cimReqMessage)
    {
        _serializeCIMRequestMessage(out, cimReqMessage);
    }
    else
    {
        PEGASUS_ASSERT(cimRespMessage != 0);
        _serializeCIMResponseMessage(out, cimRespMessage);
    }

    XmlWriter::append(out, "</PGMESSAGE>");
}

//
// _serializeCIMRequestMessage
//
void CIMMessageSerializer::_serializeCIMRequestMessage(
    Buffer& out,
    CIMRequestMessage* cimMessage)
{
    PEGASUS_ASSERT(cimMessage != 0);

    XmlWriter::append(out, "<PGREQ>");

    _serializeQueueIdStack(out, cimMessage->queueIds);

    CIMOperationRequestMessage* cimOpReqMessage;
    cimOpReqMessage = dynamic_cast<CIMOperationRequestMessage*>(cimMessage);

    CIMIndicationRequestMessage* cimIndReqMessage;
    cimIndReqMessage = dynamic_cast<CIMIndicationRequestMessage*>(cimMessage);

    if (cimOpReqMessage)
    {
        XmlWriter::append(out, "<PGOPREQ>\n");

        _serializeUserInfo(
            out, cimOpReqMessage->authType, cimOpReqMessage->userName);
        _serializeCIMNamespaceName(out, cimOpReqMessage->nameSpace);
        _serializeCIMName(out, cimOpReqMessage->className);

        // Encode cimOpReqMessage->providerType as an integer
        XmlWriter::appendValueElement(out, cimOpReqMessage->providerType);

        switch (cimMessage->getType())
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
            _serializeCIMGetInstanceRequestMessage(
                out, (CIMGetInstanceRequestMessage*)cimMessage);
            break;
        case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
            _serializeCIMDeleteInstanceRequestMessage(
                out, (CIMDeleteInstanceRequestMessage*)cimMessage);
            break;
        case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
            _serializeCIMCreateInstanceRequestMessage(
                out, (CIMCreateInstanceRequestMessage*)cimMessage);
            break;
        case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
            _serializeCIMModifyInstanceRequestMessage(
                out, (CIMModifyInstanceRequestMessage*)cimMessage);
            break;
        case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
            _serializeCIMEnumerateInstancesRequestMessage(
                out, (CIMEnumerateInstancesRequestMessage*)cimMessage);
            break;
        case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
            _serializeCIMEnumerateInstanceNamesRequestMessage(
                out, (CIMEnumerateInstanceNamesRequestMessage*)cimMessage);
            break;
        case CIM_EXEC_QUERY_REQUEST_MESSAGE:
            _serializeCIMExecQueryRequestMessage(
                out, (CIMExecQueryRequestMessage*)cimMessage);
            break;

        // Property operations
        case CIM_GET_PROPERTY_REQUEST_MESSAGE:
            _serializeCIMGetPropertyRequestMessage(
                out, (CIMGetPropertyRequestMessage*)cimMessage);
            break;
        case CIM_SET_PROPERTY_REQUEST_MESSAGE:
            _serializeCIMSetPropertyRequestMessage(
                out, (CIMSetPropertyRequestMessage*)cimMessage);
            break;

        // Association operations
        case CIM_ASSOCIATORS_REQUEST_MESSAGE:
            _serializeCIMAssociatorsRequestMessage(
                out, (CIMAssociatorsRequestMessage*)cimMessage);
            break;
        case CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE:
            _serializeCIMAssociatorNamesRequestMessage(
                out, (CIMAssociatorNamesRequestMessage*)cimMessage);
            break;
        case CIM_REFERENCES_REQUEST_MESSAGE:
            _serializeCIMReferencesRequestMessage(
                out, (CIMReferencesRequestMessage*)cimMessage);
            break;
        case CIM_REFERENCE_NAMES_REQUEST_MESSAGE:
            _serializeCIMReferenceNamesRequestMessage(
                out, (CIMReferenceNamesRequestMessage*)cimMessage);
            break;

        // Method operations
        case CIM_INVOKE_METHOD_REQUEST_MESSAGE:
            _serializeCIMInvokeMethodRequestMessage(
                out, (CIMInvokeMethodRequestMessage*)cimMessage);
            break;

        default:
            PEGASUS_ASSERT(0);
        }

        XmlWriter::append(out, "</PGOPREQ>");
    }
    else if (cimIndReqMessage)
    {
        XmlWriter::append(out, "<PGINDREQ>");

        _serializeUserInfo(
            out, cimIndReqMessage->authType, cimIndReqMessage->userName);

        switch (cimMessage->getType())
        {
        case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
            _serializeCIMCreateSubscriptionRequestMessage(
                out, (CIMCreateSubscriptionRequestMessage*)cimMessage);
            break;
        case CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE:
            _serializeCIMModifySubscriptionRequestMessage(
                out, (CIMModifySubscriptionRequestMessage*)cimMessage);
            break;
        case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
            _serializeCIMDeleteSubscriptionRequestMessage(
                out, (CIMDeleteSubscriptionRequestMessage*)cimMessage);
            break;
        default:
            PEGASUS_ASSERT(0);
        }

        XmlWriter::append(out, "</PGINDREQ>");
    }
    else    // Other message types
    {
        XmlWriter::append(out, "<PGOTHERREQ>");

        switch (cimMessage->getType())
        {
        case CIM_EXPORT_INDICATION_REQUEST_MESSAGE:
            _serializeCIMExportIndicationRequestMessage(
                out, (CIMExportIndicationRequestMessage*)cimMessage);
            break;
        case CIM_PROCESS_INDICATION_REQUEST_MESSAGE:
            _serializeCIMProcessIndicationRequestMessage(
                out, (CIMProcessIndicationRequestMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_REGISTRATION_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderRegistrationRequestMessage(
            //    out,
            //    (CIMNotifyProviderRegistrationRequestMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_TERMINATION_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderTerminationRequestMessage(
            //    out, (CIMNotifyProviderTerminationRequestMessage*)cimMessage);
            break;
        //case CIM_HANDLE_INDICATION_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMHandleIndicationRequestMessage(
            //    out, (CIMHandleIndicationRequestMessage*)cimMessage);
            break;
        case CIM_DISABLE_MODULE_REQUEST_MESSAGE:
            _serializeCIMDisableModuleRequestMessage(
                out, (CIMDisableModuleRequestMessage*)cimMessage);
            break;
        case CIM_ENABLE_MODULE_REQUEST_MESSAGE:
            _serializeCIMEnableModuleRequestMessage(
                out, (CIMEnableModuleRequestMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_ENABLE_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderEnableRequestMessage(
            //    out, (CIMNotifyProviderEnableRequestMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_FAIL_REQUEST_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderFailRequestMessage(
            //    out, (CIMNotifyProviderFailRequestMessage*)cimMessage);
            //break;
        case CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE:
            _serializeCIMStopAllProvidersRequestMessage(
                out, (CIMStopAllProvidersRequestMessage*)cimMessage);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_REQUEST_MESSAGE:
            _serializeCIMInitializeProviderAgentRequestMessage(
                out, (CIMInitializeProviderAgentRequestMessage*)cimMessage);
            break;

        case CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE:
            _serializeCIMNotifyConfigChangeRequestMessage(
                out, (CIMNotifyConfigChangeRequestMessage*)cimMessage);
            break;

        case CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE:
            _serializeCIMSubscriptionInitCompleteRequestMessage(
                out,
                (CIMSubscriptionInitCompleteRequestMessage *)
                cimMessage);
            break;

        case CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE:
            _serializeCIMIndicationServiceDisabledRequestMessage(
                out,
                (CIMIndicationServiceDisabledRequestMessage *)
                cimMessage);
            break;

        case PROVAGT_GET_SCMOCLASS_REQUEST_MESSAGE:
            _serializeProvAgtGetScmoClassRequestMessage(
                out,
                (ProvAgtGetScmoClassRequestMessage *)
                cimMessage);
            break;

        default:
            PEGASUS_ASSERT(0);
        }

        XmlWriter::append(out, "</PGOTHERREQ>");
    }

    XmlWriter::append(out, "</PGREQ>");
}

//
// _serializeCIMResponseMessage
//
void CIMMessageSerializer::_serializeCIMResponseMessage(
    Buffer& out,
    CIMResponseMessage* cimMessage)
{
    PEGASUS_ASSERT(cimMessage != 0);

    XmlWriter::append(out, "<PGRESP>\n");

    _serializeQueueIdStack(out, cimMessage->queueIds);
    _serializeCIMException(out, cimMessage->cimException);

    switch (cimMessage->getType())
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
            _serializeCIMGetInstanceResponseMessage(
                out, (CIMGetInstanceResponseMessage*)cimMessage);
            break;
        case CIM_DELETE_INSTANCE_RESPONSE_MESSAGE:
            _serializeCIMDeleteInstanceResponseMessage(
                out, (CIMDeleteInstanceResponseMessage*)cimMessage);
            break;
        case CIM_CREATE_INSTANCE_RESPONSE_MESSAGE:
            _serializeCIMCreateInstanceResponseMessage(
                out, (CIMCreateInstanceResponseMessage*)cimMessage);
            break;
        case CIM_MODIFY_INSTANCE_RESPONSE_MESSAGE:
            _serializeCIMModifyInstanceResponseMessage(
                out, (CIMModifyInstanceResponseMessage*)cimMessage);
            break;
        case CIM_ENUMERATE_INSTANCES_RESPONSE_MESSAGE:
            _serializeCIMEnumerateInstancesResponseMessage(
                out, (CIMEnumerateInstancesResponseMessage*)cimMessage);
            break;
        case CIM_ENUMERATE_INSTANCE_NAMES_RESPONSE_MESSAGE:
            _serializeCIMEnumerateInstanceNamesResponseMessage(
                out, (CIMEnumerateInstanceNamesResponseMessage*)cimMessage);
            break;
        case CIM_EXEC_QUERY_RESPONSE_MESSAGE:
            _serializeCIMExecQueryResponseMessage(
                out, (CIMExecQueryResponseMessage*)cimMessage);
            break;

        // Property operations
        case CIM_GET_PROPERTY_RESPONSE_MESSAGE:
            _serializeCIMGetPropertyResponseMessage(
                out, (CIMGetPropertyResponseMessage*)cimMessage);
            break;
        case CIM_SET_PROPERTY_RESPONSE_MESSAGE:
            _serializeCIMSetPropertyResponseMessage(
                out, (CIMSetPropertyResponseMessage*)cimMessage);
            break;

        // Association operations
        case CIM_ASSOCIATORS_RESPONSE_MESSAGE:
            _serializeCIMAssociatorsResponseMessage(
                out, (CIMAssociatorsResponseMessage*)cimMessage);
            break;
        case CIM_ASSOCIATOR_NAMES_RESPONSE_MESSAGE:
            _serializeCIMAssociatorNamesResponseMessage(
                out, (CIMAssociatorNamesResponseMessage*)cimMessage);
            break;
        case CIM_REFERENCES_RESPONSE_MESSAGE:
            _serializeCIMReferencesResponseMessage(
                out, (CIMReferencesResponseMessage*)cimMessage);
            break;
        case CIM_REFERENCE_NAMES_RESPONSE_MESSAGE:
            _serializeCIMReferenceNamesResponseMessage(
                out, (CIMReferenceNamesResponseMessage*)cimMessage);
            break;

        // Method operations
        case CIM_INVOKE_METHOD_RESPONSE_MESSAGE:
            _serializeCIMInvokeMethodResponseMessage(
                out, (CIMInvokeMethodResponseMessage*)cimMessage);
            break;

        //
        // CIM Indication Response Messages
        //

        case CIM_CREATE_SUBSCRIPTION_RESPONSE_MESSAGE:
            _serializeCIMCreateSubscriptionResponseMessage(
                out, (CIMCreateSubscriptionResponseMessage*)cimMessage);
            break;
        case CIM_MODIFY_SUBSCRIPTION_RESPONSE_MESSAGE:
            _serializeCIMModifySubscriptionResponseMessage(
                out, (CIMModifySubscriptionResponseMessage*)cimMessage);
            break;
        case CIM_DELETE_SUBSCRIPTION_RESPONSE_MESSAGE:
            _serializeCIMDeleteSubscriptionResponseMessage(
                out, (CIMDeleteSubscriptionResponseMessage*)cimMessage);
            break;

        //
        // Other CIM Response Messages
        //

        case CIM_EXPORT_INDICATION_RESPONSE_MESSAGE:
            _serializeCIMExportIndicationResponseMessage(
                out, (CIMExportIndicationResponseMessage*)cimMessage);
            break;
        case CIM_PROCESS_INDICATION_RESPONSE_MESSAGE:
            _serializeCIMProcessIndicationResponseMessage(
                out, (CIMProcessIndicationResponseMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_REGISTRATION_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderRegistrationResponseMessage(
            //    out,
            //    (CIMNotifyProviderRegistrationResponseMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_TERMINATION_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderTerminationResponseMessage(
            //    out,
            //    (CIMNotifyProviderTerminationResponseMessage*)cimMessage);
            break;
        //case CIM_HANDLE_INDICATION_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMHandleIndicationResponseMessage(
            //    out, (CIMHandleIndicationResponseMessage*)cimMessage);
            break;
        case CIM_DISABLE_MODULE_RESPONSE_MESSAGE:
            _serializeCIMDisableModuleResponseMessage(
                out, (CIMDisableModuleResponseMessage*)cimMessage);
            break;
        case CIM_ENABLE_MODULE_RESPONSE_MESSAGE:
            _serializeCIMEnableModuleResponseMessage(
                out, (CIMEnableModuleResponseMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_ENABLE_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderEnableResponseMessage(
            //    out, (CIMNotifyProviderEnableResponseMessage*)cimMessage);
            break;
        //case CIM_NOTIFY_PROVIDER_FAIL_RESPONSE_MESSAGE:
            // ATTN: No need to serialize this yet
            //_serializeCIMNotifyProviderFailResponseMessage(
            //    out, (CIMNotifyProviderFailResponseMessage*)cimMessage);
            //break;
        case CIM_STOP_ALL_PROVIDERS_RESPONSE_MESSAGE:
            _serializeCIMStopAllProvidersResponseMessage(
                out, (CIMStopAllProvidersResponseMessage*)cimMessage);
            break;
        case CIM_INITIALIZE_PROVIDER_AGENT_RESPONSE_MESSAGE:
            _serializeCIMInitializeProviderAgentResponseMessage(
                out, (CIMInitializeProviderAgentResponseMessage*)cimMessage);
            break;
        case CIM_NOTIFY_CONFIG_CHANGE_RESPONSE_MESSAGE:
            _serializeCIMNotifyConfigChangeResponseMessage(
                out, (CIMNotifyConfigChangeResponseMessage*)cimMessage);
            break;
        case CIM_SUBSCRIPTION_INIT_COMPLETE_RESPONSE_MESSAGE:
            _serializeCIMSubscriptionInitCompleteResponseMessage(
                out,
                (CIMSubscriptionInitCompleteResponseMessage *)
                cimMessage);
            break;
        case CIM_INDICATION_SERVICE_DISABLED_RESPONSE_MESSAGE:
            _serializeCIMIndicationServiceDisabledResponseMessage(
                out,
                (CIMIndicationServiceDisabledResponseMessage *)
                cimMessage);
            break;
        case PROVAGT_GET_SCMOCLASS_RESPONSE_MESSAGE:
            _serializeProvAgtGetScmoClassResponseMessage(
                out,
                (ProvAgtGetScmoClassResponseMessage *)
                cimMessage);
            break;

        default:
            PEGASUS_ASSERT(0);
    }

    XmlWriter::append(out, "</PGRESP>");
}


//
// Utility Methods
//

//
// _serializeUserInfo consolidates encoding of these common message attributes
//
void CIMMessageSerializer::_serializeUserInfo(
    Buffer& out,
    const String& authType,
    const String& userName)
{
    XmlWriter::appendValueElement(out, authType);
    XmlWriter::appendValueElement(out, userName);
}

//
// _serializeQueueIdStack
//
void CIMMessageSerializer::_serializeQueueIdStack(
    Buffer& out,
    const QueueIdStack& queueIdStack)
{
    QueueIdStack stackCopy = queueIdStack;

    // Use a PGQIDSTACK element to encapsulate the QueueIdStack encoding
    XmlWriter::append(out, "<PGQIDSTACK>\n");
    while (!stackCopy.isEmpty())
    {
        Uint32 item = stackCopy.top();
        stackCopy.pop();
        XmlWriter::appendValueElement(out, item);
    }
    XmlWriter::append(out, "</PGQIDSTACK>\n");
}

//
// _serializeOperationContext
//
void CIMMessageSerializer::_serializeOperationContext(
    Buffer& out,
    const OperationContext& operationContext)
{
    // Use a PGOP element to encapsulate the OperationContext encoding
    XmlWriter::append(out, "<PGOC>\n");

    // Note: OperationContext class does not allow iteration through Containers

    if (operationContext.contains(IdentityContainer::NAME))
    {
        const IdentityContainer container =
            operationContext.get(IdentityContainer::NAME);

        XmlWriter::append(out, "<PGOCID>\n");
        XmlWriter::appendValueElement(out, container.getUserName());
        XmlWriter::append(out, "</PGOCID>\n");
    }

    if (operationContext.contains(SubscriptionInstanceContainer::NAME))
    {
        const SubscriptionInstanceContainer container =
            operationContext.get(SubscriptionInstanceContainer::NAME);

        XmlWriter::append(out, "<PGOCSI>\n");
        _serializeCIMInstance(out, container.getInstance());
        XmlWriter::append(out, "</PGOCSI>\n");
    }

    if (operationContext.contains(SubscriptionFilterConditionContainer::NAME))
    {
        const SubscriptionFilterConditionContainer container =
            operationContext.get(SubscriptionFilterConditionContainer::NAME);

        XmlWriter::append(out, "<PGOCSFC>\n");
        XmlWriter::appendValueElement(out, container.getFilterCondition());
        XmlWriter::appendValueElement(out, container.getQueryLanguage());
        XmlWriter::append(out, "</PGOCSFC>\n");
    }

    if (operationContext.contains(SubscriptionFilterQueryContainer::NAME))
    {
        const SubscriptionFilterQueryContainer container =
            operationContext.get(SubscriptionFilterQueryContainer::NAME);

        XmlWriter::append(out, "<PGOCSFQ>\n");
        XmlWriter::appendValueElement(out, container.getFilterQuery());
        XmlWriter::appendValueElement(out, container.getQueryLanguage());
        _serializeCIMNamespaceName(out, container.getSourceNameSpace());
        XmlWriter::append(out, "</PGOCSFQ>\n");
    }

    if (operationContext.contains(SubscriptionInstanceNamesContainer::NAME))
    {
        const SubscriptionInstanceNamesContainer container =
            operationContext.get(SubscriptionInstanceNamesContainer::NAME);

        XmlWriter::append(out, "<PGOCSIN>\n");

        Array<CIMObjectPath> cimObjectPaths = container.getInstanceNames();
        for (Uint32 i=0; i < cimObjectPaths.size(); i++)
        {
            _serializeCIMObjectPath(out, cimObjectPaths[i]);
        }

        XmlWriter::append(out, "</PGOCSIN>\n");
    }

    if (operationContext.contains(TimeoutContainer::NAME))
    {
        const TimeoutContainer container =
            operationContext.get(TimeoutContainer::NAME);

        XmlWriter::append(out, "<PGOCTO>\n");
        XmlWriter::appendValueElement(out, container.getTimeOut());
        XmlWriter::append(out, "</PGOCTO>\n");
    }

    if (operationContext.contains(AcceptLanguageListContainer::NAME))
    {
        const AcceptLanguageListContainer container =
            operationContext.get(AcceptLanguageListContainer::NAME);

        XmlWriter::append(out, "<PGOCALL>\n");
        _serializeAcceptLanguageList(out, container.getLanguages());
        XmlWriter::append(out, "</PGOCALL>\n");
    }

    if (operationContext.contains(ContentLanguageListContainer::NAME))
    {
        const ContentLanguageListContainer container =
            operationContext.get(ContentLanguageListContainer::NAME);

        XmlWriter::append(out, "<PGOCCLL>\n");
        _serializeContentLanguageList(out, container.getLanguages());
        XmlWriter::append(out, "</PGOCCLL>\n");
    }

    if (operationContext.contains(SnmpTrapOidContainer::NAME))
    {
        const SnmpTrapOidContainer container =
            operationContext.get(SnmpTrapOidContainer::NAME);

        XmlWriter::append(out, "<PGOCSTO>\n");
        XmlWriter::appendValueElement(out, container.getSnmpTrapOid());
        XmlWriter::append(out, "</PGOCSTO>\n");
    }

    if (operationContext.contains(LocaleContainer::NAME))
    {
        const LocaleContainer container =
            operationContext.get(LocaleContainer::NAME);

        XmlWriter::append(out, "<PGOCL>\n");
        XmlWriter::appendValueElement(out, container.getLanguageId());
        XmlWriter::append(out, "</PGOCL>\n");
    }

    if (operationContext.contains(ProviderIdContainer::NAME))
    {
        const ProviderIdContainer container =
            operationContext.get(ProviderIdContainer::NAME);

        XmlWriter::append(out, "<PGOCPI>\n");
        _serializeCIMInstance(out, container.getModule());
        _serializeCIMInstance(out, container.getProvider());
        XmlWriter::appendValueElement(out, container.isRemoteNameSpace());
        XmlWriter::appendValueElement(out, container.getRemoteInfo());
        XmlWriter::appendValueElement(out, container.getProvMgrPath());
        XmlWriter::append(out, "</PGOCPI>\n");
    }

    if (operationContext.contains(CachedClassDefinitionContainer::NAME))
    {
        const CachedClassDefinitionContainer container =
            operationContext.get(CachedClassDefinitionContainer::NAME);

        CIMConstClass cimClass = container.getClass();
        XmlWriter::append(out, "<PGOCCCD>\n");
        XmlWriter::appendClassElement(out, cimClass);
        XmlWriter::append(out, "</PGOCCCD>\n");
    }

    XmlWriter::append(out, "</PGOC>\n");
}

//
// _serializeContentLanguageList
//
void CIMMessageSerializer::_serializeContentLanguageList(
    Buffer& out,
    const ContentLanguageList& contentLanguages)
{
    // Use a PGCONTLANGS element to encapsulate the ContentLanguageList
    // encoding
    XmlWriter::append(out, "<PGCONTLANGS>\n");
    for (Uint32 i=0; i < contentLanguages.size(); i++)
    {
        XmlWriter::appendValueElement(
            out, contentLanguages.getLanguageTag(i).toString());
    }
    XmlWriter::append(out, "</PGCONTLANGS>\n");
}

//
// _serializeAcceptLanguageList
//
void CIMMessageSerializer::_serializeAcceptLanguageList(
    Buffer& out,
    const AcceptLanguageList& acceptLanguages)
{
    // Use a PGACCLANGS element to encapsulate the AcceptLanguageList encoding
    XmlWriter::append(out, "<PGACCLANGS>\n");
    for (Uint32 i=0; i < acceptLanguages.size(); i++)
    {
        XmlWriter::appendValueElement(
            out, acceptLanguages.getLanguageTag(i).toString());
        XmlWriter::appendValueElement(out, acceptLanguages.getQualityValue(i));
    }
    XmlWriter::append(out, "</PGACCLANGS>\n");
}

//
// _serializeCIMException
//
void CIMMessageSerializer::_serializeCIMException(
    Buffer& out,
    const CIMException& cimException)
{
    TraceableCIMException e(cimException);

    // Use a PGCIMEXC element to encapsulate the CIMException encoding
    // (Note: This is not truly necessary and could be removed.)
    XmlWriter::append(out, "<PGCIMEXC>\n");

    XmlWriter::appendValueElement(out, Uint32(e.getCode()));
    XmlWriter::appendValueElement(out, e.getMessage());
    XmlWriter::appendValueElement(out, e.getCIMMessage());
    XmlWriter::appendValueElement(out, e.getFile());
    XmlWriter::appendValueElement(out, e.getLine());
    _serializeContentLanguageList(out, e.getContentLanguages());

    XmlWriter::append(out, "</PGCIMEXC>\n");
}

//
// _serializeCIMPropertyList
//
void CIMMessageSerializer::_serializeCIMPropertyList(
    Buffer& out,
    const CIMPropertyList& cimPropertyList)
{
    // Need IPARAMVALUE wrapper because the value can be null.
    XmlWriter::appendPropertyListIParameter(out, cimPropertyList);
}

//
// _serializeCIMObjectPath
//
void CIMMessageSerializer::_serializeCIMObjectPath(
    Buffer& out,
    const CIMObjectPath& cimObjectPath)
{
    // Use a PGPATH element to encapsulate the CIMObjectPath encoding
    // to account for uninitialized objects
    XmlWriter::append(out, "<PGPATH>\n");
    if (!cimObjectPath.getClassName().isNull())
    {
        XmlWriter::appendValueReferenceElement(out, cimObjectPath, true);
    }
    XmlWriter::append(out, "</PGPATH>\n");
}

//
// _serializeCIMInstance
//
void CIMMessageSerializer::_serializeCIMInstance(
    Buffer& out,
    const CIMInstance& cimInstance)
{
    // Use a PGINST element to encapsulate the CIMInstance encoding
    // to account for uninitialized objects
    XmlWriter::append(out, "<PGINST>\n");
    if (!cimInstance.isUninitialized())
    {
        XmlWriter::appendInstanceElement(out, cimInstance);
        _serializeCIMObjectPath(out, cimInstance.getPath());
    }
    XmlWriter::append(out, "</PGINST>\n");
}

//
// _serializeCIMNamespaceName
//
void CIMMessageSerializer::_serializeCIMNamespaceName(
    Buffer& out,
    const CIMNamespaceName& cimNamespaceName)
{
    // Encode CIMNamespaceName as a String for efficiency and so that null
    // values can be handled
    XmlWriter::appendValueElement(out, cimNamespaceName.getString());
}

//
// _serializeCIMName
//
void CIMMessageSerializer::_serializeCIMName(
    Buffer& out,
    const CIMName& cimName)
{
    // Encode CIMName as a String so that null values can be handled
    XmlWriter::appendValueElement(out, cimName.getString());
}

//
// _serializeCIMObject
//
void CIMMessageSerializer::_serializeCIMObject(
    Buffer& out,
    const CIMObject& object)
{
    // Use a PGOBJ element to encapsulate the CIMObject encoding
    // to account for uninitialized objects
    XmlWriter::append(out, "<PGOBJ>\n");
    if (!object.isUninitialized())
    {
        XmlWriter::appendObjectElement(out, object);
        _serializeCIMObjectPath(out, object.getPath());
    }
    XmlWriter::append(out, "</PGOBJ>\n");
}

//
// _serializeCIMParamValue
//
void CIMMessageSerializer::_serializeCIMParamValue(
    Buffer& out,
    const CIMParamValue& paramValue)
{
    if (paramValue.getValue().isNull())
    {
        // The CIM-XML encoding does not preserve type information for null
        // parameter values, so use our own PGNULLPARAMVALUE element to encode
        // null parameter values with their type information.

        out << STRLIT("<PGNULLPARAMVALUE PARAMTYPE=\"")
            << cimTypeToString(paramValue.getValue().getType())
            << STRLIT("\">\n");

        XmlWriter::appendValueElement(out, paramValue.getParameterName());
        XmlWriter::appendValueElement(out, paramValue.getValue().isArray());

        out << STRLIT("</PGNULLPARAMVALUE>\n");
    }
    else
    {
        XmlWriter::appendParamValueElement(out, paramValue);
    }
}


//
//
// Request Messages
//
//

//
//
// CIMOperationRequestMessages
//
//

//
// _serializeCIMGetInstanceRequestMessage
//
void CIMMessageSerializer::_serializeCIMGetInstanceRequestMessage(
    Buffer& out,
    CIMGetInstanceRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->instanceName);
    XmlWriter::appendValueElement(out, message->includeQualifiers);
    XmlWriter::appendValueElement(out, message->includeClassOrigin);
    _serializeCIMPropertyList(out, message->propertyList);
}

//
// _serializeCIMDeleteInstanceRequestMessage
//
void CIMMessageSerializer::_serializeCIMDeleteInstanceRequestMessage(
    Buffer& out,
    CIMDeleteInstanceRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->instanceName);
}

//
// _serializeCIMCreateInstanceRequestMessage
//
void CIMMessageSerializer::_serializeCIMCreateInstanceRequestMessage(
    Buffer& out,
    CIMCreateInstanceRequestMessage* message)
{
    _serializeCIMInstance(out, message->newInstance);
}

//
// _serializeCIMModifyInstanceRequestMessage
//
void CIMMessageSerializer::_serializeCIMModifyInstanceRequestMessage(
    Buffer& out,
    CIMModifyInstanceRequestMessage* message)
{
    _serializeCIMInstance(out, message->modifiedInstance);
    XmlWriter::appendValueElement(out, message->includeQualifiers);
    _serializeCIMPropertyList(out, message->propertyList);
}

//
// _serializeCIMEnumerateInstancesRequestMessage
//
void CIMMessageSerializer::_serializeCIMEnumerateInstancesRequestMessage(
    Buffer& out,
    CIMEnumerateInstancesRequestMessage* message)
{
    XmlWriter::appendValueElement(out, message->deepInheritance);
    XmlWriter::appendValueElement(out, message->includeQualifiers);
    XmlWriter::appendValueElement(out, message->includeClassOrigin);
    _serializeCIMPropertyList(out, message->propertyList);
}

//
// _serializeCIMEnumerateInstanceNamesRequestMessage
//
void CIMMessageSerializer::_serializeCIMEnumerateInstanceNamesRequestMessage(
    Buffer& out,
    CIMEnumerateInstanceNamesRequestMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMExecQueryRequestMessage
//
void CIMMessageSerializer::_serializeCIMExecQueryRequestMessage(
    Buffer& out,
    CIMExecQueryRequestMessage* message)
{
    XmlWriter::appendValueElement(out, message->queryLanguage);
    XmlWriter::appendValueElement(out, message->query);
}

//
// _serializeCIMAssociatorsRequestMessage
//
void CIMMessageSerializer::_serializeCIMAssociatorsRequestMessage(
    Buffer& out,
    CIMAssociatorsRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->objectName);
    _serializeCIMName(out, message->assocClass);
    _serializeCIMName(out, message->resultClass);
    XmlWriter::appendValueElement(out, message->role);
    XmlWriter::appendValueElement(out, message->resultRole);
    XmlWriter::appendValueElement(out, message->includeQualifiers);
    XmlWriter::appendValueElement(out, message->includeClassOrigin);
    _serializeCIMPropertyList(out, message->propertyList);
}

//
// _serializeCIMAssociatorNamesRequestMessage
//
void CIMMessageSerializer::_serializeCIMAssociatorNamesRequestMessage(
    Buffer& out,
    CIMAssociatorNamesRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->objectName);
    _serializeCIMName(out, message->assocClass);
    _serializeCIMName(out, message->resultClass);
    XmlWriter::appendValueElement(out, message->role);
    XmlWriter::appendValueElement(out, message->resultRole);
}

//
// _serializeCIMReferencesRequestMessage
//
void CIMMessageSerializer::_serializeCIMReferencesRequestMessage(
    Buffer& out,
    CIMReferencesRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->objectName);
    _serializeCIMName(out, message->resultClass);
    XmlWriter::appendValueElement(out, message->role);
    XmlWriter::appendValueElement(out, message->includeQualifiers);
    XmlWriter::appendValueElement(out, message->includeClassOrigin);
    _serializeCIMPropertyList(out, message->propertyList);
}

//
// _serializeCIMReferenceNamesRequestMessage
//
void CIMMessageSerializer::_serializeCIMReferenceNamesRequestMessage(
    Buffer& out,
    CIMReferenceNamesRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->objectName);
    _serializeCIMName(out, message->resultClass);
    XmlWriter::appendValueElement(out, message->role);
}

//
// _serializeCIMGetPropertyRequestMessage
//
void CIMMessageSerializer::_serializeCIMGetPropertyRequestMessage(
    Buffer& out,
    CIMGetPropertyRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->instanceName);
    _serializeCIMName(out, message->propertyName);
}

//
// _serializeCIMSetPropertyRequestMessage
//
void CIMMessageSerializer::_serializeCIMSetPropertyRequestMessage(
    Buffer& out,
    CIMSetPropertyRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->instanceName);

    // Use PARAMVALUE element so we can preserve the CIMType information
    _serializeCIMParamValue(
        out,
        CIMParamValue(
            message->propertyName.getString(), message->newValue, true));
}

//
// _serializeCIMInvokeMethodRequestMessage
//
void CIMMessageSerializer::_serializeCIMInvokeMethodRequestMessage(
    Buffer& out,
    CIMInvokeMethodRequestMessage* message)
{
    _serializeCIMObjectPath(out, message->instanceName);
    _serializeCIMName(out, message->methodName);

    // Use PGPARAMS element so we can find the end of the PARAMVALUE elements
    XmlWriter::append(out, "<PGPARAMS>\n");

    for (Uint32 i=0; i < message->inParameters.size(); i++)
    {
        _serializeCIMParamValue(out, message->inParameters[i]);
    }

    XmlWriter::append(out, "</PGPARAMS>\n");
}


//
//
// CIMIndicationRequestMessages
//
//

//
// _serializeCIMCreateSubscriptionRequestMessage
//
void CIMMessageSerializer::_serializeCIMCreateSubscriptionRequestMessage(
    Buffer& out,
    CIMCreateSubscriptionRequestMessage* message)
{
    _serializeCIMNamespaceName(out, message->nameSpace);

    _serializeCIMInstance(out, message->subscriptionInstance);

    // Use PGNAMEARRAY element to encapsulate the CIMName elements
    XmlWriter::append(out, "<PGNAMEARRAY>\n");
    for (Uint32 i=0; i < message->classNames.size(); i++)
    {
        _serializeCIMName(out, message->classNames[i]);
    }
    XmlWriter::append(out, "</PGNAMEARRAY>\n");

    _serializeCIMPropertyList(out, message->propertyList);

    // Encode message->repeatNotificationPolicy as an integer
    XmlWriter::appendValueElement(out, message->repeatNotificationPolicy);

    XmlWriter::appendValueElement(out, message->query);
}

//
// _serializeCIMModifySubscriptionRequestMessage
//
void CIMMessageSerializer::_serializeCIMModifySubscriptionRequestMessage(
    Buffer& out,
    CIMModifySubscriptionRequestMessage* message)
{
    _serializeCIMNamespaceName(out, message->nameSpace);

    _serializeCIMInstance(out, message->subscriptionInstance);

    // Use PGNAMEARRAY element to encapsulate the CIMName elements
    XmlWriter::append(out, "<PGNAMEARRAY>\n");
    for (Uint32 i=0; i < message->classNames.size(); i++)
    {
        _serializeCIMName(out, message->classNames[i]);
    }
    XmlWriter::append(out, "</PGNAMEARRAY>\n");

    _serializeCIMPropertyList(out, message->propertyList);

    // Encode message->repeatNotificationPolicy as an integer
    XmlWriter::appendValueElement(out, message->repeatNotificationPolicy);

    XmlWriter::appendValueElement(out, message->query);
}

//
// _serializeCIMDeleteSubscriptionRequestMessage
//
void CIMMessageSerializer::_serializeCIMDeleteSubscriptionRequestMessage(
    Buffer& out,
    CIMDeleteSubscriptionRequestMessage* message)
{
    _serializeCIMNamespaceName(out, message->nameSpace);

    _serializeCIMInstance(out, message->subscriptionInstance);

    // Use PGNAMEARRAY element to encapsulate the CIMName elements
    XmlWriter::append(out, "<PGNAMEARRAY>\n");
    for (Uint32 i=0; i < message->classNames.size(); i++)
    {
        _serializeCIMName(out, message->classNames[i]);
    }
    XmlWriter::append(out, "</PGNAMEARRAY>\n");
}


//
//
// Other CIMRequestMessages
//
//

//
// _serializeCIMExportIndicationRequestMessage
//
void CIMMessageSerializer::_serializeCIMExportIndicationRequestMessage(
    Buffer& out,
    CIMExportIndicationRequestMessage* message)
{
    _serializeUserInfo(out, message->authType, message->userName);

    XmlWriter::appendValueElement(out, message->destinationPath);
    _serializeCIMInstance(out, message->indicationInstance);
}

//
// _serializeCIMProcessIndicationRequestMessage
//
void CIMMessageSerializer::_serializeCIMProcessIndicationRequestMessage(
    Buffer& out,
    CIMProcessIndicationRequestMessage* message)
{
    _serializeCIMNamespaceName(out, message->nameSpace);

    _serializeCIMInstance(out, message->indicationInstance);

    // Use PGPATHARRAY element to encapsulate the object path elements
    XmlWriter::append(out, "<PGPATHARRAY>\n");
    for (Uint32 i=0; i < message->subscriptionInstanceNames.size(); i++)
    {
        _serializeCIMObjectPath(out, message->subscriptionInstanceNames[i]);
    }
    XmlWriter::append(out, "</PGPATHARRAY>\n");

    _serializeCIMInstance(out, message->provider);
}

//
// _serializeCIMDisableModuleRequestMessage
//
void CIMMessageSerializer::_serializeCIMDisableModuleRequestMessage(
    Buffer& out,
    CIMDisableModuleRequestMessage* message)
{
    _serializeUserInfo(out, message->authType, message->userName);

    _serializeCIMInstance(out, message->providerModule);

    // Use PGINSTARRAY element to encapsulate the CIMInstance elements
    XmlWriter::append(out, "<PGINSTARRAY>\n");
    for (Uint32 i=0; i < message->providers.size(); i++)
    {
        _serializeCIMInstance(out, message->providers[i]);
    }
    XmlWriter::append(out, "</PGINSTARRAY>\n");

    XmlWriter::appendValueElement(out, message->disableProviderOnly);

    // Use PGBOOLARRAY element to encapsulate the Boolean elements
    XmlWriter::append(out, "<PGBOOLARRAY>\n");
    for (Uint32 i=0; i < message->indicationProviders.size(); i++)
    {
        XmlWriter::appendValueElement(out, message->indicationProviders[i]);
    }
    XmlWriter::append(out, "</PGBOOLARRAY>\n");
}

//
// _serializeCIMEnableModuleRequestMessage
//
void CIMMessageSerializer::_serializeCIMEnableModuleRequestMessage(
    Buffer& out,
    CIMEnableModuleRequestMessage* message)
{
    _serializeUserInfo(out, message->authType, message->userName);

    _serializeCIMInstance(out, message->providerModule);
}

//
// _serializeCIMStopAllProvidersRequestMessage
//
void CIMMessageSerializer::_serializeCIMStopAllProvidersRequestMessage(
    Buffer& out,
    CIMStopAllProvidersRequestMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMInitializeProviderAgentRequestMessage
//
void CIMMessageSerializer::_serializeCIMInitializeProviderAgentRequestMessage(
    Buffer& out,
    CIMInitializeProviderAgentRequestMessage* message)
{
    XmlWriter::appendValueElement(out, message->pegasusHome);

    // Use PGCONFARRAY element to encapsulate the config property elements
    XmlWriter::append(out, "<PGCONFARRAY>\n");
    for (Uint32 i=0; i < message->configProperties.size(); i++)
    {
        XmlWriter::appendValueElement(out, message->configProperties[i].first);
        XmlWriter::appendValueElement(out, message->configProperties[i].second);
    }
    XmlWriter::append(out, "</PGCONFARRAY>\n");

    XmlWriter::appendValueElement(out, message->bindVerbose);

    XmlWriter::appendValueElement(out, message->subscriptionInitComplete);
}

//
// _serializeCIMNotifyConfigChangeRequestMessage
//
void CIMMessageSerializer::_serializeCIMNotifyConfigChangeRequestMessage(
    Buffer& out,
    CIMNotifyConfigChangeRequestMessage* message)
{
    XmlWriter::appendValueElement(out, message->propertyName);
    XmlWriter::appendValueElement(out, message->newPropertyValue);
    XmlWriter::appendValueElement(out, message->currentValueModified);
}

//
// _serializeCIMSubscriptionInitCompleteRequestMessage
//
void CIMMessageSerializer::_serializeCIMSubscriptionInitCompleteRequestMessage(
    Buffer& out,
    CIMSubscriptionInitCompleteRequestMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMIndicationServiceDisabledRequestMessage
//
void CIMMessageSerializer::_serializeCIMIndicationServiceDisabledRequestMessage(
    Buffer& out,
    CIMIndicationServiceDisabledRequestMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeProvAgtGetScmoClassRequestMessage
//
void CIMMessageSerializer::_serializeProvAgtGetScmoClassRequestMessage(
    Buffer& out,
    ProvAgtGetScmoClassRequestMessage* message)
{
    _serializeCIMNamespaceName(out, message->nameSpace);

    _serializeCIMName(out, message->className);
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
// _serializeCIMGetInstanceResponseMessage
//
void CIMMessageSerializer::_serializeCIMGetInstanceResponseMessage(
    Buffer& out,
    CIMGetInstanceResponseMessage* message)
{
    _serializeCIMInstance(out, message->getResponseData().getInstance());
}

//
// _serializeCIMDeleteInstanceResponseMessage
//
void CIMMessageSerializer::_serializeCIMDeleteInstanceResponseMessage(
    Buffer& out,
    CIMDeleteInstanceResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMCreateInstanceResponseMessage
//
void CIMMessageSerializer::_serializeCIMCreateInstanceResponseMessage(
    Buffer& out,
    CIMCreateInstanceResponseMessage* message)
{
    _serializeCIMObjectPath(out, message->instanceName);
}

//
// _serializeCIMModifyInstanceResponseMessage
//
void CIMMessageSerializer::_serializeCIMModifyInstanceResponseMessage(
    Buffer& out,
    CIMModifyInstanceResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMEnumerateInstancesResponseMessage
//
void CIMMessageSerializer::_serializeCIMEnumerateInstancesResponseMessage(
    Buffer& out,
    CIMEnumerateInstancesResponseMessage* message)
{
    // Use PGINSTARRAY element to encapsulate the CIMInstance elements
    XmlWriter::append(out, "<PGINSTARRAY>\n");

    const Array<CIMInstance>& a = message->getResponseData().getInstances();
    for (Uint32 i=0; i < a.size(); i++)
    {
        _serializeCIMInstance(out, a[i]);
    }
    XmlWriter::append(out, "</PGINSTARRAY>\n");
}

//
// _serializeCIMEnumerateInstanceNamesResponseMessage
//
void CIMMessageSerializer::_serializeCIMEnumerateInstanceNamesResponseMessage(
    Buffer& out,
    CIMEnumerateInstanceNamesResponseMessage* message)
{
    // Use PGPATHARRAY element to encapsulate the object path elements
    XmlWriter::append(out, "<PGPATHARRAY>\n");
    for (Uint32 i=0;i<message->getResponseData().getInstanceNames().size();i++)
    {
        _serializeCIMObjectPath(
            out,
            message->getResponseData().getInstanceNames()[i]);
    }
    XmlWriter::append(out, "</PGPATHARRAY>\n");
}

//
// _serializeCIMExecQueryResponseMessage
//
void CIMMessageSerializer::_serializeCIMExecQueryResponseMessage(
    Buffer& out,
    CIMExecQueryResponseMessage* message)
{
    // Use PGOBJARRAY element to encapsulate the CIMObject elements
    XmlWriter::append(out, "<PGOBJARRAY>\n");
    for (Uint32 i=0; i < message->getResponseData().getObjects().size(); i++)
    {
        _serializeCIMObject(out, message->getResponseData().getObjects()[i]);
    }
    XmlWriter::append(out, "</PGOBJARRAY>\n");
}

//
// _serializeCIMAssociatorsResponseMessage
//
void CIMMessageSerializer::_serializeCIMAssociatorsResponseMessage(
    Buffer& out,
    CIMAssociatorsResponseMessage* message)
{
    // Use PGOBJARRAY element to encapsulate the CIMObject elements
    XmlWriter::append(out, "<PGOBJARRAY>\n");
    for (Uint32 i=0; i < message->getResponseData().getObjects().size(); i++)
    {
        _serializeCIMObject(out, message->getResponseData().getObjects()[i]);
    }
    XmlWriter::append(out, "</PGOBJARRAY>\n");
}

//
// _serializeCIMAssociatorNamesResponseMessage
//
void CIMMessageSerializer::_serializeCIMAssociatorNamesResponseMessage(
    Buffer& out,
    CIMAssociatorNamesResponseMessage* message)
{
    // Use PGPATHARRAY element to encapsulate the object path elements
    XmlWriter::append(out, "<PGPATHARRAY>\n");
    for (Uint32 i=0;i<message->getResponseData().getInstanceNames().size();i++)
    {
        _serializeCIMObjectPath(
            out,
            message->getResponseData().getInstanceNames()[i]);
    }
    XmlWriter::append(out, "</PGPATHARRAY>\n");
}

//
// _serializeCIMReferencesResponseMessage
//
void CIMMessageSerializer::_serializeCIMReferencesResponseMessage(
    Buffer& out,
    CIMReferencesResponseMessage* message)
{
    // Use PGOBJARRAY element to encapsulate the CIMObject elements
    XmlWriter::append(out, "<PGOBJARRAY>\n");
    for (Uint32 i=0; i < message->getResponseData().getObjects().size(); i++)
    {
        _serializeCIMObject(out, message->getResponseData().getObjects()[i]);
    }
    XmlWriter::append(out, "</PGOBJARRAY>\n");
}

//
// _serializeCIMReferenceNamesResponseMessage
//
void CIMMessageSerializer::_serializeCIMReferenceNamesResponseMessage(
    Buffer& out,
    CIMReferenceNamesResponseMessage* message)
{
    // Use PGPATHARRAY element to encapsulate the object path elements
    XmlWriter::append(out, "<PGPATHARRAY>\n");
    for (Uint32 i=0;i<message->getResponseData().getInstanceNames().size();i++)
    {
        _serializeCIMObjectPath(
            out,
            message->getResponseData().getInstanceNames()[i]);
    }
    XmlWriter::append(out, "</PGPATHARRAY>\n");
}

//
// _serializeCIMGetPropertyResponseMessage
//
void CIMMessageSerializer::_serializeCIMGetPropertyResponseMessage(
    Buffer& out,
    CIMGetPropertyResponseMessage* message)
{
    // Use PARAMVALUE element so we can preserve the CIMType information
    _serializeCIMParamValue(
        out,
        CIMParamValue(String("ignore"), message->value, true));
}

//
// _serializeCIMSetPropertyResponseMessage
//
void CIMMessageSerializer::_serializeCIMSetPropertyResponseMessage(
    Buffer& out,
    CIMSetPropertyResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMInvokeMethodResponseMessage
//
void CIMMessageSerializer::_serializeCIMInvokeMethodResponseMessage(
    Buffer& out,
    CIMInvokeMethodResponseMessage* message)
{
    // Use PARAMVALUE element so we can preserve the CIMType information
    _serializeCIMParamValue(
        out,
        CIMParamValue(String("ignore"), message->retValue, true));

    // Use PGPARAMS element so we can find the end of the PARAMVALUE elements
    XmlWriter::append(out, "<PGPARAMS>\n");

    for (Uint32 i=0; i < message->outParameters.size(); i++)
    {
        _serializeCIMParamValue(out, message->outParameters[i]);
    }

    XmlWriter::append(out, "</PGPARAMS>\n");

    _serializeCIMName(out, message->methodName);
}


//
//
// CIM Indication Response Messages
//
//

//
// _serializeCIMCreateSubscriptionResponseMessage
//
void CIMMessageSerializer::_serializeCIMCreateSubscriptionResponseMessage(
    Buffer& out,
    CIMCreateSubscriptionResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMModifySubscriptionResponseMessage
//
void CIMMessageSerializer::_serializeCIMModifySubscriptionResponseMessage(
    Buffer& out,
    CIMModifySubscriptionResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMDeleteSubscriptionResponseMessage
//
void CIMMessageSerializer::_serializeCIMDeleteSubscriptionResponseMessage(
    Buffer& out,
    CIMDeleteSubscriptionResponseMessage* message)
{
    // No additional attributes to serialize!
}


//
//
// Other CIMResponseMessages
//
//

//
// _serializeCIMExportIndicationResponseMessage
//
void CIMMessageSerializer::_serializeCIMExportIndicationResponseMessage(
    Buffer& out,
    CIMExportIndicationResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMProcessIndicationResponseMessage
//
void CIMMessageSerializer::_serializeCIMProcessIndicationResponseMessage(
    Buffer& out,
    CIMProcessIndicationResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMDisableModuleResponseMessage
//
void CIMMessageSerializer::_serializeCIMDisableModuleResponseMessage(
    Buffer& out,
    CIMDisableModuleResponseMessage* message)
{
    // Use PGUINT16ARRAY element to encapsulate the Uint16 elements
    XmlWriter::append(out, "<PGUINT16ARRAY>\n");
    for (Uint32 i=0; i < message->operationalStatus.size(); i++)
    {
        XmlWriter::appendValueElement(out, message->operationalStatus[i]);
    }
    XmlWriter::append(out, "</PGUINT16ARRAY>\n");
}

//
// _serializeCIMEnableModuleResponseMessage
//
void CIMMessageSerializer::_serializeCIMEnableModuleResponseMessage(
    Buffer& out,
    CIMEnableModuleResponseMessage* message)
{
    // Use PGUINT16ARRAY element to encapsulate the Uint16 elements
    XmlWriter::append(out, "<PGUINT16ARRAY>\n");
    for (Uint32 i=0; i < message->operationalStatus.size(); i++)
    {
        XmlWriter::appendValueElement(out, message->operationalStatus[i]);
    }
    XmlWriter::append(out, "</PGUINT16ARRAY>\n");
}

//
// _serializeCIMStopAllProvidersResponseMessage
//
void CIMMessageSerializer::_serializeCIMStopAllProvidersResponseMessage(
    Buffer& out,
    CIMStopAllProvidersResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMInitializeProviderAgentResponseMessage
//
void CIMMessageSerializer::_serializeCIMInitializeProviderAgentResponseMessage(
    Buffer& out,
    CIMInitializeProviderAgentResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMNotifyConfigChangeResponseMessage
//
void CIMMessageSerializer::_serializeCIMNotifyConfigChangeResponseMessage(
    Buffer& out,
    CIMNotifyConfigChangeResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMSubscriptionInitCompleteResponseMessage
//
void CIMMessageSerializer::_serializeCIMSubscriptionInitCompleteResponseMessage(
    Buffer& out,
    CIMSubscriptionInitCompleteResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeCIMIndicationServiceDisabledResponseMessage
//
void
CIMMessageSerializer::_serializeCIMIndicationServiceDisabledResponseMessage(
        Buffer& out,
        CIMIndicationServiceDisabledResponseMessage* message)
{
    // No additional attributes to serialize!
}

//
// _serializeProvAgtGetScmoClassResponseMessage
//
void CIMMessageSerializer::_serializeProvAgtGetScmoClassResponseMessage(
        Buffer& out,
        ProvAgtGetScmoClassResponseMessage* message)
{
   CIMClass cimClass;
   if (!message->scmoClass.isEmpty())
   {
       message->scmoClass.getCIMClass(cimClass);
       XmlWriter::appendClassElement(out, cimClass);
       // The namespace has to be added separately, because a XML encodes a
       // CIMClass without, but it is needed in a SCMOClass.
       _serializeCIMNamespaceName(out,cimClass.getPath().getNameSpace());

   }

}

PEGASUS_NAMESPACE_END
