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

#include "ProviderManagerService.h"

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/PegasusVersion.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Common/Thread.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/Logger.h>
#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/StatisticalData.h>

#include <Pegasus/Config/ConfigManager.h>

#include <Pegasus/ProviderManagerService/BasicProviderManagerRouter.h>
#include <Pegasus/ProviderManagerService/OOPProviderManagerRouter.h>

#include <Pegasus/Server/ProviderRegistrationManager/ProviderManagerMap.h>

#ifdef PEGASUS_ZOS_SECURITY
// This include file will not be provided in the OpenGroup CVS for now.
// Do NOT try to include it in your compile
#include <Pegasus/ProviderManager2/ProviderManagerzOS_inline.h>
#endif


PEGASUS_NAMESPACE_BEGIN

ProviderManagerService* ProviderManagerService::providerManagerService=NULL;
Boolean ProviderManagerService::_allProvidersStopped = false;
Uint32 ProviderManagerService::_indicationServiceQueueId = PEG_NOT_FOUND;

ProviderManagerService::ProviderManagerService(
        ProviderRegistrationManager * providerRegistrationManager,
        CIMRepository * repository,
        ProviderManager* (*createDefaultProviderManagerCallback)())
    : MessageQueueService(PEGASUS_QUEUENAME_PROVIDERMANAGER_CPP)
{
    providerManagerService=this;
    _repository=repository;

    _providerRegistrationManager = providerRegistrationManager;

    _unloadIdleProvidersBusy = 0;

    _basicProviderManagerRouter = 0;
    _oopProviderManagerRouter = 0;

    // Determine which ProviderManagerRouter(s) to use

    _forceProviderProcesses = ConfigManager::parseBooleanValue(
        ConfigManager::getInstance()->getCurrentValue(
            "forceProviderProcesses"));

#if defined(PEGASUS_DISABLE_PROV_USERCTXT) || defined(PEGASUS_OS_ZOS)
    if (_forceProviderProcesses)
    {
        _oopProviderManagerRouter = new OOPProviderManagerRouter(
            indicationCallback, responseChunkCallback,
            providerModuleFailureCallback);
    }
    else
    {
        _basicProviderManagerRouter = new BasicProviderManagerRouter(
            indicationCallback, responseChunkCallback,
            createDefaultProviderManagerCallback);
    }
#else
    _oopProviderManagerRouter = new OOPProviderManagerRouter(
        indicationCallback, responseChunkCallback,
        providerModuleFailureCallback);

    if (!_forceProviderProcesses)
    {
        _basicProviderManagerRouter = new BasicProviderManagerRouter(
            indicationCallback, responseChunkCallback,
            createDefaultProviderManagerCallback);
    }
#endif
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
   if (!_basicProviderManagerRouter)
   {
       _basicProviderManagerRouter = new BasicProviderManagerRouter(
           indicationCallback, responseChunkCallback,
           createDefaultProviderManagerCallback);
   }
#endif
}

ProviderManagerService::~ProviderManagerService(void)
{
    delete _basicProviderManagerRouter;
    delete _oopProviderManagerRouter;
    providerManagerService=NULL;
}

void ProviderManagerService::handleEnqueue(void)
{
    Message * message = dequeue();

    handleEnqueue(message);
}

void ProviderManagerService::handleEnqueue(Message * message)
{
    PEGASUS_ASSERT(message != 0);

    AsyncLegacyOperationStart* asyncRequest =
        static_cast<AsyncLegacyOperationStart*>(message->get_async());

    if (asyncRequest == 0)
    {
        asyncRequest = new AsyncLegacyOperationStart(
            0,
            this->getQueueId(),
            message);
    }

    _handle_async_request(asyncRequest);
}

void ProviderManagerService::_handle_async_request(AsyncRequest * request)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderManagerService::_handle_async_request");

    PEGASUS_ASSERT((request != 0) && (request->op != 0));

    if (request->getType() == ASYNC_ASYNC_LEGACY_OP_START)
    {
        _incomingQueue.insert_back(request->op);
        ThreadStatus rtn = PEGASUS_THREAD_OK;
        while ((rtn = _thread_pool->allocate_and_awaken(
                          (void *)this,
                          ProviderManagerService::handleCimOperation)) !=
                   PEGASUS_THREAD_OK)
        {
            if (rtn == PEGASUS_THREAD_INSUFFICIENT_RESOURCES)
                Threads::yield();
            else
            {
                PEG_TRACE((
                    TRC_PROVIDERMANAGER,
                    Tracer::LEVEL1,
                    "Could not allocate thread for %s.",
                    getQueueName()));
                break;
           }
        }
    }
    else
    {
        // pass all other operations to the default handler
        MessageQueueService::_handle_async_request(request);
    }

    PEG_METHOD_EXIT();

    return;
}

// Note: This method should not throw an exception.  It is used as a thread
// entry point, and any exceptions thrown are ignored.
ThreadReturnType PEGASUS_THREAD_CDECL
ProviderManagerService::handleCimOperation(void* arg)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderManagerService::handleCimOperation");

    PEGASUS_ASSERT(arg != 0);

    // get the service from argument
    ProviderManagerService* service =
        reinterpret_cast<ProviderManagerService *>(arg);
    PEGASUS_ASSERT(service != 0);

    try
    {
        AsyncOpNode* op = service->_incomingQueue.remove_front();
        PEGASUS_ASSERT(op != 0);

        AsyncRequest* request =
            static_cast<AsyncRequest*>(op->getRequest());
        PEGASUS_ASSERT(request != 0);
        PEGASUS_ASSERT(request->getType() == ASYNC_ASYNC_LEGACY_OP_START);

        Message* legacy =
            static_cast<AsyncLegacyOperationStart *>(request)->get_action();

        AutoPtr<Message> xmessage(legacy);

        // Set the client's requested language into this service thread.
        // This will allow functions in this service to return messages
        // in the correct language.
        CIMMessage* msg = dynamic_cast<CIMMessage *>(legacy);
        PEGASUS_ASSERT(msg != 0);

        Thread::setLanguages(
            ((AcceptLanguageListContainer)msg->operationContext.get(
                AcceptLanguageListContainer::NAME)).getLanguages());

        service->handleCimRequest(op, legacy);
    }
    catch (const Exception& e)
    {
        PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Unexpected exception in handleCimOperation: %s",
            (const char*)e.getMessage().getCString()));
    }
    catch (...)
    {
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Unexpected exception in handleCimOperation.");
    }

    PEG_METHOD_EXIT();

    return ThreadReturnType(0);
}

void ProviderManagerService::handleCimRequest(
    AsyncOpNode * op,
    Message * message)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderManagerService::handleCimRequest");

    CIMRequestMessage * request = dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0);

    AutoPtr<Message> response;

    try
    {
        if (request->getType() == CIM_EXPORT_INDICATION_REQUEST_MESSAGE)
        {
            //
            // Get a ProviderIdContainer for ExportIndicationRequestMessage.
            // Note: This can be removed when the CIMExportRequestDispatcher
            // is updated to add the ProviderIdContainer to the message.
            //
            CIMInstance providerModule;
            CIMInstance provider;
            const CIMExportIndicationRequestMessage* expRequest =
                dynamic_cast<const CIMExportIndicationRequestMessage*>(request);
            PEGASUS_ASSERT(expRequest != 0);
            if (_providerRegistrationManager->lookupIndicationConsumer(
                    expRequest->destinationPath, provider, providerModule))
            {
                request->operationContext.insert(
                    ProviderIdContainer(providerModule, provider));
                response.reset(_processMessage(request));
            }
            else
            {
                CIMResponseMessage* cimResponse = request->buildResponse();
                cimResponse->cimException = PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_NOT_SUPPORTED, String::EMPTY);
                response.reset(cimResponse);
            }
        }
        else if ((dynamic_cast<CIMOperationRequestMessage*>(request) != 0) ||
            (dynamic_cast<CIMIndicationRequestMessage*>(request) != 0) ||
            (request->getType() == CIM_EXPORT_INDICATION_REQUEST_MESSAGE))
        {
            // Handle CIMOperationRequestMessage,
            // CIMExportIndicationRequestMessage,
            // and CIMIndicationRequestMessage.
            // (These should be blocked when the provider module is disabled.)

            //
            // Get the provider module instance to check for a disabled module
            //
            CIMInstance providerModule;

            // The provider ID container is added to the OperationContext
            // by the CIMOperationRequestDispatcher for all CIM operation
            // requests to providers, so it does not need to be added again.
            ProviderIdContainer pidc =
                request->operationContext.get(ProviderIdContainer::NAME);
            providerModule = pidc.getModule();

            //
            // Check if the target provider is disabled
            //
            Boolean moduleDisabled = false;
            Uint32 pos =
                providerModule.findProperty(CIMName("OperationalStatus"));
            PEGASUS_ASSERT(pos != PEG_NOT_FOUND);
            Array<Uint16> operationalStatus;
            providerModule.getProperty(pos).getValue().get(operationalStatus);

            for (Uint32 i = 0; i < operationalStatus.size(); i++)
            {
                if ((operationalStatus[i] == CIM_MSE_OPSTATUS_VALUE_STOPPED) ||
                    (operationalStatus[i] == CIM_MSE_OPSTATUS_VALUE_STOPPING))
                {
                    moduleDisabled = true;
                    break;
                }
            }

            if (moduleDisabled)
            {
                //
                // Send a "provider blocked" response
                //
                CIMResponseMessage* cimResponse = request->buildResponse();
                cimResponse->cimException = PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_NOT_SUPPORTED,
                    MessageLoaderParms(
                        "ProviderManager.ProviderManagerService."
                            "PROVIDER_BLOCKED",
                        "provider blocked."));
                response.reset(cimResponse);
            }
            else
            {
                //
                // Forward the request to the appropriate ProviderManagerRouter
                //
                response.reset(_processMessage(request));
            }
        }
        else if (request->getType() == CIM_ENABLE_MODULE_REQUEST_MESSAGE)
        {
            // Handle CIMEnableModuleRequestMessage
            CIMEnableModuleRequestMessage * emReq =
                dynamic_cast<CIMEnableModuleRequestMessage*>(request);

            CIMInstance providerModule = emReq->providerModule;

            // Forward the request to the ProviderManager
            response.reset(_processMessage(request));

            // If successful, update provider module status to OK
            // ATTN: Use CIMEnableModuleResponseMessage operationalStatus?
            CIMEnableModuleResponseMessage * emResp =
                dynamic_cast<CIMEnableModuleResponseMessage*>(response.get());
            if (emResp->cimException.getCode() == CIM_ERR_SUCCESS)
            {
                //
                //  On a successful enable, remove Stopped status and
                //  append OK status
                //
                Array<Uint16> removeStatus;
                Array<Uint16> appendStatus;
                removeStatus.append (CIM_MSE_OPSTATUS_VALUE_STOPPED);
                appendStatus.append (CIM_MSE_OPSTATUS_VALUE_OK);
                _updateProviderModuleStatus(
                    providerModule, removeStatus, appendStatus);
            }
        }
        else if (request->getType() == CIM_DISABLE_MODULE_REQUEST_MESSAGE)
        {
            // Handle CIMDisableModuleRequestMessage
            CIMDisableModuleRequestMessage * dmReq =
                dynamic_cast<CIMDisableModuleRequestMessage*>(request);

            CIMInstance providerModule = dmReq->providerModule;
            Boolean updateModuleStatus = !dmReq->disableProviderOnly;

            //
            //  On issuing a disable request, append Stopping status
            //  Do not remove existing status
            //
            if (updateModuleStatus)
            {
                Array<Uint16> removeStatus;
                Array<Uint16> appendStatus;
                appendStatus.append (CIM_MSE_OPSTATUS_VALUE_STOPPING);
                _updateProviderModuleStatus(
                    providerModule, removeStatus, appendStatus);
            }

            // Forward the request to the ProviderManager
            response.reset(_processMessage(request));

            // Update provider module status based on success or failure
            if (updateModuleStatus)
            {
                CIMDisableModuleResponseMessage * dmResp =
                    dynamic_cast<CIMDisableModuleResponseMessage*>(
                        response.get());
                if (dmResp->cimException.getCode() != CIM_ERR_SUCCESS)
                {
                    //
                    //  On an unsuccessful disable, remove Stopping status
                    //
                    Array<Uint16> removeStatus;
                    Array<Uint16> appendStatus;
                    removeStatus.append (CIM_MSE_OPSTATUS_VALUE_STOPPING);
                    _updateProviderModuleStatus(
                        providerModule, removeStatus, appendStatus);
                }
                else
                {
                    // Disable may or may not have been successful,
                    // depending on whether there are outstanding requests.
                    // Remove Stopping status
                    // Append status, if any, from disable module response
                    Array<Uint16> removeStatus;
                    Array<Uint16> appendStatus;
                    removeStatus.append (CIM_MSE_OPSTATUS_VALUE_STOPPING);
                    if (dmResp->operationalStatus.size() > 0)
                    {
                        //
                        //  On a successful disable, remove an OK or a Degraded
                        //  status, if present
                        //
                        if (dmResp->operationalStatus[
                            dmResp->operationalStatus.size()-1] ==
                            CIM_MSE_OPSTATUS_VALUE_STOPPED)
                        {
                            removeStatus.append (CIM_MSE_OPSTATUS_VALUE_OK);
                            removeStatus.append
                                (CIM_MSE_OPSTATUS_VALUE_DEGRADED);
                        }
                        appendStatus.append (dmResp->operationalStatus[
                            dmResp->operationalStatus.size()-1]);
                    }
                    _updateProviderModuleStatus(
                        providerModule, removeStatus, appendStatus);
                }
            }
        }
        else
        {
            response.reset(_processMessage(request));
        }
    }
    catch (Exception& e)
    {
        CIMResponseMessage* cimResponse = request->buildResponse();
        cimResponse->cimException =
            CIMException(CIM_ERR_FAILED, e.getMessage());
        response.reset(cimResponse);
    }
    catch (PEGASUS_STD(exception)& e)
    {
        CIMResponseMessage* cimResponse = request->buildResponse();
        cimResponse->cimException = CIMException(CIM_ERR_FAILED, e.what());
        response.reset(cimResponse);
    }
    catch (...)
    {
        CIMResponseMessage* cimResponse = request->buildResponse();
        cimResponse->cimException = CIMException(CIM_ERR_FAILED, String::EMPTY);
        response.reset(cimResponse);
    }

    AsyncLegacyOperationResult * async_result =
        new AsyncLegacyOperationResult(
        op,
        response.release());

    _complete_op_node(op);

    PEG_METHOD_EXIT();
}

void ProviderManagerService::responseChunkCallback(
    CIMRequestMessage* request,
    CIMResponseMessage* response)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderManagerService::responseChunkCallback");

    try
    {
        // only incomplete messages are processed because the caller ends up
        // sending the complete() stage
        PEGASUS_ASSERT(response->isComplete() == false);

        AsyncLegacyOperationStart *requestAsync =
            dynamic_cast<AsyncLegacyOperationStart *>(request->get_async());
        request->put_async(requestAsync);  // Put it back for the next chunk
        PEGASUS_ASSERT(requestAsync);
        AsyncOpNode *op = requestAsync->op;
        PEGASUS_ASSERT(op);
        PEGASUS_ASSERT(!response->get_async());
        response->put_async(new AsyncLegacyOperationResult(op, response));

        // set the destination
        op->_op_dest = op->_callback_response_q;

        MessageQueueService *service =
            dynamic_cast<MessageQueueService *>(op->_callback_response_q);

        PEGASUS_ASSERT(service);

        // the last chunk MUST be sent last, so use execute the callback
        // not all chunks are going through the dispatcher's chunk
        // resequencer, so this must be a synchronous call here
        // After the call is done, response and asyncResponse are now invalid
        // as they have been sent and deleted externally

        op->_async_callback(op, service, op->_callback_ptr);
    }
    catch (Exception &e)
    {
        PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Exception in ProviderManagerService::responseChunkCallback: %s"
            ". Chunk not delivered.",(const char*)e.getMessage().getCString()));
    }
    catch (...)
    {
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Unknown exception in ProviderManagerService::"
            "responseChunkCallback. Chunk not delivered.");
    }

    PEG_METHOD_EXIT();
}

Message* ProviderManagerService::_processMessage(CIMRequestMessage* request)
{
    Message* response = 0;

    // Add CachedClassDefinitionContainer here needed by CMPIProviderManager
    // to correct mismatches between Embedded Instances/Objects for InvokeMethod
    // requests.
#if defined (PEGASUS_ENABLE_CMPI_PROVIDER_MANAGER)
    if (request->getType() == CIM_INVOKE_METHOD_REQUEST_MESSAGE)
    {
        CIMOperationRequestMessage *reqMsg =
            dynamic_cast<CIMOperationRequestMessage*>(request);
        PEGASUS_ASSERT(reqMsg);
        ProviderIdContainer pidc = (ProviderIdContainer)
           reqMsg->operationContext.get(ProviderIdContainer::NAME);
        CIMInstance providerModule = pidc.getModule();
        String interfaceType;
        CIMValue itValue = providerModule.getProperty(
            providerModule.findProperty("InterfaceType")).getValue();
        itValue.get(interfaceType);
        if (interfaceType == "CMPI")
        {
            CIMConstClass cls = _repository->getFullConstClass(
                reqMsg->nameSpace,
               reqMsg->className);
            reqMsg->operationContext.insert(
                CachedClassDefinitionContainer(cls));
        }
    }
 #endif

    if ((request->getType() == CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE) ||
        (request->getType() ==
            CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE) ||
        (request->getType() ==
            CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE) ||
        (request->getType() == CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE))
    {
        if (_basicProviderManagerRouter)
        {
            response = _basicProviderManagerRouter->processMessage(request);
        }

        if (_oopProviderManagerRouter)
        {
            // Note: These responses do not contain interesting data, so just
            // use the last one.
            delete response;

            response = _oopProviderManagerRouter->processMessage(request);
        }

        if (request->getType() == CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE)
        {
            _allProvidersStopped = true;
        }
    }
    else
    {
        CIMInstance providerModule;

        if (request->getType() == CIM_ENABLE_MODULE_REQUEST_MESSAGE)
        {
            CIMEnableModuleRequestMessage* emReq =
                dynamic_cast<CIMEnableModuleRequestMessage*>(request);
            PEGASUS_ASSERT(emReq != 0);
            providerModule = emReq->providerModule;
        }
        else if (request->getType() == CIM_DISABLE_MODULE_REQUEST_MESSAGE)
        {
            CIMDisableModuleRequestMessage* dmReq =
                dynamic_cast<CIMDisableModuleRequestMessage*>(request);
            PEGASUS_ASSERT(dmReq != 0);
            providerModule = dmReq->providerModule;
        }
        else
        {
            ProviderIdContainer pidc =
                request->operationContext.get(ProviderIdContainer::NAME);
            providerModule = pidc.getModule();

            String interfaceType;
            String interfaceVersion;
            CIMValue itValue = providerModule.getProperty(
                providerModule.findProperty("InterfaceType")).getValue();
            CIMValue ivValue = providerModule.getProperty(
                providerModule.findProperty("InterfaceVersion")).getValue();
            itValue.get(interfaceType);
            ivValue.get(interfaceVersion);

            String provMgrPath;

            if (!ProviderManagerMap::instance().getProvMgrPathForIfcType(
                    interfaceType, interfaceVersion, provMgrPath))
            {
                MessageLoaderParms parms(
                    "ProviderManager.ProviderManagerService."
                        "PROVIDERMANAGER_LOOKUP_FAILED",
                    "Provider interface type \"$0\" version \"$1\" is not "
                        "recognized.",
                    interfaceType,
                    interfaceVersion);
                Logger::put_l(
                    Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
                    parms);

                CIMResponseMessage* cimResponse = request->buildResponse();
                cimResponse->cimException = PEGASUS_CIM_EXCEPTION_L(
                    CIM_ERR_FAILED, parms);
                return cimResponse;
            }

            pidc.setProvMgrPath(provMgrPath);

            request->operationContext.set(pidc);

#ifdef PEGASUS_ZOS_SECURITY
            if (request->getType() != CIM_EXPORT_INDICATION_REQUEST_MESSAGE)
            {
                // this is a z/OS only function
                // the function checks user authorization
                // based on CIM operation versus provider profile
                // Input: request and Provider ID Container
                //Return: failure: a response message for the client
                //        success: NULL
                response = checkSAFProviderProfile(request, pidc);
                if (response != NULL)
                {
                    return response;
                }
            }
#endif
        }

        Uint16 userContext = PEGASUS_DEFAULT_PROV_USERCTXT;
        Uint32 pos = providerModule.findProperty(
            PEGASUS_PROPERTYNAME_MODULE_USERCONTEXT);
        if (pos != PEG_NOT_FOUND)
        {
            providerModule.getProperty(pos).getValue().get(userContext);
        }

        // Load proxy-provider into CIMServer, in case of remote namespace
        // requests. (ie through _basicProviderManagerRouter). -V 3913
#ifdef PEGASUS_ENABLE_REMOTE_CMPI
        if ((dynamic_cast<CIMOperationRequestMessage*>(request) != 0) ||
            (request->getType() == CIM_EXPORT_INDICATION_REQUEST_MESSAGE))
        {
            ProviderIdContainer pidc1 =
            request->operationContext.get(ProviderIdContainer::NAME);
            if (pidc1.isRemoteNameSpace() )
            {
                PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL4,
                                "Processing Remote NameSpace request ");
                response = _basicProviderManagerRouter->processMessage(request);
                return response;
            }
        }
#endif
        // Forward the request to the appropriate ProviderManagerRouter, based
        // on the CIM Server configuration and the UserContext setting.

        if (_forceProviderProcesses
#if !defined(PEGASUS_DISABLE_PROV_USERCTXT) && !defined(PEGASUS_OS_ZOS)
            || (userContext == PG_PROVMODULE_USERCTXT_REQUESTOR)
            || (userContext == PG_PROVMODULE_USERCTXT_DESIGNATED)
            || ((userContext == PG_PROVMODULE_USERCTXT_PRIVILEGED) &&
                !System::isPrivilegedUser(System::getEffectiveUserName()))
#endif
           )
        {
            response = _oopProviderManagerRouter->processMessage(request);
        }
        else
        {
            response = _basicProviderManagerRouter->processMessage(request);
        }
    }

    return response;
}

void ProviderManagerService::unloadIdleProviders()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderManagerService::unloadIdleProviders");

    // Ensure that only one _unloadIdleProvidersHandler thread runs at a time
    _unloadIdleProvidersBusy++;
    if (_unloadIdleProvidersBusy.get() != 1)
    {
        _unloadIdleProvidersBusy--;
        PEG_METHOD_EXIT();
        return;
    }

    //
    // Start an idle provider unload thread
    //

    if (_thread_pool->allocate_and_awaken((void*)this,
            ProviderManagerService::_unloadIdleProvidersHandler) !=
                PEGASUS_THREAD_OK)
    {
        PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
            "Could not allocate thread for %s to unload idle providers.",
            getQueueName()));

        // If we fail to allocate a thread, don't retry now.
        _unloadIdleProvidersBusy--;
        PEG_METHOD_EXIT();
        return;
    }

    // Note: _unloadIdleProvidersBusy is decremented in
    // _unloadIdleProvidersHandler

    PEG_METHOD_EXIT();
}

ThreadReturnType PEGASUS_THREAD_CDECL
ProviderManagerService::_unloadIdleProvidersHandler(void* arg) throw()
{
    ProviderManagerService* myself =
        reinterpret_cast<ProviderManagerService*>(arg);

    try
    {
        PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
            "ProviderManagerService::_unloadIdleProvidersHandler");

        if (myself->_basicProviderManagerRouter)
        {
            try
            {
                myself->_basicProviderManagerRouter->unloadIdleProviders();
            }
            catch (...)
            {
                // Ignore errors
                PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
                    "Unexpected exception from "
                        "BasicProviderManagerRouter::_unloadIdleProviders");
            }
        }

        if (myself->_oopProviderManagerRouter)
        {
            try
            {
                myself->_oopProviderManagerRouter->unloadIdleProviders();
            }
            catch (...)
            {
                // Ignore errors
                PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
                    "Unexpected exception from "
                        "OOPProviderManagerRouter::_unloadIdleProviders");
            }
        }

        myself->_unloadIdleProvidersBusy--;
        PEG_METHOD_EXIT();
    }
    catch (...)
    {
        // Ignore errors
        PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
            "Unexpected exception in _unloadIdleProvidersHandler");

        myself->_unloadIdleProvidersBusy--;
    }

    return ThreadReturnType(0);
}

// Updates the providerModule instance and the ProviderRegistrationManager
//
// This method is used to update the provider module status when the module is
// disabled or enabled.  If a Degraded status has been set (appended) to the
// OperationalStatus, it is cleared (removed) when the module is disabled or
// enabled.
//
void ProviderManagerService::_updateProviderModuleStatus(
    CIMInstance& providerModule,
    const Array<Uint16>& removeStatus,
    const Array<Uint16>& appendStatus)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderManagerService::_updateProviderModuleStatus");

    Array<Uint16> operationalStatus;
    String providerModuleName;

    Uint32 pos = providerModule.findProperty(PEGASUS_PROPERTYNAME_NAME);
    PEGASUS_ASSERT(pos != PEG_NOT_FOUND);
    providerModule.getProperty(pos).getValue().get(providerModuleName);

    //
    // get operational status
    //
    pos = providerModule.findProperty(CIMName("OperationalStatus"));
    PEGASUS_ASSERT(pos != PEG_NOT_FOUND);
    CIMProperty operationalStatusProperty = providerModule.getProperty(pos);

    if (_providerRegistrationManager->updateProviderModuleStatus(
        providerModuleName, removeStatus, appendStatus, operationalStatus) ==
        false)
    {
        throw PEGASUS_CIM_EXCEPTION_L(
            CIM_ERR_FAILED,
            MessageLoaderParms(
                "ProviderManager.ProviderManagerService."
                    "SET_MODULE_STATUS_FAILED",
                "set module status failed."));
    }

    operationalStatusProperty.setValue(CIMValue(operationalStatus));

    PEG_METHOD_EXIT();
}

void ProviderManagerService::indicationCallback(
    CIMProcessIndicationRequestMessage* request)
{
    if (request->operationContext.contains(AcceptLanguageListContainer::NAME))
    {
        AcceptLanguageListContainer cntr =
            request->operationContext.get(AcceptLanguageListContainer::NAME);
    }
    else
    {
        request->operationContext.insert(
            AcceptLanguageListContainer(AcceptLanguageList()));
    }

    if (_indicationServiceQueueId == PEG_NOT_FOUND)
    {
        _indicationServiceQueueId = providerManagerService->find_service_qid(
            PEGASUS_QUEUENAME_INDICATIONSERVICE);
    }

    request->queueIds = QueueIdStack(
        _indicationServiceQueueId, providerManagerService->getQueueId());

    AsyncLegacyOperationStart * asyncRequest =
        new AsyncLegacyOperationStart(
        0,
        _indicationServiceQueueId,
        request);

    providerManagerService->SendForget(asyncRequest);




#ifdef PEGASUS_INDICATIONS_Q_THRESHOLD

    // See Comments in config.mak asociated with
    //  PEGASUS_INDICATIONS_Q_THRESHOLD
    //
    // if INDICATIONS_Q_STALL THRESHOLD is gt 0
    // then if there are over INDICATIONS_Q_STALL_THRESHOLD
    //           indications in the queue
    //      then force this provider to sleep until the queue count
    //      is lower than INDICATIONS_Q_RESUME_THRESHOLD

static Mutex   indicationThresholdReportedLock;
static Boolean indicationThresholdReported = false;

#define INDICATIONS_Q_STALL_THRESHOLD PEGASUS_INDICATIONS_Q_THRESHOLD
#define INDICATIONS_Q_RESUME_THRESHOLD \
    (int)(PEGASUS_INDICATIONS_Q_THRESHOLD*.90)
#define INDICATIONS_Q_STALL_DURATION 250 // milli-seconds

    MessageQueue* indicationsQueue =
        MessageQueue::lookup(_indicationServiceQueueId);

    if (((MessageQueueService *)indicationsQueue)->getIncomingCount() >
            INDICATIONS_Q_STALL_THRESHOLD)
    {
        AutoMutex indicationThresholdReportedAutoMutex(
            indicationThresholdReportedLock);
        if (!indicationThresholdReported)
        {
            indicationThresholdReported = true;
            indicationThresholdReportedAutoMutex.unlock();

            // make log entry to record que max exceeded
            Logger::put(
                Logger::STANDARD_LOG, System::CIMSERVER, Logger::INFORMATION,
                "Indication generation stalled: maximum queue count ($0) "
                    "exceeded.",
                INDICATIONS_Q_STALL_THRESHOLD);
        }
        else
        {
            indicationThresholdReportedAutoMutex.unlock();
        }

        while (((MessageQueueService *)indicationsQueue)->getIncomingCount() >
                   INDICATIONS_Q_RESUME_THRESHOLD)
        {
            Threads::sleep(INDICATIONS_Q_STALL_DURATION);
        }

        AutoMutex indicationThresholdReportedAutoMutex1(
            indicationThresholdReportedLock);

        if (indicationThresholdReported)
        {
            indicationThresholdReported = false;
            indicationThresholdReportedAutoMutex1.unlock();

            Logger::put(
                Logger::STANDARD_LOG, System::CIMSERVER, Logger::INFORMATION,
                "Indication generation resumed: current queue count = $0",
                ((MessageQueueService *)indicationsQueue)->getIncomingCount());

        }
        else
        {
            indicationThresholdReportedAutoMutex1.unlock();
        }
    }
#endif /* INDICATIONS_Q_STALL_THRESHOLD */

}

void ProviderManagerService::providerModuleFailureCallback
    (const String & moduleName,
     const String & userName,
     Uint16 userContext)
{
    PEG_METHOD_ENTER (TRC_PROVIDERMANAGER,
        "ProviderManagerService::providerModuleFailureCallback");

    if (userContext == PG_PROVMODULE_USERCTXT_REQUESTOR)
    {
        Logger::put_l(
            Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
            MessageLoaderParms(
                "ProviderManager.OOPProviderManagerRouter."
                    "OOP_PROVIDER_MODULE_USER_CTXT_FAILURE_DETECTED",
                "A failure was detected in provider module $0 with "
                    "user context $1.",
                moduleName, userName));
    }
    else  //  not requestor context
    {
        Logger::put_l(
            Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
            MessageLoaderParms(
                "ProviderManager.OOPProviderManagerRouter."
                    "OOP_PROVIDER_MODULE_FAILURE_DETECTED",
                "A failure was detected in provider module $0.",
                moduleName));
    }

    //
    // Send Notify Provider Fail request message to Indication Service.
    // (After a CIMStopAllProvidersRequestMessage is processed, the
    // IndicationService may be destructed, so it cannot be used then.)
    //

    if (_indicationServiceQueueId == PEG_NOT_FOUND)
    {
        MessageQueue* indicationServiceQueue =
            lookup(PEGASUS_QUEUENAME_INDICATIONSERVICE);

        if (indicationServiceQueue)
        {
            _indicationServiceQueueId = indicationServiceQueue->getQueueId();
        }
    }

    if (!_allProvidersStopped && (_indicationServiceQueueId != PEG_NOT_FOUND))
    {
        //
        //  Create Notify Provider Fail request message
        //
        CIMNotifyProviderFailRequestMessage* request =
            new CIMNotifyProviderFailRequestMessage(
                XmlWriter::getNextMessageId(),
                moduleName,
                userName,
                QueueIdStack());

        request->queueIds = QueueIdStack(
            _indicationServiceQueueId, providerManagerService->getQueueId());

        AsyncLegacyOperationStart asyncRequest(
            0,
            _indicationServiceQueueId,
            request);

        AutoPtr <AsyncReply> asyncReply(
            providerManagerService->SendWait(&asyncRequest));

        AutoPtr <CIMNotifyProviderFailResponseMessage> response(
            reinterpret_cast<CIMNotifyProviderFailResponseMessage *>(
                (dynamic_cast<AsyncLegacyOperationResult *>(
                    asyncReply.get()))->get_result()));

        if (response->cimException.getCode () != CIM_ERR_SUCCESS)
        {
            PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL2,
                "Unexpected exception in providerModuleFailureCallback: %s",
                (const char*)response->cimException.getMessage().getCString()));
        }
        else
        {
            //
            //  Successful response
            //  Examine result to see if any subscriptions were affected
            //
            if (response->numSubscriptionsAffected > 0)
            {
                //
                //  Subscriptions were affected
                //  Update the provider module status to Degraded
                //
                try
                {
                    CIMInstance providerModule;
                    CIMKeyBinding keyBinding(
                        _PROPERTY_PROVIDERMODULE_NAME,
                        moduleName,
                        CIMKeyBinding::STRING);
                    Array<CIMKeyBinding> kbArray;
                    kbArray.append(keyBinding);
                    CIMObjectPath modulePath("", PEGASUS_NAMESPACENAME_INTEROP,
                        PEGASUS_CLASSNAME_PROVIDERMODULE, kbArray);
                    providerModule =
                        providerManagerService->_providerRegistrationManager->
                            getInstance(
                                modulePath, false, false, CIMPropertyList());

                    Array<Uint16> removeStatus;
                    Array<Uint16> appendStatus;
                    removeStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);
                    appendStatus.append(CIM_MSE_OPSTATUS_VALUE_DEGRADED);
                    providerManagerService->_updateProviderModuleStatus(
                        providerModule, removeStatus, appendStatus);
                }
                catch (const Exception & e)
                {
                    PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                        "Failed to update provider module status: %s",
                        (const char*)e.getMessage().getCString()));
                }

                //
                //  Log a warning message since subscriptions were affected
                //
                Logger::put_l(
                    Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
                    MessageLoaderParms(
                        "ProviderManager.OOPProviderManagerRouter."
                            "OOP_PROVIDER_MODULE_SUBSCRIPTIONS_AFFECTED",
                        "The generation of indications by providers in module "
                            "$0 may be affected.  To ensure these providers "
                            "are serving active subscriptions, disable and "
                            "then re-enable this module using the cimprovider "
                            "command.",
                        moduleName));
            }
        }
    }

    PEG_METHOD_EXIT();
}

PEGASUS_NAMESPACE_END
