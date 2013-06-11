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
//%////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  ConsoleManager
/////////////////////////////////////////////////////////////////////////////

#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/Logger.h>
#include <Pegasus/Common/PegasusVersion.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/AuditLogger.h>
#include <Pegasus/Config/ConfigManager.h>

#include <sys/__messag.h>

#include "ConsoleManager_zOS.h"
#include "CIMServer.h"

PEGASUS_NAMESPACE_BEGIN


#define ZOSCONSOLEMANAGER_TOKEN_APPL    "CONFIG,"
#define ZOSCONSOLEMANAGER_TOKEN_PLANNED "PLANNED"


char* ZOSConsoleManager::skipBlanks( char* commandPtr)
{
    if (commandPtr != NULL)
    {
        while (*commandPtr == ' ')
        {
            commandPtr++;
        }
    }

    return commandPtr;
}

void ZOSConsoleManager::stripTrailingBlanks( char* token )
{
    if (token != NULL)
    {
        int len = strlen(token)-1;

        while ((len >= 0) && (token[len] == ' '))
        {
            token[len] = '\0';
            len--;
        }
    }

    return;
}

void ZOSConsoleManager::issueSyntaxError(const char* command)
{
    PEG_METHOD_ENTER(TRC_SERVER,
        "ZOSConsoleManager::issueSyntaxError");
    Logger::put_l(
        Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
        MessageLoaderParms(
            "Server.ConsoleManager_zOS.CON_SYNTAX_ERR.PEGASUS_OS_ZOS",
            "CIM MODIFY COMMAND REJECTED DUE TO SYNTAX ERROR"));
    Logger::put_l(
        Logger::STANDARD_LOG, System::CIMSERVER, Logger::INFORMATION,
        MessageLoaderParms(
            "Server.ConsoleManager_zOS.CON_MODIFY_SYNTAX.PEGASUS_OS_ZOS",
            "Syntax is: MODIFY CFZCIM,APPL=CONFIG,<name>=<value>[,PLANNED]"));

    PEG_METHOD_EXIT();
    return;
}


void ZOSConsoleManager::updateConfiguration( const String& configProperty,
                                             const String& propertyValue,
                                             Boolean currentValueIsNull,
                                             Boolean planned)
{
    PEG_METHOD_ENTER(TRC_SERVER,
        "ZOSConsoleManager::updateConfiguration");

    String preValue;
    String currentValue;

    try
    {
        ConfigManager* _configManager = ConfigManager::getInstance();

        preValue = _configManager->getCurrentValue(configProperty);

        if (!planned)
        {
            //
            // Update the current value
            //
            if ( !_configManager->updateCurrentValue(
                                      configProperty,
                                      propertyValue,
                                      System::getEffectiveUserName(),
                                      0,
                                      currentValueIsNull) )
            {
                Logger::put_l(
                    Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
                    MessageLoaderParms(
                        "Server.ConsoleManager_zOS."
                            "CON_MODIFY_FAILED.PEGASUS_OS_ZOS",
                        "Failed to update CONFIG value."));
            }
            else
            {
                Logger::put_l(
                    Logger::STANDARD_LOG, System::CIMSERVER,
                    Logger::INFORMATION,
                    MessageLoaderParms(
                        "Server.ConsoleManager_zOS."
                            "CON_MODIFY_UPDATED.PEGASUS_OS_ZOS",
                        "Updated current value for $0 to $1",
                        configProperty, propertyValue));
            }
        }
        else
        {
            //
            // Update the planned value
            //
            if ( !_configManager->updatePlannedValue(configProperty,
                                                     propertyValue,
                                                     currentValueIsNull) )
            {
                Logger::put_l(
                    Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
                    MessageLoaderParms(
                        "Server.ConsoleManager_zOS."
                            "CON_MODIFY_FAILED.PEGASUS_OS_ZOS",
                        "Failed to update CONFIG value."));
            }
            else
            {
                Logger::put_l(
                    Logger::STANDARD_LOG, System::CIMSERVER,
                    Logger::INFORMATION,
                    MessageLoaderParms(
                        "Server.ConsoleManager_zOS."
                            "CON_MODIFY_PLANNED.PEGASUS_OS_ZOS",
                        "Updated planned value for $0 to $1",
                        configProperty, propertyValue));
                Logger::put_l(
                    Logger::STANDARD_LOG, System::CIMSERVER,
                    Logger::INFORMATION,
                    MessageLoaderParms(
                        "Server.ConsoleManager_zOS."
                            "CON_MODIFY_PLANNED2.PEGASUS_OS_ZOS",
                        "This change will become effective "
                        "after CIM Server restart."));
            }
        }

        // It is unset, get current value which is default
        if (currentValueIsNull)
        {
            currentValue = _configManager->getCurrentValue(configProperty);
        }
        else
        {
            currentValue = propertyValue;
        }

        // send notify config change message to ProviderManager Service
        _sendNotifyConfigChangeMessage(String(configProperty),
                                       currentValue,
                                          !planned);

        PEG_AUDIT_LOG(logSetConfigProperty("OPERATOR",
                                           configProperty,
                                           preValue,
                                           currentValue,
                                           planned));

    }
    catch (const NonDynamicConfigProperty& ndcp)
    {
        Logger::put_l(
            Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
            MessageLoaderParms(
                "Server.ConsoleManager_zOS.CON_MODIFY_ERR.PEGASUS_OS_ZOS",
                "MODIFY command failed: \"$0\"",
                ndcp.getMessage()));
    }
    catch (const InvalidPropertyValue& ipv)
    {
        Logger::put_l(
            Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
            MessageLoaderParms(
                "Server.ConsoleManager_zOS.CON_MODIFY_ERR.PEGASUS_OS_ZOS",
                "MODIFY command failed: \"$0\"",
                ipv.getMessage()));
    }
    catch (const UnrecognizedConfigProperty&)
    {
        Logger::put_l(
            Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
            MessageLoaderParms(
                "Server.ConsoleManager_zOS.CON_MODIFY_INVALID.PEGASUS_OS_ZOS",
                "$0 is not a valid configuration property",
                configProperty));
    }

    PEG_METHOD_EXIT();
    return;
}


void ZOSConsoleManager::processModifyCommand( char* command )
{
    PEG_METHOD_ENTER(TRC_SERVER,
        "ZOSConsoleManager::processModifyCommand");

    char* currentPtr = command;
    char* cmdPtr = NULL;
    char* cfgProperty = NULL;
    char* cfgValue = NULL;
    char* planned = NULL;
    Boolean currentValueIsNull = false;


    currentPtr = skipBlanks(currentPtr);

    if (!memcmp(currentPtr,STRLIT_ARGS(ZOSCONSOLEMANAGER_TOKEN_APPL)))
    {
        currentPtr += strlen(ZOSCONSOLEMANAGER_TOKEN_APPL);
    }
    else
    {
        issueSyntaxError(command);
        return;
    }

    currentPtr = skipBlanks(currentPtr);

    cfgProperty = currentPtr;
    currentPtr = strchr(currentPtr,'=');

    if (currentPtr==NULL)
    {
        issueSyntaxError(command);
        return;
    }
    else
    {
        // skip the "="
        *currentPtr = '\0';
        currentPtr++;

        currentPtr = skipBlanks(currentPtr);

        if (*currentPtr == '\0')
        {
            currentValueIsNull=true;
        }
        else if (*currentPtr == '\'')
        {
            char* temp = strchr(currentPtr+1,'\'');
            if (temp!=NULL)
            {
                // skip the starting "'"
                *currentPtr = '\0';
                currentPtr++;

                cfgValue = currentPtr;
                currentPtr = temp;

                // skip the ending "'"
                *currentPtr = '\0';
                currentPtr++;
            }
            else
            {
                issueSyntaxError(command);
                return;
            }
        }
        else
        {
            cfgValue = currentPtr;
        }
    }

    currentPtr = skipBlanks(currentPtr);

    planned = strchr(currentPtr,',');
    if (planned!=NULL)
    {
        *planned = '\0';
        planned++;

        planned = skipBlanks(planned);

        if (memcmp(planned,STRLIT_ARGS(ZOSCONSOLEMANAGER_TOKEN_PLANNED)))
        {
            issueSyntaxError(command);
            return;
        }
    }


    if (cfgProperty != NULL)
    {
        stripTrailingBlanks( cfgProperty );

        PEG_TRACE((TRC_SERVER, Tracer::LEVEL4,
            "Update property: %s", cfgProperty));
    }

    if (currentValueIsNull)
    {
        PEG_TRACE_CSTRING(TRC_SERVER, Tracer::LEVEL4,
            "Set property with null value");
    }
    else if (cfgValue != NULL)
    {
        stripTrailingBlanks( cfgValue );

        PEG_TRACE((TRC_SERVER, Tracer::LEVEL4,
            "Update property value to: %s", cfgValue));
    }

    if (planned != NULL)
    {
        PEG_TRACE_CSTRING(TRC_SERVER, Tracer::LEVEL4,
            "Updating planned value");
    }

    String propertyString(cfgProperty);
    String propertyValue;

    if (!currentValueIsNull)
    {
         propertyValue.assign(cfgValue);
    }

    updateConfiguration(propertyString,
                        propertyValue,
                        currentValueIsNull,
                        planned);
    PEG_METHOD_EXIT();
    return;
}


void ZOSConsoleManager::startConsoleWatchThread(void)
{
    PEG_METHOD_ENTER(TRC_SERVER,
        "ZOSConsoleManager::startConsoleWatchThread");

    pthread_t thid;

    if ( pthread_create(&thid,
                        NULL,
                        ZOSConsoleManager::consoleCommandWatchThread,
                        NULL) != 0 )
    {
        char str_errno2[10];
        sprintf(str_errno2,"%08X",__errno2());
        Logger::put_l(Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
            MessageLoaderParms(
                "Server.ConsoleManager_zOS.NO_CONSOLE_THREAD.PEGASUS_OS_ZOS",
                "CIM Server Console command thread cannot be created: "
                    "$0 ( errno $1, reason code 0x$2 ).",
                strerror(errno),
                errno,
                str_errno2));
    }

    PEG_METHOD_EXIT();
    return;
}



//
// z/OS console interface waiting for operator stop command
//
void* ZOSConsoleManager::consoleCommandWatchThread(void*)
{
    PEG_METHOD_ENTER(TRC_SERVER,
        "ZOSConsoleManager::consoleCommandWatchThread");

    struct __cons_msg    cons;
    int                  concmd=0;
    char                 modstr[128];
    int                  rc;

    memset(&cons,0,sizeof(cons));
    memset(modstr,0,sizeof(modstr));

    do
    {
        rc = __console(&cons, modstr, &concmd);

        if (rc != 0)
        {
            int errornumber = errno;
            char str_errno2[10];
            sprintf(str_errno2,"%08X",__errno2());

            Logger::put_l(
                Logger::ERROR_LOG, System::CIMSERVER, Logger::SEVERE,
                MessageLoaderParms(
                    "Server.ConsoleManager_zOS.CONSOLE_ERROR.PEGASUS_OS_ZOS",
                    "Console Communication Service failed:"
                        "$0 ( errno $1, reason code 0x$2 ).",
                    strerror(errornumber),
                    errornumber,
                    str_errno2));

            break;
        }

        // Check if we received a stop command from the console
        if (concmd == _CC_modify)
        {
            // Ensure the command we received from the console is
            // null terminated.
            modstr[127] = '\0';

            PEG_TRACE((TRC_SERVER, Tracer::LEVEL4,
                "Received MODIFY command: %s", modstr));

            processModifyCommand(modstr);
        }
        else if (concmd != _CC_stop)
        {
            // Just issue a console message that the command was
            // not recognized and wait again for the stop command.
            Logger::put_l(
                Logger::STANDARD_LOG, System::CIMSERVER, Logger::INFORMATION,
                MessageLoaderParms(
                    "Server.ConsoleManager_zOS.CONSOLE_NO_MODIFY."
                        "PEGASUS_OS_ZOS",
                    "Command not recognized by CIM server."));
        }
        else
        {
            Logger::put_l(
                Logger::STANDARD_LOG, System::CIMSERVER, Logger::INFORMATION,
                MessageLoaderParms(
                    "Server.ConsoleManager_zOS.CONSOLE_STOP.PEGASUS_OS_ZOS",
                    "STOP command received from z/OS console,"
                        " initiating shutdown."));
        }

    // keep on until we encounter an error or received a STOP
    } while ( (concmd != _CC_stop) && (rc == 0) );

    CIMServer::shutdownSignal();

    PEG_METHOD_EXIT();
    pthread_exit(0);

    return NULL;
}


//
// Send notify config change message to provider manager service
// This code was borrowed from the ConfigSettingProvider and should
// be kept in sync.
// The purpose is to ensure that OOP agents also get the update.
// TBD, or is it for other reasons as well?
//
void ZOSConsoleManager::_sendNotifyConfigChangeMessage(
    const String& propertyName,
    const String& newPropertyValue,
    Boolean currentValueModified)
{
    PEG_METHOD_ENTER(TRC_SERVER,
        "ZOSConsoleManager::_sendNotifyConfigChangeMessage");

    ModuleController* controller = ModuleController::getModuleController();

    MessageQueue * queue = MessageQueue::lookup(
        PEGASUS_QUEUENAME_PROVIDERMANAGER_CPP);

    MessageQueueService * service = dynamic_cast<MessageQueueService *>(queue);

    if (service != NULL)
    {
        // create CIMNotifyConfigChangeRequestMessage
        CIMNotifyConfigChangeRequestMessage * notify_req =
            new CIMNotifyConfigChangeRequestMessage (
            XmlWriter::getNextMessageId (),
            propertyName,
            newPropertyValue,
            currentValueModified,
            QueueIdStack(service->getQueueId()));

        // create request envelope
        AsyncLegacyOperationStart asyncRequest(
            NULL,
            service->getQueueId(),
            notify_req);

        AutoPtr<AsyncReply> asyncReply(
            controller->ClientSendWait(service->getQueueId(), &asyncRequest));

        AutoPtr<CIMNotifyConfigChangeResponseMessage> response(
            reinterpret_cast<CIMNotifyConfigChangeResponseMessage *>(
            (static_cast<AsyncLegacyOperationResult *>
            (asyncReply.get()))->get_result()));

        if (response->cimException.getCode() != CIM_ERR_SUCCESS)
        {
            CIMException e = response->cimException;
            PEG_METHOD_EXIT();
            throw (e);
        }
    }
    PEG_METHOD_EXIT();
}



PEGASUS_NAMESPACE_END
