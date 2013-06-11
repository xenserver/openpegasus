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

#include <Pegasus/Common/Signal.h>
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Common/CIMMessageSerializer.h>
#include <Pegasus/Common/CIMMessageDeserializer.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Common/System.h>
#include <Pegasus/Common/AnonymousPipe.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/Logger.h>
#include <Pegasus/Common/Thread.h>
#include <Pegasus/Common/MessageQueueService.h>
#include <Pegasus/Config/ConfigManager.h>
#include <Pegasus/Common/Executor.h>
#include <Pegasus/Common/StringConversion.h>
#include <Pegasus/Common/SCMOClassCache.h>

#if defined (PEGASUS_OS_TYPE_WINDOWS)
# include <windows.h>  // For CreateProcess()
#elif defined (PEGASUS_OS_VMS)
# include <perror.h>
# include <climsgdef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <processes.h>
# include <unixio.h>
#else
# include <unistd.h>  // For fork(), exec(), and _exit()
# include <errno.h>
# include <sys/types.h>
# include <sys/resource.h>
# if defined(PEGASUS_HAS_SIGNALS)
#  include <sys/wait.h>
# endif
#endif

#include "OOPProviderManagerRouter.h"

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////////////////
// OutstandingRequestTable and OutstandingRequestEntry
/////////////////////////////////////////////////////////////////////////////

/**
    An OutstandingRequestEntry represents a request message sent to a
    Provider Agent for which no response has been received.  The request
    sender provides the message ID and a location for the response to be
    returned, and then waits on the semaphore.  When a response matching
    the message ID is received, it is placed into the specified location
    and the semaphore is signaled.
 */
class OutstandingRequestEntry
{
public:
    OutstandingRequestEntry(
        String originalMessageId_,
        CIMRequestMessage* requestMessage_,
        CIMResponseMessage*& responseMessage_,
        Semaphore* responseReady_)
        : originalMessageId(originalMessageId_),
          requestMessage(requestMessage_),
          responseMessage(responseMessage_),
          responseReady(responseReady_)
    {
    }

    /**
        A unique value is substituted as the request messageId attribute to
        allow responses to be definitively correllated with requests.
        The original messageId value is stored here to avoid a race condition
        between the processing of a response chunk and the resetting of the
        original messageId in the request message.
     */
    String originalMessageId;
    CIMRequestMessage* requestMessage;
    CIMResponseMessage*& responseMessage;
    Semaphore* responseReady;
};

typedef HashTable<String, OutstandingRequestEntry*, EqualFunc<String>,
    HashFunc<String> > OutstandingRequestTable;


/////////////////////////////////////////////////////////////////////////////
// ProviderAgentContainer
/////////////////////////////////////////////////////////////////////////////

class ProviderAgentContainer
{
public:
    ProviderAgentContainer(
        const String & moduleName,
        const String & userName,
        Uint16 userContext,
        PEGASUS_INDICATION_CALLBACK_T indicationCallback,
        PEGASUS_RESPONSE_CHUNK_CALLBACK_T responseChunkCallback,
        PEGASUS_PROVIDERMODULEFAIL_CALLBACK_T providerModuleFailCallback,
        Boolean subscriptionInitComplete);

    ~ProviderAgentContainer();

    Boolean isInitialized();

    String getModuleName() const;

    CIMResponseMessage* processMessage(CIMRequestMessage* request);
    void unloadIdleProviders();

private:
    //
    // Private methods
    //

    /** Unimplemented */
    ProviderAgentContainer();
    /** Unimplemented */
    ProviderAgentContainer(const ProviderAgentContainer& pa);
    /** Unimplemented */
    ProviderAgentContainer& operator=(const ProviderAgentContainer& pa);

    /**
        Start a Provider Agent process and establish a pipe connection with it.
        Note: The caller must lock the _agentMutex.
     */
    void _startAgentProcess();

    /**
        Send initialization data to the Provider Agent.
        Note: The caller must lock the _agentMutex.
     */
    void _sendInitializationData();

    /**
        Initialize the ProviderAgentContainer if it is not already
        initialized.  Initialization includes starting the Provider Agent
        process, establishing a pipe connection with it, and starting a
        thread to read response messages from the Provider Agent.

        Note: The caller must lock the _agentMutex.
     */
    void _initialize();

    /**
        Uninitialize the ProviderAgentContainer if it is initialized.
        The connection is closed and outstanding requests are completed
        with an error result.

        @param cleanShutdown Indicates whether the provider agent process
        exited cleanly.  A value of true indicates that responses have been
        sent for all requests that have been processed.  A value of false
        indicates that one or more requests may have been partially processed.
     */
    void _uninitialize(Boolean cleanShutdown);

    /**
        Performs the processMessage work, but does not retry on a transient
        error.
     */
    CIMResponseMessage* _processMessage(CIMRequestMessage* request);

    /**
        Read and process response messages from the Provider Agent until
        the connection is closed.
     */
    void _processResponses();
    static ThreadReturnType PEGASUS_THREAD_CDECL
        _responseProcessor(void* arg);

    /**
        Process the ProvAgtGetScmoClassRequestMessage and sends the
        requested SCMOClass back to the agent.
     */
    void _processGetSCMOClassRequest(
        ProvAgtGetScmoClassRequestMessage* request);

    //
    // Private data
    //

    /**
        The _agentMutex must be locked whenever writing to the Provider
        Agent connection, accessing the _isInitialized flag, or changing
        the Provider Agent state.
     */
    Mutex _agentMutex;

    /**
        Name of the provider module served by this Provider Agent.
     */
    String _moduleName;

    /**
        The user context in which this Provider Agent operates.
     */
    String _userName;

    /**
        User Context setting of the provider module served by this Provider
        Agent.
     */
    Uint16 _userContext;

    /**
        Callback function to which all generated indications are sent for
        processing.
     */
    PEGASUS_INDICATION_CALLBACK_T _indicationCallback;

    /**
        Callback function to which response chunks are sent for processing.
     */
    PEGASUS_RESPONSE_CHUNK_CALLBACK_T _responseChunkCallback;

    /**
        Callback function to be called upon detection of failure of a
        provider module.
     */
    PEGASUS_PROVIDERMODULEFAIL_CALLBACK_T _providerModuleFailCallback;

    /**
        Indicates whether the Provider Agent is active.
     */
    Boolean _isInitialized;

    /**
        Pipe connection used to read responses from the Provider Agent.
     */
    AutoPtr<AnonymousPipe> _pipeFromAgent;
    /**
        Pipe connection used to write requests to the Provider Agent.
     */
    AutoPtr<AnonymousPipe> _pipeToAgent;

#if defined(PEGASUS_HAS_SIGNALS)
    /**
        Process ID of the active Provider Agent.
     */
    pid_t _pid;
#endif

    /**
        The _outstandingRequestTable holds an entry for each request that has
        been sent to this Provider Agent for which no response has been
        received.  Entries are added (by the writing thread) when a request
        is sent, and are removed (by the reading thread) when the response is
        received (or when it is determined that no response is forthcoming).
     */
    OutstandingRequestTable _outstandingRequestTable;
    /**
        The _outstandingRequestTableMutex must be locked whenever reading or
        updating the _outstandingRequestTable.
     */
    Mutex _outstandingRequestTableMutex;

    /**
        Holds the last provider module instance sent to the Provider Agent in
        a ProviderIdContainer.  Since the provider module instance rarely
        changes, an optimization is used to send it only when it differs from
        the last provider module instance sent.
     */
    CIMInstance _providerModuleCache;

    /**
        The number of Provider Agent processes that are currently initialized
        (active).
    */
    static Uint32 _numProviderProcesses;

    /**
        The _numProviderProcessesMutex must be locked whenever reading or
        updating the _numProviderProcesses count.
    */
    static Mutex _numProviderProcessesMutex;

    /**
        A value indicating that a request message has not been processed.
        A CIMResponseMessage pointer with this value indicates that the
        corresponding CIMRequestMessage has not been processed.  This is
        used to indicate that a provider agent exited without starting to
        process the request, and that the request should be retried.
     */
    static CIMResponseMessage* _REQUEST_NOT_PROCESSED;

    /**
        Indicates whether the Indication Service has completed initialization.

        For more information, please see the description of the
        ProviderManagerRouter::_subscriptionInitComplete member variable.
     */
    Boolean _subscriptionInitComplete;
};

Uint32 ProviderAgentContainer::_numProviderProcesses = 0;
Mutex ProviderAgentContainer::_numProviderProcessesMutex;

// Set this to a value that no valid CIMResponseMessage* will have.
CIMResponseMessage* ProviderAgentContainer::_REQUEST_NOT_PROCESSED =
    static_cast<CIMResponseMessage*>((void*)&_REQUEST_NOT_PROCESSED);

ProviderAgentContainer::ProviderAgentContainer(
    const String & moduleName,
    const String & userName,
    Uint16 userContext,
    PEGASUS_INDICATION_CALLBACK_T indicationCallback,
    PEGASUS_RESPONSE_CHUNK_CALLBACK_T responseChunkCallback,
    PEGASUS_PROVIDERMODULEFAIL_CALLBACK_T providerModuleFailCallback,
    Boolean subscriptionInitComplete)
    :
      _moduleName(moduleName),
      _userName(userName),
      _userContext(userContext),
      _indicationCallback(indicationCallback),
      _responseChunkCallback(responseChunkCallback),
      _providerModuleFailCallback(providerModuleFailCallback),
      _isInitialized(false),
      _subscriptionInitComplete(subscriptionInitComplete)
{

    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::ProviderAgentContainer");
    PEG_METHOD_EXIT();
}

ProviderAgentContainer::~ProviderAgentContainer()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::~ProviderAgentContainer");

    // Ensure the destructor does not throw an exception
    try
    {
        if (isInitialized())
        {
            {
                AutoMutex lock(_agentMutex);
                // Check if the _pipeFromAgent is alive.
                if( _pipeFromAgent.get() != 0 )
                {
                    // Stop the responseProcessor thread by closing its
                    // connection.
                    _pipeFromAgent->closeReadHandle();
                }
            }

            // Wait for the responseProcessor thread to exit
            while (isInitialized())
            {
                Threads::yield();
            }
        }
    }
    catch (...)
    {
    }

    PEG_METHOD_EXIT();
}

void ProviderAgentContainer::_startAgentProcess()
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER, "ProviderAgentContainer::_startAgentProcess");

    // Start the provider agent.

    int pid;
    AnonymousPipe* readPipe;
    AnonymousPipe* writePipe;

    int status = Executor::startProviderAgent(
        (const char*)_moduleName.getCString(),
        ConfigManager::getPegasusHome(),
        _userName,
        pid,
        readPipe,
        writePipe);

    if (status != 0)
    {
        PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
            "Executor::startProviderAgent() failed"));
        PEG_METHOD_EXIT();
        throw Exception(MessageLoaderParms(
            "ProviderManager.OOPProviderManagerRouter.CIMPROVAGT_START_FAILED",
            "Failed to start cimprovagt \"$0\".",
            _moduleName));
    }

# if defined(PEGASUS_HAS_SIGNALS)
    _pid = pid;
# endif

    _pipeFromAgent.reset(readPipe);
    _pipeToAgent.reset(writePipe);

    PEG_METHOD_EXIT();
}

// Note: Caller must lock _agentMutex
void ProviderAgentContainer::_sendInitializationData()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::_sendInitializationData");

    //
    // Gather config properties to pass to the Provider Agent
    //
    ConfigManager* configManager = ConfigManager::getInstance();
    Array<Pair<String, String> > configProperties;

    Array<String> configPropertyNames;
    configManager->getAllPropertyNames(configPropertyNames, true);
    for (Uint32 i = 0; i < configPropertyNames.size(); i++)
    {
        String configPropertyValue =
            configManager->getCurrentValue(configPropertyNames[i]);
        String configPropertyDefaultValue =
            configManager->getDefaultValue(configPropertyNames[i]);
        if (configPropertyValue != configPropertyDefaultValue)
        {
            configProperties.append(Pair<String, String>(
                configPropertyNames[i], configPropertyValue));
        }
    }

    //
    // Create a Provider Agent initialization message
    //
    AutoPtr<CIMInitializeProviderAgentRequestMessage> request(
        new CIMInitializeProviderAgentRequestMessage(
            String("0"),    // messageId
            configManager->getPegasusHome(),
            configProperties,
            System::bindVerbose,
            _subscriptionInitComplete,
            QueueIdStack()));

    //
    // Write the initialization message to the pipe
    //
    AnonymousPipe::Status writeStatus =
        _pipeToAgent->writeMessage(request.get());

    if (writeStatus != AnonymousPipe::STATUS_SUCCESS)
    {
        PEG_METHOD_EXIT();
        throw Exception(MessageLoaderParms(
            "ProviderManager.OOPProviderManagerRouter."
                "CIMPROVAGT_COMMUNICATION_FAILED",
            "Failed to communicate with cimprovagt \"$0\".",
            _moduleName));
    }

    // Wait for a null response from the Provider Agent indicating it has
    // initialized successfully.

    CIMMessage* message;
    AnonymousPipe::Status readStatus;
    do
    {
        readStatus = _pipeFromAgent->readMessage(message);

    } while (readStatus == AnonymousPipe::STATUS_INTERRUPT);

    if (readStatus != AnonymousPipe::STATUS_SUCCESS)
    {
        PEG_METHOD_EXIT();
        throw Exception(MessageLoaderParms(
            "ProviderManager.OOPProviderManagerRouter."
                "CIMPROVAGT_COMMUNICATION_FAILED",
            "Failed to communicate with cimprovagt \"$0\".",
            _moduleName));
    }

    PEGASUS_ASSERT(message == 0);

    PEG_METHOD_EXIT();
}

// Note: Caller must lock _agentMutex
void ProviderAgentContainer::_initialize()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::_initialize");

    if (_isInitialized)
    {
        PEGASUS_ASSERT(0);
        PEG_METHOD_EXIT();
        return;
    }

    //Get the current value of maxProviderProcesses
    String maxProviderProcessesString = ConfigManager::getInstance()->
        getCurrentValue("maxProviderProcesses");
    Uint64 v;
    StringConversion::decimalStringToUint64(
        maxProviderProcessesString.getCString(),
        v);
    Uint32 maxProviderProcesses = (Uint32)v;

    char* end = 0;

    {
        AutoMutex lock(_numProviderProcessesMutex);

        if ((maxProviderProcesses != 0) &&
            (_numProviderProcesses >= maxProviderProcesses))
        {
            throw PEGASUS_CIM_EXCEPTION(
                CIM_ERR_FAILED,
                MessageLoaderParms(
                    "ProviderManager.OOPProviderManagerRouter."
                        "MAX_PROVIDER_PROCESSES_REACHED",
                    "The maximum number of cimprovagt processes has been "
                        "reached."));
        }
        else
        {
            _numProviderProcesses++;
        }
    }

    try
    {
        _startAgentProcess();
        _isInitialized = true;
        _sendInitializationData();

        // Start a thread to read and process responses from the Provider Agent
        ThreadStatus rtn = PEGASUS_THREAD_OK;
        while ((rtn = MessageQueueService::get_thread_pool()->
                   allocate_and_awaken(this, _responseProcessor)) !=
               PEGASUS_THREAD_OK)
        {
            if (rtn == PEGASUS_THREAD_INSUFFICIENT_RESOURCES)
            {
                Threads::yield();
            }
            else
            {
                PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL1,
                    "Could not allocate thread to process responses from the "
                        "provider agent.");

                throw Exception(MessageLoaderParms(
                    "ProviderManager.OOPProviderManagerRouter."
                        "CIMPROVAGT_THREAD_ALLOCATION_FAILED",
                    "Failed to allocate thread for cimprovagt \"$0\".",
                    _moduleName));
            }
        }
    }
    catch (...)
    {
        // Closing the connection causes the agent process to exit
        _pipeToAgent.reset();
        _pipeFromAgent.reset();

#if defined(PEGASUS_HAS_SIGNALS)
        if (_isInitialized)
        {
            // Harvest the status of the agent process to prevent a zombie
            int status = Executor::reapProviderAgent(_pid);

            if (status == -1)
            {
                PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
                    "ProviderAgentContainer::_initialize(): "
                        "Executor::reapProviderAgent() failed"));
            }
        }
#endif

        _isInitialized = false;

        {
            AutoMutex lock(_numProviderProcessesMutex);
            _numProviderProcesses--;
        }

        PEG_METHOD_EXIT();
        throw;
    }

    PEG_METHOD_EXIT();
}

Boolean ProviderAgentContainer::isInitialized()
{
    AutoMutex lock(_agentMutex);
    return _isInitialized;
}

void ProviderAgentContainer::_uninitialize(Boolean cleanShutdown)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::_uninitialize");

#if defined(PEGASUS_HAS_SIGNALS)
    pid_t pid = 0;
#endif

    try
    {
        AutoMutex lock(_agentMutex);

        PEGASUS_ASSERT(_isInitialized);

        // Close the connection with the Provider Agent
        _pipeFromAgent.reset();
        _pipeToAgent.reset();

        _providerModuleCache = CIMInstance();

        {
            AutoMutex lock2(_numProviderProcessesMutex);
            _numProviderProcesses--;
        }

        _isInitialized = false;

#if defined(PEGASUS_HAS_SIGNALS)
        // Save the _pid so we can use it after we've released the _agentMutex
        pid = _pid;
#endif

        //
        // Complete with null responses all outstanding requests on this
        // connection
        //
        {
            AutoMutex tableLock(_outstandingRequestTableMutex);

            CIMResponseMessage* response =
                cleanShutdown ? _REQUEST_NOT_PROCESSED : 0;

            for (OutstandingRequestTable::Iterator i =
                     _outstandingRequestTable.start();
                 i != 0; i++)
            {
                PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL4,
                    "Completing messageId \"%s\" with a null response.",
                    (const char*)i.key().getCString()));
                i.value()->responseMessage = response;
                i.value()->responseReady->signal();
            }

            _outstandingRequestTable.clear();
        }

        //
        //  If not a clean shutdown, call the provider module failure callback
        //
        if (!cleanShutdown)
        {
            //
            // Call the provider module failure callback to communicate
            // the failure to the Provider Manager Service.  The Provider
            // Manager Service will inform the Indication Service.
            //
            _providerModuleFailCallback(_moduleName, _userName, _userContext);
        }
    }
    catch (...)
    {
        // We're uninitializing, so do not propagate the exception
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL2,
            "Ignoring _uninitialize() exception.");
    }

#if defined(PEGASUS_HAS_SIGNALS)
    // Harvest the status of the agent process to prevent a zombie.  Do not
    // hold the _agentMutex during this operation.

    if ((pid != 0) && (Executor::reapProviderAgent(pid) == -1))
    {
        PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL2,
            "ProviderAgentContainer::_uninitialize(): "
                "Executor::reapProviderAgent() failed."));
    }
#endif

    PEG_METHOD_EXIT();
}

String ProviderAgentContainer::getModuleName() const
{
    return _moduleName;
}

CIMResponseMessage* ProviderAgentContainer::processMessage(
    CIMRequestMessage* request)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::processMessage");

    CIMResponseMessage* response;

    do
    {
        response = _processMessage(request);

        if (response == _REQUEST_NOT_PROCESSED)
        {
            // Check for request message types that should not be retried.
            if ((request->getType() ==
                     CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE) ||
                (request->getType() ==
                     CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE) ||
                (request->getType() ==
                     CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE) ||
                (request->getType() ==
                     CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE) ||
                (request->getType() ==
                     CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE))
            {
                response = request->buildResponse();
                break;
            }
            else if (request->getType() == CIM_DISABLE_MODULE_REQUEST_MESSAGE)
            {
                CIMDisableModuleResponseMessage* dmResponse =
                    dynamic_cast<CIMDisableModuleResponseMessage*>(response);
                PEGASUS_ASSERT(dmResponse != 0);

                Array<Uint16> operationalStatus;
                operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_STOPPED);
                dmResponse->operationalStatus = operationalStatus;
                break;
            }
        }
    } while (response == _REQUEST_NOT_PROCESSED);

    if (request->getType() == CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE)
    {
        _subscriptionInitComplete = true;
    }
    else if (request->getType() ==
        CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE)
    {
        _subscriptionInitComplete = false;
    }


    PEG_METHOD_EXIT();
    return response;
}

CIMResponseMessage* ProviderAgentContainer::_processMessage(
    CIMRequestMessage* request)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::_processMessage");

    CIMResponseMessage* response;
    String originalMessageId = request->messageId;

    PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL3,
        "ProviderAgentContainer, process message ID %s",
        (const char*)request->messageId.getCString()));

    // These three variables are used for the provider module optimization.
    // See the _providerModuleCache member description for more information.
    AutoPtr<ProviderIdContainer> origProviderId;
    Boolean doProviderModuleOptimization = false;
    Boolean updateProviderModuleCache = false;

    try
    {
        // The messageId attribute is used to correlate response messages
        // from the Provider Agent with request messages, so it is imperative
        // that the ID is unique for each request.  The incoming ID cannot be
        // trusted to be unique, so we substitute a unique one.  The memory
        // address of the request is used as the source of a unique piece of
        // data.  (The message ID is only required to be unique while the
        // request is outstanding.)
        char messagePtrString[20];
        sprintf(messagePtrString, "%p", request);
        String uniqueMessageId = messagePtrString;

        //
        // Set up the OutstandingRequestEntry for this request
        //
        Semaphore waitSemaphore(0);
        OutstandingRequestEntry outstandingRequestEntry(
            originalMessageId, request, response, &waitSemaphore);

        //
        // Lock the Provider Agent Container while initializing the
        // agent and writing the request to the connection
        //
        {
            AutoMutex lock(_agentMutex);

            //
            // Initialize the Provider Agent, if necessary
            //
            if (!_isInitialized)
            {
                _initialize();
            }

            //
            // Add an entry to the OutstandingRequestTable for this request
            //
            {
                AutoMutex tableLock(_outstandingRequestTableMutex);

                _outstandingRequestTable.insert(
                    uniqueMessageId, &outstandingRequestEntry);
            }

            // Get the provider module from the ProviderIdContainer to see if
            // we can optimize out the transmission of this instance to the
            // Provider Agent.  (See the _providerModuleCache description.)
            if (request->operationContext.contains(ProviderIdContainer::NAME))
            {
                ProviderIdContainer pidc = request->operationContext.get(
                    ProviderIdContainer::NAME);
                origProviderId.reset(new ProviderIdContainer(
                    pidc.getModule(), pidc.getProvider(),
                    pidc.isRemoteNameSpace(), pidc.getRemoteInfo()));
                if (_providerModuleCache.isUninitialized() ||
                    (!pidc.getModule().identical(_providerModuleCache)))
                {
                    // We haven't sent this provider module instance to the
                    // Provider Agent yet.  Update our cache after we send it.
                    updateProviderModuleCache = true;
                }
                else
                {
                    // Replace the provider module in the ProviderIdContainer
                    // with an uninitialized instance.  We'll need to put the
                    // original one back after the message is sent.
                    request->operationContext.set(ProviderIdContainer(
                        CIMInstance(), pidc.getProvider(),
                        pidc.isRemoteNameSpace(), pidc.getRemoteInfo()));
                    doProviderModuleOptimization = true;
                }
            }

            //
            // Write the message to the pipe
            //
            try
            {
                PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL3,
                    "Sending request to agent with messageId %s",
                    (const char*)uniqueMessageId.getCString()));

                request->messageId = uniqueMessageId;
                AnonymousPipe::Status writeStatus =
                    _pipeToAgent->writeMessage(request);
                request->messageId = originalMessageId;

                if (doProviderModuleOptimization)
                {
                    request->operationContext.set(*origProviderId.get());
                }

                if (writeStatus != AnonymousPipe::STATUS_SUCCESS)
                {
                    PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
                        "Failed to write message to pipe.  writeStatus = %d.",
                        writeStatus));

                    request->messageId = originalMessageId;

                    if (doProviderModuleOptimization)
                    {
                        request->operationContext.set(*origProviderId.get());
                    }

                    // Remove this OutstandingRequestTable entry
                    {
                        AutoMutex tableLock(_outstandingRequestTableMutex);
                        Boolean removed =
                            _outstandingRequestTable.remove(uniqueMessageId);
                        PEGASUS_ASSERT(removed);
                    }

                    // A response value of _REQUEST_NOT_PROCESSED indicates
                    // that the request was not processed by the provider
                    // agent, so it can be retried safely.
                    PEG_METHOD_EXIT();
                    return _REQUEST_NOT_PROCESSED;
                }

                if (updateProviderModuleCache)
                {
                    _providerModuleCache = origProviderId->getModule();
                }
            }
            catch (...)
            {
                request->messageId = originalMessageId;

                if (doProviderModuleOptimization)
                {
                    request->operationContext.set(*origProviderId.get());
                }

                PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL1,
                    "Failed to write message to pipe.");
                // Remove the OutstandingRequestTable entry for this request
                {
                    AutoMutex tableLock(_outstandingRequestTableMutex);
                    Boolean removed =
                        _outstandingRequestTable.remove(uniqueMessageId);
                    PEGASUS_ASSERT(removed);
                }
                PEG_METHOD_EXIT();
                throw;
            }
        }

        //
        // Wait for the response
        //
        try
        {
            // Must not hold _agentMutex while waiting for the response
            waitSemaphore.wait();
        }
        catch (...)
        {
            // Remove the OutstandingRequestTable entry for this request
            {
                AutoMutex tableLock(_outstandingRequestTableMutex);
                Boolean removed =
                    _outstandingRequestTable.remove(uniqueMessageId);
                PEGASUS_ASSERT(removed);
            }
            PEG_METHOD_EXIT();
            throw;
        }

        // A response value of _REQUEST_NOT_PROCESSED indicates that the
        // provider agent process was terminating when the request was sent.
        // The request was not processed by the provider agent, so it can be
        // retried safely.
        if (response == _REQUEST_NOT_PROCESSED)
        {
            PEG_METHOD_EXIT();
            return response;
        }

        // A null response is returned when an agent connection is closed
        // while requests remain outstanding.
        if (response == 0)
        {
            response = request->buildResponse();
            response->cimException = PEGASUS_CIM_EXCEPTION(
                CIM_ERR_FAILED,
                MessageLoaderParms(
                    "ProviderManager.OOPProviderManagerRouter."
                        "CIMPROVAGT_CONNECTION_LOST",
                    "Lost connection with cimprovagt \"$0\".",
                    _moduleName));
        }
    }
    catch (CIMException& e)
    {
        PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
            "Caught CIMException: %s",
            (const char*)e.getMessage().getCString()));
        response = request->buildResponse();
        response->cimException = e;
    }
    catch (Exception& e)
    {
        PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
            "Caught Exception: %s",
            (const char*)e.getMessage().getCString()));
        response = request->buildResponse();
        response->cimException = PEGASUS_CIM_EXCEPTION(
            CIM_ERR_FAILED, e.getMessage());
    }
    catch (...)
    {
        PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
            "Caught unknown exception");
        response = request->buildResponse();
        response->cimException = PEGASUS_CIM_EXCEPTION(
            CIM_ERR_FAILED, String::EMPTY);
    }

    response->messageId = originalMessageId;
    response->syncAttributes(request);

    PEG_METHOD_EXIT();
    return response;
}

void ProviderAgentContainer::unloadIdleProviders()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::unloadIdleProviders");

    AutoMutex lock(_agentMutex);
    if (_isInitialized)
    {
        // Send a "wake up" message to the Provider Agent.
        // Don't bother checking whether the operation is successful.
        Uint32 messageLength = 0;
        _pipeToAgent->writeBuffer((const char*)&messageLength, sizeof(Uint32));
    }

    PEG_METHOD_EXIT();
}

void ProviderAgentContainer::_processGetSCMOClassRequest(
    ProvAgtGetScmoClassRequestMessage* request)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::_processGetSCMOClassRequest");

    AutoPtr<ProvAgtGetScmoClassResponseMessage> response(
        new ProvAgtGetScmoClassResponseMessage(
            XmlWriter::getNextMessageId(),
            CIMException(),
            QueueIdStack(),
            SCMOClass("","")));

    CString ns = request->nameSpace.getString().getCString();
    CString cn = request->className.getString().getCString();

    delete request;

    response->scmoClass = SCMOClassCache::getInstance()->getSCMOClass(
                              ns,strlen(ns),
                              cn,strlen(cn));

    //
    // Lock the Provider Agent Container and
    // writing the response to the connection
    //
    {

        AutoMutex lock(_agentMutex);

        //
        // Write the message to the pipe
        //
        try
        {

            AnonymousPipe::Status writeStatus =
                _pipeToAgent->writeMessage(response.get());

            if (writeStatus != AnonymousPipe::STATUS_SUCCESS)
            {
                PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
                    "Failed to write message to pipe.  writeStatus = %d.",
                    writeStatus));

                PEG_METHOD_EXIT();
                return;
            }

        }
        catch (Exception & e)
        {
            PEG_TRACE((TRC_PROVIDERMANAGER, Tracer::LEVEL1,
                "Exception: Failed to write message to pipe. Error: %s",
                       (const char*)e.getMessage().getCString()));
            PEG_METHOD_EXIT();
            throw;
        }
        catch (...)
        {

            PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL1,
                "Unkonwn exception. Failed to write message to pipe.");
            PEG_METHOD_EXIT();
            throw;
        }
    }

    PEG_METHOD_EXIT();
    return;
}

void ProviderAgentContainer::_processResponses()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "ProviderAgentContainer::_processResponses");

    //
    // Process responses until the pipe is closed
    //
    while (1)
    {
        try
        {
            CIMMessage* message;

            //
            // Read a response from the Provider Agent
            //
            AnonymousPipe::Status readStatus =
                _pipeFromAgent->readMessage(message);

            // Ignore interrupts
            if (readStatus == AnonymousPipe::STATUS_INTERRUPT)
            {
                continue;
            }

            // Handle an error the same way as a closed connection
            if ((readStatus == AnonymousPipe::STATUS_ERROR) ||
                (readStatus == AnonymousPipe::STATUS_CLOSED))
            {
                _uninitialize(false);
                return;
            }

            // A null message indicates that the provider agent process has
            // finished its processing and is ready to exit.
            if (message == 0)
            {
                _uninitialize(true);
                return;
            }

            if (message->getType() == CIM_PROCESS_INDICATION_REQUEST_MESSAGE)
            {
                // Process an indication message

                _indicationCallback(
                    reinterpret_cast<CIMProcessIndicationRequestMessage*>(
                        message));
            }
            else if (message->getType()==PROVAGT_GET_SCMOCLASS_REQUEST_MESSAGE)
            {

                _processGetSCMOClassRequest(
                    reinterpret_cast<ProvAgtGetScmoClassRequestMessage*>(
                        message));
            }
            else if (!message->isComplete())
            {
                // Process an incomplete response chunk

                CIMResponseMessage* response;
                response = dynamic_cast<CIMResponseMessage*>(message);
                PEGASUS_ASSERT(response != 0);

                // Get the OutstandingRequestEntry for this response chunk
                OutstandingRequestEntry* _outstandingRequestEntry = 0;
                {
                    AutoMutex tableLock(_outstandingRequestTableMutex);
                    Boolean foundEntry = _outstandingRequestTable.lookup(
                        response->messageId, _outstandingRequestEntry);
                    PEGASUS_ASSERT(foundEntry);
                }

                // Put the original message ID into the response
                response->messageId =
                    _outstandingRequestEntry->originalMessageId;

                // Call the response chunk callback to process the chunk
                _responseChunkCallback(
                    _outstandingRequestEntry->requestMessage, response);
            }
            else
            {
                // Process a completed response

                CIMResponseMessage* response;
                response = dynamic_cast<CIMResponseMessage*>(message);
                PEGASUS_ASSERT(response != 0);

                // Give the response to the waiting OutstandingRequestEntry
                OutstandingRequestEntry* _outstandingRequestEntry = 0;
                {
                    AutoMutex tableLock(_outstandingRequestTableMutex);
                    Boolean foundEntry = _outstandingRequestTable.lookup(
                        response->messageId, _outstandingRequestEntry);
                    PEGASUS_ASSERT(foundEntry);

                    // Remove the completed request from the table
                    Boolean removed =
                        _outstandingRequestTable.remove(response->messageId);
                    PEGASUS_ASSERT(removed);
                }

                _outstandingRequestEntry->responseMessage = response;
                _outstandingRequestEntry->responseReady->signal();
            }
        }
        catch (Exception& e)
        {
            PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL2,
                "Ignoring exception: %s",
                (const char*)e.getMessage().getCString()));
        }
        catch (...)
        {
            PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL2,
                "Ignoring exception");
        }
    }
}

ThreadReturnType PEGASUS_THREAD_CDECL
ProviderAgentContainer::_responseProcessor(void* arg)
{
    ProviderAgentContainer* pa =
        reinterpret_cast<ProviderAgentContainer*>(arg);

    pa->_processResponses();

    return ThreadReturnType(0);
}

/////////////////////////////////////////////////////////////////////////////
// OOPProviderManagerRouter
/////////////////////////////////////////////////////////////////////////////

OOPProviderManagerRouter::OOPProviderManagerRouter(
    PEGASUS_INDICATION_CALLBACK_T indicationCallback,
    PEGASUS_RESPONSE_CHUNK_CALLBACK_T responseChunkCallback,
    PEGASUS_PROVIDERMODULEFAIL_CALLBACK_T providerModuleFailCallback)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "OOPProviderManagerRouter::OOPProviderManagerRouter");

    _indicationCallback = indicationCallback;
    _responseChunkCallback = responseChunkCallback;
    _providerModuleFailCallback = providerModuleFailCallback;
    _subscriptionInitComplete = false;

    PEG_METHOD_EXIT();
}

OOPProviderManagerRouter::~OOPProviderManagerRouter()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "OOPProviderManagerRouter::~OOPProviderManagerRouter");

    try
    {
        // Clean up the ProviderAgentContainers
        AutoMutex lock(_providerAgentTableMutex);
        ProviderAgentTable::Iterator i = _providerAgentTable.start();
        for (; i != 0; i++)
        {
            delete i.value();
        }
    }
    catch (...) {}

    PEG_METHOD_EXIT();
}

Message* OOPProviderManagerRouter::processMessage(Message* message)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "OOPProviderManagerRouter::processMessage");

    CIMRequestMessage* request = dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0);

    AutoPtr<CIMResponseMessage> response;

    //
    // Get the provider information from the request
    //
    CIMInstance providerModule;

    if ((dynamic_cast<CIMOperationRequestMessage*>(request) != 0) ||
        (dynamic_cast<CIMIndicationRequestMessage*>(request) != 0) ||
        (request->getType() == CIM_EXPORT_INDICATION_REQUEST_MESSAGE))
    {
        // Provider information is in the OperationContext
        ProviderIdContainer pidc = (ProviderIdContainer)
            request->operationContext.get(ProviderIdContainer::NAME);
        providerModule = pidc.getModule();
    }
    else if (request->getType() == CIM_ENABLE_MODULE_REQUEST_MESSAGE)
    {
        CIMEnableModuleRequestMessage* emReq =
            dynamic_cast<CIMEnableModuleRequestMessage*>(request);
        providerModule = emReq->providerModule;
    }
    else if (request->getType() == CIM_DISABLE_MODULE_REQUEST_MESSAGE)
    {
        CIMDisableModuleRequestMessage* dmReq =
            dynamic_cast<CIMDisableModuleRequestMessage*>(request);
        providerModule = dmReq->providerModule;
    }
    else if ((request->getType() == CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE) ||
             (request->getType() ==
                 CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE) ||
             (request->getType() ==
                 CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE) ||
             (request->getType() == CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE))
    {
        // This operation is not provider-specific
    }
    else
    {
        // Unrecognized message type.  This should never happen.
        PEGASUS_ASSERT(0);
        response.reset(request->buildResponse());
        response->cimException = PEGASUS_CIM_EXCEPTION(
            CIM_ERR_FAILED, "Unrecognized message type.");
        PEG_METHOD_EXIT();
        return response.release();
    }

    //
    // Process the request message
    //
    if (request->getType() == CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE)
    {
        // Forward the CIMStopAllProvidersRequest to all providers
        response.reset(_forwardRequestToAllAgents(request));

        // Note: Do not uninitialize the ProviderAgentContainers here.
        // Just let the selecting thread notice when the agent connections
        // are closed.
    }
    else if (request->getType () ==
        CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE)
    {
        _subscriptionInitComplete = true;

        //
        //  Forward the CIMSubscriptionInitCompleteRequestMessage to
        //  all providers
        //
        response.reset (_forwardRequestToAllAgents (request));
    }
    else if (request->getType () ==
        CIM_INDICATION_SERVICE_DISABLED_REQUEST_MESSAGE)
    {
        _subscriptionInitComplete = false;

        //
        //  Forward the CIMIndicationServiceDisabledRequestMessage to
        //  all providers
        //
        response.reset (_forwardRequestToAllAgents (request));
    }
    else if (request->getType() == CIM_NOTIFY_CONFIG_CHANGE_REQUEST_MESSAGE)
    {
        CIMNotifyConfigChangeRequestMessage* notifyRequest =
            dynamic_cast<CIMNotifyConfigChangeRequestMessage*>(request);
        PEGASUS_ASSERT(notifyRequest != 0);

        if (notifyRequest->currentValueModified)
        {
            // Forward the CIMNotifyConfigChangeRequestMessage to all providers
            response.reset(_forwardRequestToAllAgents(request));
        }
        else
        {
            // No need to notify provider agents about changes to planned value
            response.reset(request->buildResponse());
        }
    }
    else if (request->getType() == CIM_DISABLE_MODULE_REQUEST_MESSAGE)
    {
        // Fan out the request to all Provider Agent processes for this module

        // Retrieve the provider module name
        String moduleName;
        _getProviderModuleName(providerModule,moduleName);

        // Look up the Provider Agents for this module
        Array<ProviderAgentContainer*> paArray =
            _lookupProviderAgents(moduleName);

        for (Uint32 i=0; i<paArray.size(); i++)
        {
            //
            // Do not start up an agent process just to disable the module
            //
            if (paArray[i]->isInitialized())
            {
                //
                // Forward the request to the provider agent
                //
                response.reset(paArray[i]->processMessage(request));

                // Note: Do not uninitialize the ProviderAgentContainer here
                // when a disable module operation is successful.  Just let the
                // selecting thread notice when the agent connection is closed.

                // Determine the success of the disable module operation
                CIMDisableModuleResponseMessage* dmResponse =
                    dynamic_cast<CIMDisableModuleResponseMessage*>(
                        response.get());
                PEGASUS_ASSERT(dmResponse != 0);

                Boolean isStopped = false;
                for (Uint32 j=0; j < dmResponse->operationalStatus.size(); j++)
                {
                    if (dmResponse->operationalStatus[j] ==
                        CIM_MSE_OPSTATUS_VALUE_STOPPED)
                    {
                        isStopped = true;
                        break;
                    }
                }

                // If the operation is unsuccessful, stop and return the error
                if ((dmResponse->cimException.getCode() != CIM_ERR_SUCCESS) ||
                    !isStopped)
                {
                    break;
                }
            }
        }

        // Use a default response if no Provider Agents were called
        if (!response.get())
        {
            response.reset(request->buildResponse());

            CIMDisableModuleResponseMessage* dmResponse =
                dynamic_cast<CIMDisableModuleResponseMessage*>(response.get());
            PEGASUS_ASSERT(dmResponse != 0);

            Array<Uint16> operationalStatus;
            operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_STOPPED);
            dmResponse->operationalStatus = operationalStatus;
        }
    }
    else if (request->getType() == CIM_ENABLE_MODULE_REQUEST_MESSAGE)
    {
        // Fan out the request to all Provider Agent processes for this module

        // Retrieve the provider module name
        String moduleName;
        _getProviderModuleName(providerModule,moduleName);

        // Look up the Provider Agents for this module
        Array<ProviderAgentContainer*> paArray =
            _lookupProviderAgents(moduleName);

        for (Uint32 i=0; i<paArray.size(); i++)
        {
            //
            // Do not start up an agent process just to enable the module
            //
            if (paArray[i]->isInitialized())
            {
                //
                // Forward the request to the provider agent
                //
                response.reset(paArray[i]->processMessage(request));

                // Determine the success of the enable module operation
                CIMEnableModuleResponseMessage* emResponse =
                    dynamic_cast<CIMEnableModuleResponseMessage*>(
                        response.get());
                PEGASUS_ASSERT(emResponse != 0);

                Boolean isOk = false;
                for (Uint32 j=0; j < emResponse->operationalStatus.size(); j++)
                {
                    if (emResponse->operationalStatus[j] ==
                        CIM_MSE_OPSTATUS_VALUE_OK)
                    {
                        isOk = true;
                        break;
                    }
                }

                // If the operation is unsuccessful, stop and return the error
                if ((emResponse->cimException.getCode() != CIM_ERR_SUCCESS) ||
                    !isOk)
                {
                    break;
                }
            }
        }

        // Use a default response if no Provider Agents were called
        if (!response.get())
        {
            response.reset(request->buildResponse());

            CIMEnableModuleResponseMessage* emResponse =
                dynamic_cast<CIMEnableModuleResponseMessage*>(response.get());
            PEGASUS_ASSERT(emResponse != 0);

            Array<Uint16> operationalStatus;
            operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);
            emResponse->operationalStatus = operationalStatus;
        }
    }
    else
    {
        //
        // Look up the Provider Agent for this module instance and requesting
        // user
        //
        ProviderAgentContainer* pa = _lookupProviderAgent(providerModule,
            request);
        PEGASUS_ASSERT(pa != 0);

        //
        // Forward the request to the provider agent
        //
        response.reset(pa->processMessage(request));
    }

    PEG_METHOD_EXIT();
    return response.release();
}

ProviderAgentContainer* OOPProviderManagerRouter::_lookupProviderAgent(
    const CIMInstance& providerModule,
    CIMRequestMessage* request)
{
    // Retrieve the provider module name
    String moduleName;
    _getProviderModuleName(providerModule,moduleName);

#if defined(PEGASUS_OS_ZOS)
    // For z/OS we don't start an extra provider agent for
    // each user, since the userid is switched at the thread
    // level. Therefore we set the userName to an empty String,
    // as it is used below to build the key for the provider
    // agent table
    String userName;
    Uint16 userContext = PEGASUS_DEFAULT_PROV_USERCTXT;
#else

    // Retrieve the provider user context configuration
    Uint16 userContext = 0;
    Uint32 pos = providerModule.findProperty(
        PEGASUS_PROPERTYNAME_MODULE_USERCONTEXT);
    if (pos != PEG_NOT_FOUND)
    {
        CIMValue userContextValue =
            providerModule.getProperty(pos).getValue();
        if (!userContextValue.isNull())
        {
            userContextValue.get(userContext);
        }
    }

    if (userContext == 0)
    {
        // PASE has a default user context "QYCMCIMOM",
        // so we leave userContext unset here.
#ifndef PEGASUS_OS_PASE
        userContext = PEGASUS_DEFAULT_PROV_USERCTXT;
#endif
    }

    String userName;

    if (userContext == PG_PROVMODULE_USERCTXT_REQUESTOR)
    {
        if (request->operationContext.contains(IdentityContainer::NAME))
        {
            // User Name is in the OperationContext
            IdentityContainer ic = (IdentityContainer)
                request->operationContext.get(IdentityContainer::NAME);
            userName = ic.getUserName();
        }
        //else
        //{
        //    If no IdentityContainer is present, default to the CIM
        //    Server's user context
        //}

        // If authentication is disabled, use the CIM Server's user context
        if (!userName.size())
        {
            userName = System::getEffectiveUserName();
        }
    }
    else if (userContext == PG_PROVMODULE_USERCTXT_DESIGNATED)
    {
        // Retrieve the provider module designated user property value
        providerModule.getProperty(providerModule.findProperty(
            PEGASUS_PROPERTYNAME_MODULE_DESIGNATEDUSER)).getValue().
            get(userName);
    }
    else if (userContext == PG_PROVMODULE_USERCTXT_CIMSERVER)
    {
        userName = System::getEffectiveUserName();
    }
#ifdef PEGASUS_OS_PASE // it might be unset user in PASE in this branch.
    else if (userContext == 0)
    {
        userName = "QYCMCIMOM";
    }
#endif
    else    // Privileged User
    {
        PEGASUS_ASSERT(userContext == PG_PROVMODULE_USERCTXT_PRIVILEGED);
        userName = System::getPrivilegedUserName();
    }

    PEG_TRACE((
        TRC_PROVIDERMANAGER,
        Tracer::LEVEL4,
        "Module name = %s, User context = %hd, User name = %s",
        (const char*) moduleName.getCString(),
        userContext,
        (const char*) userName.getCString()));
#endif

    ProviderAgentContainer* pa = 0;
#ifdef PEGASUS_OS_PASE
    String userUpper = userName;
    userUpper.toUpper();
    String key = moduleName + ":" + userUpper;
#else
    String key = moduleName + ":" + userName;
#endif

    AutoMutex lock(_providerAgentTableMutex);
    if (!_providerAgentTable.lookup(key, pa))
    {
        pa = new ProviderAgentContainer(
            moduleName, userName, userContext,
            _indicationCallback, _responseChunkCallback,
            _providerModuleFailCallback,
            _subscriptionInitComplete);
        _providerAgentTable.insert(key, pa);
    }
    return pa;
}

Array<ProviderAgentContainer*> OOPProviderManagerRouter::_lookupProviderAgents(
    const String& moduleName)
{
    Array<ProviderAgentContainer*> paArray;

    AutoMutex lock(_providerAgentTableMutex);
    for (ProviderAgentTable::Iterator i = _providerAgentTable.start(); i; i++)
    {
        if (i.value()->getModuleName() == moduleName)
        {
            paArray.append(i.value());
        }
    }
    return paArray;
}

CIMResponseMessage* OOPProviderManagerRouter::_forwardRequestToAllAgents(
    CIMRequestMessage* request)
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "OOPProviderManagerRouter::_forwardRequestToAllAgents");

    // Get a list of the ProviderAgentContainers.  We need our own array copy
    // because we cannot hold the _providerAgentTableMutex while calling
    // _ProviderAgentContainer::processMessage().
    Array<ProviderAgentContainer*> paContainerArray;
    {
        AutoMutex tableLock(_providerAgentTableMutex);
        for (ProviderAgentTable::Iterator i = _providerAgentTable.start();
             i != 0; i++)
        {
            paContainerArray.append(i.value());
        }
    }

    CIMException responseException;

    // Forward the request to each of the initialized provider agents
    for (Uint32 j = 0; j < paContainerArray.size(); j++)
    {
        ProviderAgentContainer* pa = paContainerArray[j];
        if (pa->isInitialized())
        {
            // Note: The ProviderAgentContainer could become uninitialized
            // before _ProviderAgentContainer::processMessage() processes
            // this request.  In this case, the Provider Agent process will
            // (unfortunately) be started to process this message.
            AutoPtr<CIMResponseMessage> response;
            response.reset(pa->processMessage(request));
            if (response.get() != 0)
            {
                // If the operation failed, save the exception data
                if ((response->cimException.getCode() != CIM_ERR_SUCCESS) &&
                    (responseException.getCode() == CIM_ERR_SUCCESS))
                {
                    responseException = response->cimException;
                }
            }
        }
    }

    CIMResponseMessage* response = request->buildResponse();
    response->cimException = responseException;

    PEG_METHOD_EXIT();
    return response;
}

void OOPProviderManagerRouter::unloadIdleProviders()
{
    PEG_METHOD_ENTER(TRC_PROVIDERMANAGER,
        "OOPProviderManagerRouter::unloadIdleProviders");

    // Get a list of the ProviderAgentContainers.  We need our own array copy
    // because we cannot hold the _providerAgentTableMutex while calling
    // ProviderAgentContainer::unloadIdleProviders().
    Array<ProviderAgentContainer*> paContainerArray;
    {
        AutoMutex tableLock(_providerAgentTableMutex);
        for (ProviderAgentTable::Iterator i = _providerAgentTable.start();
             i != 0; i++)
        {
            paContainerArray.append(i.value());
        }
    }

    // Iterate through the _providerAgentTable unloading idle providers
    for (Uint32 j = 0; j < paContainerArray.size(); j++)
    {
        paContainerArray[j]->unloadIdleProviders();
    }

    PEG_METHOD_EXIT();
}

void OOPProviderManagerRouter::_getProviderModuleName(
        const CIMInstance & providerModule,
        String & moduleName)
{
    CIMValue nameValue = providerModule.getProperty(
        providerModule.findProperty(PEGASUS_PROPERTYNAME_NAME)).getValue();
    nameValue.get(moduleName);

#if defined(PEGASUS_OS_ZOS)
    // Retrieve the providers shareAS flag, to see if it will share the
    // provider address space with other providers or requests its own
    // address space.
    Boolean shareAS = true;
    Uint32 saIndex = providerModule.findProperty("ShareAS");
    if (saIndex != PEG_NOT_FOUND)
    {
        CIMValue shareValue=providerModule.getProperty(saIndex).getValue();
        shareValue.get(shareAS);
    }
    if (shareAS == true)
    {
        moduleName.assign("SharedProviderAgent");
    }
#endif
    return;
}


PEGASUS_NAMESPACE_END
