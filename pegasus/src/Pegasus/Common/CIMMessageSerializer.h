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

#ifndef Pegasus_CIMMessageSerializer_h
#define Pegasus_CIMMessageSerializer_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/CIMMessage.h>

PEGASUS_NAMESPACE_BEGIN

/**
    CIMMessageSerializer provides a mechanism to convert a CIMMessage (or one
    of its subclasses) to a stream of bytes.  This stream of bytes can be
    converted back to a CIMMessage object through use of the related
    CIMMessageDeserializer class.

    The format of the serialized message is not defined and is therefore
    subject to change.  The only requirement is that the CIMMessageSerializer
    and CIMMessageDeserializer classes remain in sync to provide a two-way
    mapping.  (A quasi-XML encoding is currently used as an expedient
    solution.  However, this encoding is not compliant with the CIM-XML
    specification.  A number of shortcuts have been taken to improve
    operational efficiency.)

    Note:  Changes or additions to the CIMMessage definitions must be
    reflected in these serialization classes.  Likewise, changes to the
    structure of member data (such as the AcceptLanguageList class) and
    addition of new OperationContext containers will affect message
    serialization.
 */
class PEGASUS_COMMON_LINKAGE CIMMessageSerializer
{
public:

    static void serialize(Buffer& out, CIMMessage* cimMessage);

private:

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMRequestMessage(
        Buffer& out,
        CIMRequestMessage* cimMessage);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMResponseMessage(
        Buffer& out,
        CIMResponseMessage* cimMessage);

    //
    // Utility Methods
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeUserInfo(
        Buffer& out,
        const String& authType,
        const String& userName);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeQueueIdStack(
        Buffer& out,
        const QueueIdStack& queueIdStack);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeOperationContext(
        Buffer& out,
        const OperationContext& operationContext);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeContentLanguageList(
        Buffer& out,
        const ContentLanguageList& contentLanguages);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeAcceptLanguageList(
        Buffer& out,
        const AcceptLanguageList& acceptLanguages);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMException(
        Buffer& out,
        const CIMException& cimException);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMPropertyList(
        Buffer& out,
        const CIMPropertyList& cimPropertyList);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMObjectPath(
        Buffer& out,
        const CIMObjectPath& cimObjectPath);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMInstance(
        Buffer& out,
        const CIMInstance& cimInstance);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMNamespaceName(
        Buffer& out,
        const CIMNamespaceName& cimNamespaceName);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMName(
        Buffer& out,
        const CIMName& cimName);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMObject(
        Buffer& out,
        const CIMObject& object);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMParamValue(
        Buffer& out,
        const CIMParamValue& paramValue);

    //
    //
    // CIM Request Messages
    //
    //

    //
    // CIMOperationRequestMessages
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMGetInstanceRequestMessage(
        Buffer& out,
        CIMGetInstanceRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMDeleteInstanceRequestMessage(
        Buffer& out,
        CIMDeleteInstanceRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMCreateInstanceRequestMessage(
        Buffer& out,
        CIMCreateInstanceRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMModifyInstanceRequestMessage(
        Buffer& out,
        CIMModifyInstanceRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMEnumerateInstancesRequestMessage(
        Buffer& out,
        CIMEnumerateInstancesRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMEnumerateInstanceNamesRequestMessage(
        Buffer& out,
        CIMEnumerateInstanceNamesRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMExecQueryRequestMessage(
        Buffer& out,
        CIMExecQueryRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMAssociatorsRequestMessage(
        Buffer& out,
        CIMAssociatorsRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMAssociatorNamesRequestMessage(
        Buffer& out,
        CIMAssociatorNamesRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMReferencesRequestMessage(
        Buffer& out,
        CIMReferencesRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMReferenceNamesRequestMessage(
        Buffer& out,
        CIMReferenceNamesRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMGetPropertyRequestMessage(
        Buffer& out,
        CIMGetPropertyRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMSetPropertyRequestMessage(
        Buffer& out,
        CIMSetPropertyRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMInvokeMethodRequestMessage(
        Buffer& out,
        CIMInvokeMethodRequestMessage* message);

    //
    // CIMIndicationRequestMessages
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMCreateSubscriptionRequestMessage(
        Buffer& out,
        CIMCreateSubscriptionRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMModifySubscriptionRequestMessage(
        Buffer& out,
        CIMModifySubscriptionRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMDeleteSubscriptionRequestMessage(
        Buffer& out,
        CIMDeleteSubscriptionRequestMessage* message);

    //
    // Generic CIMRequestMessages
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMExportIndicationRequestMessage(
        Buffer& out,
        CIMExportIndicationRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMProcessIndicationRequestMessage(
        Buffer& out,
        CIMProcessIndicationRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMDisableModuleRequestMessage(
        Buffer& out,
        CIMDisableModuleRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMEnableModuleRequestMessage(
        Buffer& out,
        CIMEnableModuleRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMStopAllProvidersRequestMessage(
        Buffer& out,
        CIMStopAllProvidersRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMInitializeProviderAgentRequestMessage(
        Buffer& out,
        CIMInitializeProviderAgentRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMNotifyConfigChangeRequestMessage(
        Buffer& out,
        CIMNotifyConfigChangeRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMSubscriptionInitCompleteRequestMessage(
        Buffer& out,
        CIMSubscriptionInitCompleteRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMIndicationServiceDisabledRequestMessage(
        Buffer& out,
        CIMIndicationServiceDisabledRequestMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeProvAgtGetScmoClassRequestMessage(
        Buffer& out,
        ProvAgtGetScmoClassRequestMessage* message);

    //
    //
    // CIM Response Messages
    //
    //

    //
    // CIMOperationResponseMessages
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMGetInstanceResponseMessage(
        Buffer& out,
        CIMGetInstanceResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMDeleteInstanceResponseMessage(
        Buffer& out,
        CIMDeleteInstanceResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMCreateInstanceResponseMessage(
        Buffer& out,
        CIMCreateInstanceResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMModifyInstanceResponseMessage(
        Buffer& out,
        CIMModifyInstanceResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMEnumerateInstancesResponseMessage(
        Buffer& out,
        CIMEnumerateInstancesResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMEnumerateInstanceNamesResponseMessage(
        Buffer& out,
        CIMEnumerateInstanceNamesResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMExecQueryResponseMessage(
        Buffer& out,
        CIMExecQueryResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMAssociatorsResponseMessage(
        Buffer& out,
        CIMAssociatorsResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMAssociatorNamesResponseMessage(
        Buffer& out,
        CIMAssociatorNamesResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMReferencesResponseMessage(
        Buffer& out,
        CIMReferencesResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMReferenceNamesResponseMessage(
        Buffer& out,
        CIMReferenceNamesResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMGetPropertyResponseMessage(
        Buffer& out,
        CIMGetPropertyResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMSetPropertyResponseMessage(
        Buffer& out,
        CIMSetPropertyResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMInvokeMethodResponseMessage(
        Buffer& out,
        CIMInvokeMethodResponseMessage* message);

    //
    // CIMIndicationResponseMessages
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMCreateSubscriptionResponseMessage(
        Buffer& out,
        CIMCreateSubscriptionResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMModifySubscriptionResponseMessage(
        Buffer& out,
        CIMModifySubscriptionResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMDeleteSubscriptionResponseMessage(
        Buffer& out,
        CIMDeleteSubscriptionResponseMessage* message);

    //
    // Generic CIMResponseMessages
    //

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMExportIndicationResponseMessage(
        Buffer& out,
        CIMExportIndicationResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMProcessIndicationResponseMessage(
        Buffer& out,
        CIMProcessIndicationResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMDisableModuleResponseMessage(
        Buffer& out,
        CIMDisableModuleResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMEnableModuleResponseMessage(
        Buffer& out,
        CIMEnableModuleResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMStopAllProvidersResponseMessage(
        Buffer& out,
        CIMStopAllProvidersResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMInitializeProviderAgentResponseMessage(
        Buffer& out,
        CIMInitializeProviderAgentResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMNotifyConfigChangeResponseMessage(
        Buffer& out,
        CIMNotifyConfigChangeResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMSubscriptionInitCompleteResponseMessage(
        Buffer& out,
        CIMSubscriptionInitCompleteResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeCIMIndicationServiceDisabledResponseMessage(
        Buffer& out,
        CIMIndicationServiceDisabledResponseMessage* message);

    PEGASUS_HIDDEN_LINKAGE
    static void _serializeProvAgtGetScmoClassResponseMessage(
        Buffer& out,
        ProvAgtGetScmoClassResponseMessage* message);
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_CIMMessageSerializer_h */
