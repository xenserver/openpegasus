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

#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Common/XmlWriter.h>
#include <Pegasus/Common/PegasusVersion.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/MessageLoader.h>
#include <Pegasus/Common/AutoPtr.h>

#include "IndicationHandlerService.h"

PEGASUS_USING_STD;

PEGASUS_USING_PEGASUS;

PEGASUS_NAMESPACE_BEGIN

IndicationHandlerService::IndicationHandlerService(CIMRepository* repository)
    : Base("IndicationHandlerService"),
      _repository(repository)
{
}

void IndicationHandlerService::_handle_async_request(AsyncRequest* req)
{
    if (req->getType() == ASYNC_CIMSERVICE_STOP)
    {
        handle_CimServiceStop(static_cast<CimServiceStop *>(req));
    }
    else if (req->getType() == ASYNC_ASYNC_LEGACY_OP_START)
    {
        AutoPtr<Message> legacy(
            static_cast<AsyncLegacyOperationStart *>(req)->get_action());
        if (legacy->getType() == CIM_HANDLE_INDICATION_REQUEST_MESSAGE)
        {
            AutoPtr<Message> legacy_response(_handleIndication(
                (CIMHandleIndicationRequestMessage*) legacy.get()));
            legacy.release();
            AutoPtr<AsyncLegacyOperationResult> async_result(
                new AsyncLegacyOperationResult(
                    req->op,
                    legacy_response.get()));
            legacy_response.release();
            async_result.release();
            _complete_op_node(req->op);
        }
        else
        {
            PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL2,
                "IndicationHandlerService::_handle_async_request got "
                    "unexpected legacy message type '%u'", legacy->getType()));
            _make_response(req, async_results::CIM_NAK);
        }
    }
    else
    {
        Base::_handle_async_request(req);
    }
}

void IndicationHandlerService::handleEnqueue(Message* message)
{
    PEGASUS_ASSERT(message != 0);

    AutoPtr<CIMMessage> cimMessage(dynamic_cast<CIMMessage *>(message));
    PEGASUS_ASSERT(cimMessage.get() != 0);

    // Set the client's requested language into this service thread.
    // This will allow functions in this service to return messages
    // in the correct language.
    cimMessage->updateThreadLanguages();

    switch (message->getType())
    {
        case CIM_HANDLE_INDICATION_REQUEST_MESSAGE:
        {
            AutoPtr<CIMHandleIndicationResponseMessage> response(
                _handleIndication(
                    (CIMHandleIndicationRequestMessage*) message));
            SendForget(response.get());
            response.release();
            break;
        }

        default:
            PEGASUS_ASSERT(0);
            break;
    }
}

void IndicationHandlerService::handleEnqueue()
{
   AutoPtr<Message> message(dequeue());

   PEGASUS_ASSERT(message.get() != 0);
   if (message.get())
   {
       handleEnqueue(message.get());
       message.release();
   }
}

CIMHandleIndicationResponseMessage*
IndicationHandlerService::_handleIndication(
    CIMHandleIndicationRequestMessage* request)
{
    PEG_METHOD_ENTER(TRC_IND_HANDLER,
        "IndicationHandlerService::_handleIndication()");

    Boolean handleIndicationSuccess = true;
    CIMException cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_SUCCESS, String::EMPTY);

    CIMName className = request->handlerInstance.getClassName();
    CIMNamespaceName nameSpace = request->nameSpace;

    CIMInstance indication = request->indicationInstance;
    CIMInstance handler = request->handlerInstance;

    PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL4,
        "Handler service received %s Indication %s for %s:%s.%s Handler",
        (const char*)(indication.getClassName().getString().getCString()),
        (const char*)(request->messageId.getCString()),
        (const char*)(request->nameSpace.getString().getCString()),
        (const char*)(handler.getClassName().getString().getCString()),
        (const char*)(handler.getProperty(handler.findProperty(
        PEGASUS_PROPERTYNAME_NAME)).getValue().toString().getCString())));
    Uint32 pos = PEG_NOT_FOUND;

    if (className.equal (PEGASUS_CLASSNAME_INDHANDLER_CIMXML) ||
        className.equal (PEGASUS_CLASSNAME_LSTNRDST_CIMXML))
    {
        pos = handler.findProperty(PEGASUS_PROPERTYNAME_LSTNRDST_DESTINATION);

        if (pos == PEG_NOT_FOUND)
        {
            cimException = PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                MessageLoaderParms(
                    "HandlerService.IndicationHandlerService."
                        "CIMXML_HANDLER_WITHOUT_DESTINATION",
                    "CIMXml Handler missing Destination property"));
            handleIndicationSuccess = false;
        }
        else
        {
            CIMProperty prop = handler.getProperty(pos);
            String destination = prop.getValue().toString();

            if (destination.size() == 0)
            {
                cimException = PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                    MessageLoaderParms(
                        "HandlerService.IndicationHandlerService."
                            "INVALID_DESTINATION",
                        "invalid destination"));
                handleIndicationSuccess = false;
            }
//compared index 10 is not :
            else if (destination.subString(0, 10) == String("localhost/"))
            {
                Uint32 exportServer =
                    find_service_qid(PEGASUS_QUEUENAME_EXPORTREQDISPATCHER);

                // Listener is build with Cimom, so send message to ExportServer
               AutoPtr<CIMExportIndicationRequestMessage> exportmessage(
                    new CIMExportIndicationRequestMessage(
                        XmlWriter::getNextMessageId(),
                        //taking localhost/CIMListener portion out from reg
                        destination.subString(21),
                        indication,
                        QueueIdStack(exportServer, getQueueId()),
                        String::EMPTY,
                        String::EMPTY));

                exportmessage->operationContext.insert(
                    IdentityContainer(String::EMPTY));
                exportmessage->operationContext.set(
                    request->operationContext.get(
                    ContentLanguageListContainer::NAME));
                AutoPtr<AsyncOpNode> op( this->get_op());

                AutoPtr<AsyncLegacyOperationStart> asyncRequest(
                    new AsyncLegacyOperationStart(
                    op.get(),
                    exportServer,
                    exportmessage.get()));

                exportmessage.release();

                PEG_TRACE((TRC_IND_HANDLER, Tracer::LEVEL4,
                    "Indication handler forwarding message to %s",
                        ((MessageQueue::lookup(exportServer)) ?
                            ((MessageQueue::lookup(exportServer))->
                                getQueueName()):
                            "BAD queue name")));
                PEG_TRACE ((TRC_INDICATION_GENERATION, Tracer::LEVEL4,
                    "Sending %s Indication %s to destination %s",
                    (const char*) (indication.getClassName().getString().
                    getCString()),
                    (const char*)(request->messageId.getCString()),
                    (const char*) destination.getCString()));

                //SendAsync(op,
                //      exportServer[0],
                //      IndicationHandlerService::_handleIndicationCallBack,
                //      this,
                //      (void *)request->queueIds.top());
                AutoPtr<AsyncReply> asyncReply(SendWait(asyncRequest.get()));
                asyncRequest.release();

                // Return the ExportIndication results in HandleIndication
                //response
                AutoPtr<CIMExportIndicationResponseMessage> exportResponse(
                    reinterpret_cast<CIMExportIndicationResponseMessage *>(
                        (static_cast<AsyncLegacyOperationResult *>(
                            asyncReply.get()))->get_result()));
                cimException = exportResponse->cimException;

                this->return_op(op.release());
            }
            else
            {
                handleIndicationSuccess = _loadHandler(request, cimException);
            }
        }
    }
    else if (className.equal (PEGASUS_CLASSNAME_INDHANDLER_SNMP))
    {
        pos = handler.findProperty(PEGASUS_PROPERTYNAME_LSTNRDST_TARGETHOST);

        if (pos == PEG_NOT_FOUND)
        {
            cimException = PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                MessageLoaderParms(
                    "HandlerService.IndicationHandlerService."
                        "SNMP_HANDLER_WITHOUT_TARGETHOST",
                    "Snmp Handler missing Targethost property"));
            handleIndicationSuccess = false;
        }
        else
        {
            CIMProperty prop = handler.getProperty(pos);
            String destination = prop.getValue().toString();

            if (destination.size() == 0)
            {
                cimException = PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                    MessageLoaderParms(
                        "HandlerService.IndicationHandlerService."
                            "INVALID_TARGETHOST",
                        "invalid targethost"));
                handleIndicationSuccess = false;
            }
            else
            {
                handleIndicationSuccess = _loadHandler(request, cimException);
            }
        }
    }
    else if ((className.equal (PEGASUS_CLASSNAME_LSTNRDST_SYSTEM_LOG)) ||
             (className.equal (PEGASUS_CLASSNAME_LSTNRDST_EMAIL)))
    {
        handleIndicationSuccess = _loadHandler(request, cimException);
    }

    // no success to handle indication
    // somewhere an exception message was build
    // time to write the error message to the log
    if (!handleIndicationSuccess)
    {
        Logger::put_l(Logger::ERROR_LOG, System::CIMSERVER, Logger::WARNING,
            MessageLoaderParms(
                "HandlerService.IndicationHandlerService."
                    "INDICATION_DELIVERY_FAILED",
                "Failed to deliver an indication: $0",
                cimException.getMessage()));
    }

    CIMHandleIndicationResponseMessage* response =
        dynamic_cast<CIMHandleIndicationResponseMessage*>(
            request->buildResponse());
    response->cimException = cimException;

    delete request;
    PEG_METHOD_EXIT();
    return response;
}

Boolean IndicationHandlerService::_loadHandler(
    CIMHandleIndicationRequestMessage* request,
    CIMException& cimException)
{
    PEG_METHOD_ENTER(TRC_IND_HANDLER,
        "IndicationHandlerService::__loadHandler()");

    CIMName className = request->handlerInstance.getClassName();

    try
    {
        CIMHandler* handlerLib = _lookupHandlerForClass(className);

        if (handlerLib)
        {
            ContentLanguageList langs =
                ((ContentLanguageListContainer)request->operationContext.
                get(ContentLanguageListContainer::NAME)).getLanguages();

            handlerLib->handleIndication(
                request->operationContext,
                request->nameSpace.getString(),
                request->indicationInstance,
                request->handlerInstance,
                request->subscriptionInstance,
                langs);
        }
        else
        {
            cimException = PEGASUS_CIM_EXCEPTION_L(CIM_ERR_FAILED,
                MessageLoaderParms("HandlerService."
                "IndicationHandlerService.FAILED_TO_LOAD",
                "Failed to load Handler"));
            PEG_METHOD_EXIT();
            return false;
        }

    }
    catch (Exception& e)
    {
        cimException =
            PEGASUS_CIM_EXCEPTION(CIM_ERR_FAILED, e.getMessage());
        PEG_METHOD_EXIT();
        return false;
    }
    catch (...)
    {
        cimException =
            PEGASUS_CIM_EXCEPTION(CIM_ERR_FAILED, "Exception: Unknown");
        PEG_METHOD_EXIT();
        return false;
    }
    PEG_METHOD_EXIT();
    return true;
}

CIMHandler* IndicationHandlerService::_lookupHandlerForClass(
   const CIMName& className)
{
   PEG_METHOD_ENTER(TRC_IND_HANDLER,
        "IndicationHandlerService::_lookupHandlerForClass()");

   String handlerId;

   if (className.equal(PEGASUS_CLASSNAME_INDHANDLER_CIMXML) ||
       className.equal(PEGASUS_CLASSNAME_LSTNRDST_CIMXML))
   {
       handlerId = String("CIMxmlIndicationHandler");
   }
   else if (className.equal(PEGASUS_CLASSNAME_INDHANDLER_SNMP))
   {
       handlerId = String("snmpIndicationHandler");
   }
   else if (className.equal(PEGASUS_CLASSNAME_LSTNRDST_SYSTEM_LOG))
       handlerId = String("SystemLogListenerDestination");
   else if (className.equal(PEGASUS_CLASSNAME_LSTNRDST_EMAIL))
       handlerId = String("EmailListenerDestination");

   PEGASUS_ASSERT(handlerId.size() != 0);

   CIMHandler* handler = _handlerTable.getHandler(handlerId, _repository);

   PEG_METHOD_EXIT();
   return handler;
}

PEGASUS_NAMESPACE_END
