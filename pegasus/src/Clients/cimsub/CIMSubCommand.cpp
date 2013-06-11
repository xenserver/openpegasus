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

#include <iostream>
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/System.h>
#include <Pegasus/Client/CIMClient.h>
#include <Pegasus/Common/Exception.h>
#include <Pegasus/Common/PegasusVersion.h>
#include <Pegasus/Common/AutoPtr.h>

#ifdef PEGASUS_OS_ZOS
#include <Pegasus/General/SetFileDescriptorToEBCDICEncoding.h>
#endif

#include <Pegasus/getoopt/getoopt.h>
#include <Clients/cliutils/CommandException.h>
#include "CIMSubCommand.h"

PEGASUS_NAMESPACE_BEGIN


/**
 * The CLI message resource name
 */

static const char MSG_PATH [] = "pegasus/pegasusCLI";

/**
    The command name.
 */
const char COMMAND_NAME[] = "cimsub";

/**
   The default subscription namespace
 */
static const CIMNamespaceName _DEFAULT_SUBSCRIPTION_NAMESPACE  =
    PEGASUS_NAMESPACENAME_INTEROP;

/**
    The usage string for this command.  This string is displayed
    when an error occurs in parsing or validating the command line.
 */
static const char USAGE[] = "Usage: ";

/*
    These constants represent the operation modes supported by the CLI.
    Any new operation should be added here.
 */

/**
    This constant signifies that an operation option has not been recorded
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_UNINITIALIZED = 0;

/**
    This constant represents a disable subscription operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_DISABLE = 1;

/**
    This constant represents a enable subscription operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_ENABLE = 2;

/**
    This constant represents a list operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_LIST = 3;

/**
    This constant represents a remove operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_REMOVE = 4;

/**
    This constant represents a help operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_HELP = 5;

/**
    This constant represents a verbose list operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_VERBOSE = 6;

/**
    This constant represents a version display operation
 */
const Uint32 CIMSubCommand::OPERATION_TYPE_VERSION = 7;

/**
    The constants representing the messages.
 */

static const char CIMOM_NOT_RUNNING[] =
    "The CIM server may not be running.";

static const char CIMOM_NOT_RUNNING_KEY[] =
    "Clients.cimsub.CIMSubCommand.CIMOM_NOT_RUNNING";

static const char SUBSCRIPTION_NOT_FOUND_FAILURE[] =
    "The requested subscription could not be found.";

static const char SUBSCRIPTION_NOT_FOUND_KEY[] =
    "Clients.cimsub.CIMSubCommand."
        "SUBSCRIPTION_NOT_FOUND_FAILURE_KEY";

static const char HANDLER_NOT_FOUND_FAILURE[] =
    "The requested handler could not be found.";

static const char HANDLER_NOT_FOUND_KEY[] =
    "Clients.cimsub.CIMSubCommand."
        "HANDLER_NOT_FOUND_FAILURE_KEY";

static const char FILTER_NOT_FOUND_FAILURE[] =
    "The requested filter could not be found.";

static const char FILTER_NOT_FOUND_KEY[] =
    "Clients.cimsub.CIMSubCommand."
        "FILTER_NOT_FOUND_FAILURE_KEY";

static const char SUBSCRIPTION_ALREADY_DISABLED[] =
    "The subscription is already disabled.";

static const char SUBSCRIPTION_ALREADY_DISABLED_KEY[] =
    "Clients.cimsub.CIMSubCommand.SUBSCRIPTION_ALREADY_DISABLED";

static const char SUBSCRIPTION_ALREADY_ENABLED[] =
    "The subscription is already enabled.";

static const char SUBSCRIPTION_ALREADY_ENABLED_KEY[] =
    "Clients.cimsub.CIMSubCommand.SUBSCRIPTION_ALREADY_ENABLED";

static const char REQUIRED_OPTION_MISSING[] =
    "Required option missing.";

static const char REQUIRED_OPTION_MISSING_KEY[] =
    "Clients.cimsub.CIMSubCommand.REQUIRED_OPTION_MISSING";

static const char ERR_USAGE_KEY[] =
    "Clients.cimsub.CIMSubCommand.ERR_USAGE";

static const char ERR_USAGE[] =
    "Use '--help' to obtain command syntax.";

static const char LONG_HELP[] = "help";

static const char LONG_VERSION[] = "version";

/**
    The option character used to specify disable a specified subscription
 */
static const char OPTION_DISABLE = 'd';

/**
    The option character used to specify remove a specified subscription
 */
static const char OPTION_REMOVE = 'r';

/**
    The option character used to specify the Filter Name of a subscription
 */
static const char OPTION_FILTER = 'F';

/**
    The option character used to specify enable a specified subscription.
 */
static const char OPTION_ENABLE = 'e';

/**
    The option character used to specify the Handler Name of a subscription
 */
static const char OPTION_HANDLER = 'H';

/**
    The option character used to specify listing
 */
static const char OPTION_LIST = 'l';

/**
    The option argument character used to specify subscriptions
 */
static String ARG_SUBSCRIPTIONS = "s";

/**
    The option argument character used to specify filters
 */
static String ARG_FILTERS = "f";

/**
    The option argument character used to specify handlers
 */
static String ARG_HANDLERS = "h";

/**
    The option argument character used to specify handlers, filters,
    and subscriptions
 */
static String ARG_ALL = "a";

/**
    The option character used to specify namespace of subscription
 */
static const char OPTION_NAMESPACE = 'n';

/**
    The option character used to display verbose info.
 */
static const char OPTION_VERBOSE = 'v';

static const char DELIMITER_NAMESPACE = ':';
static const char DELIMITER_HANDLER_CLASS = '.';

/**
    List output header values
 */

const Uint32 TITLE_SEPERATOR_LEN = 2;

static const Uint32 RC_CONNECTION_FAILED = 2;
static const Uint32 RC_CONNECTION_TIMEOUT = 3;
static const Uint32 RC_ACCESS_DENIED = 4;
static const Uint32 RC_NAMESPACE_NONEXISTENT = 5;
static const Uint32 RC_OBJECT_NOT_FOUND = 6;
static const Uint32 RC_OPERATION_NOT_SUPPORTED = 7;

//
// List column header constants
//
const Uint32 _HANDLER_LIST_NAME_COLUMN = 0;
const Uint32 _HANDLER_LIST_DESTINATION_COLUMN = 1;
const Uint32 _FILTER_LIST_NAME_COLUMN = 0;
const Uint32 _FILTER_LIST_QUERY_COLUMN = 1;
const Uint32 _FILTER_LIST_QUERYLANGUAGE_COLUMN = 2;
const Uint32 _SUBSCRIPTION_LIST_NS_COLUMN = 0;
const Uint32 _SUBSCRIPTION_LIST_FILTER_COLUMN = 1;
const Uint32 _SUBSCRIPTION_LIST_HANDLER_COLUMN = 2;
const Uint32 _SUBSCRIPTION_LIST_STATE_COLUMN = 3;
//
// Handler persistence display values
//
const String _PERSISTENTENCE_OTHER_STRING = "Other";
const String _PERSISTENTENCE_PERMANENT_STRING = "Permanent";
const String _PERSISTENTENCE_TRANSIENT_STRING = "Transient";
const String _PERSISTENTENCE_UNKNOWN_STRING = "Unknown";

//
// Subscription state display values
//
const String _SUBSCRIPTION_STATE_UNKNOWN_STRING = "Unknown";
const String _SUBSCRIPTION_STATE_OTHER_STRING = "Other";
const String _SUBSCRIPTION_STATE_ENABLED_STRING = "Enabled";
const String _SUBSCRIPTION_STATE_ENABLED_DEGRADED_STRING = "Enabled Degraded";
const String _SUBSCRIPTION_STATE_DISABLED_STRING = "Disabled";
const String _SUBSCRIPTION_STATE_NOT_SUPPORTED_STRING = "Not Supported";
//
// SNMP version display values
//
const String _SNMP_VERSION_SNMPV1_TRAP_STRING = "SNMPv1 Trap";
const String _SNMP_VERSION_SNMPV2C_TRAP_STRING = "SNMPv2C Trap";
const String _SNMP_VERSION_PEGASUS_RESERVED_STRING = "Pegasus Reserved";
/**

    Constructs a CIMSubCommand and initializes instance variables.
 */
CIMSubCommand::CIMSubCommand()
{
    /**
        Initialize the instance variables.
    */
    _operationType = OPERATION_TYPE_UNINITIALIZED;
    _verbose = false;


    /**
        Build the usage string for the config command.
    */

    usage.reserveCapacity(200);
    usage.append(USAGE);
    usage.append(COMMAND_NAME);
    usage.append(" -").append(OPTION_LIST);
    usage.append(" ").append(ARG_SUBSCRIPTIONS).append("|");
    usage.append(ARG_FILTERS).append("|");
    usage.append(ARG_HANDLERS);
    usage.append(" [-").append(OPTION_VERBOSE).append("]");
    usage.append(" [-").append(OPTION_NAMESPACE).append(" namespace]");
    usage.append(" [-").append(OPTION_FILTER).append
        (" [fnamespace:]filtername] \n");

    usage.append("                  [-").append(OPTION_HANDLER).append
        (" [hnamespace:][hclassname.]handlername] \n");

    usage.append("              -").append(OPTION_ENABLE);
    usage.append(" [-").append(OPTION_NAMESPACE).append(" namespace]");
    usage.append(" -").append(OPTION_FILTER).append
        (" [fnamespace:]filtername \n");

    usage.append("                  -").append(OPTION_HANDLER).append
        (" [hnamespace:][hclassname.]handlername \n");

    usage.append("              -").append(OPTION_DISABLE);
    usage.append(" [-").append(OPTION_NAMESPACE).append(" namespace]");
    usage.append(" -").append(OPTION_FILTER).append
        (" [fnamespace:]filtername\n");
    usage.append("                  -").append(OPTION_HANDLER).append
        (" [hnamespace:][hclassname.]handlername \n");

    usage.append("              -").append(OPTION_REMOVE);
    usage.append(" ").append (ARG_SUBSCRIPTIONS).append("|");
    usage.append(ARG_FILTERS).append("|");
    usage.append(ARG_HANDLERS).append("|");
    usage.append(ARG_ALL);
    usage.append(" [-").append(OPTION_NAMESPACE).append(" namespace]");
    usage.append(" [-").append(OPTION_FILTER).append
        ("[fnamespace:]filtername] \n");
    usage.append("                  [-").append(OPTION_HANDLER).append
        (" [hnamespace:][hclassname.]handlername]\n");

    usage.append("              --").append(LONG_HELP).append("\n");
    usage.append("              --").append(LONG_VERSION).append("\n");

    usage.append("Options : \n");
    usage.append("    -l         - List and display information\n");
    usage.append("    -e         - Enable specified subscription\n");
    usage.append("                   (set SubscriptionState to Enabled) \n");
    usage.append("    -d         - Disable specified subscription \n");
    usage.append("                   (set SubscriptionState to Disabled) \n");
    usage.append("    -r         - Remove specified subscription, handler,"
                                " filter \n");
    usage.append("    -v         - Include verbose information \n");
    usage.append("    -F         - Specify Filter Name of subscription for"
                                " the operation\n");
    usage.append("    -H         - Specify Handler Name of subscription for"
                                " the operation\n");
    usage.append("    -n         - Specify namespace of subscription\n");
    usage.append("                   (root/PG_InterOp, if not specified) \n");
    usage.append("    --help     - Display this help message\n");
    usage.append("    --version  - Display CIM Server version\n");
    usage.append("\n");
    usage.append("Usage note: The cimsub command requires that the CIM Server"
                                " is running.\n");
    usage.append("\n");

#ifdef PEGASUS_HAS_ICU

    MessageLoaderParms menuparms(
            "Clients.cimsub.CIMSubCommand.MENU.STANDARD",usage);
    menuparms.msg_src_path = MSG_PATH;
    usage = MessageLoader::getMessage(menuparms);

#endif

    setUsage(usage);
}



/**
    Parses the command line, validates the options, and sets instance
    variables based on the option arguments.
*/
void CIMSubCommand::setCommand(
    ostream& outPrintWriter,
    ostream& errPrintWriter,
    Uint32 argc,
    char* argv[])
{
    Uint32 i = 0;
    Uint32 c = 0;
    String badOptionString;
    String optString;
    String filterNameString;
    String handlerNameString;
    Boolean filterSet = false;
    Boolean handlerSet = false;

    //
    //  Construct optString
    //
    optString.append(OPTION_LIST);
    optString.append(getoopt::GETOPT_ARGUMENT_DESIGNATOR);
    optString.append(OPTION_DISABLE);
    optString.append(OPTION_ENABLE);
    optString.append(OPTION_REMOVE);
    optString.append(getoopt::GETOPT_ARGUMENT_DESIGNATOR);
    optString.append(OPTION_VERBOSE);
    optString.append(OPTION_FILTER);
    optString.append(getoopt::GETOPT_ARGUMENT_DESIGNATOR);
    optString.append(OPTION_HANDLER);
    optString.append(getoopt::GETOPT_ARGUMENT_DESIGNATOR);
    optString.append(OPTION_NAMESPACE);
    optString.append(getoopt::GETOPT_ARGUMENT_DESIGNATOR);

    //
    //  Initialize and parse options
    //
    getoopt options("");
    options.addFlagspec(optString);

    options.addLongFlagspec(LONG_HELP,getoopt::NOARG);
    options.addLongFlagspec(LONG_VERSION,getoopt::NOARG);

    options.parse(argc, argv);

    if (options.hasErrors())
    {
        throw CommandFormatException(options.getErrorStrings()[0]);
    }
    _operationType = OPERATION_TYPE_UNINITIALIZED;


    //
    //  Get options and arguments from the command line
    //
    for (i = options.first(); i <  options.last(); i++)
    {
        if (options[i].getType () == Optarg::LONGFLAG)
        {
            if (options[i].getopt() == LONG_HELP)
            {
                if (_operationType != OPERATION_TYPE_UNINITIALIZED)
                {
                    //
                    // More than one operation option was found
                    //
                    throw UnexpectedOptionException(LONG_HELP);
                }

               _operationType = OPERATION_TYPE_HELP;
            }
            else if (options[i].getopt() == LONG_VERSION)
            {
                if (_operationType != OPERATION_TYPE_UNINITIALIZED)
                {
                    //
                    // More than one operation option was found
                    //
                    throw UnexpectedOptionException(LONG_VERSION);
                }

                _operationType = OPERATION_TYPE_VERSION;
            }
        }
        else if (options [i].getType() == Optarg::REGULAR)
        {
            //
            //  The cimsub command has no non-option argument options
            //
            throw UnexpectedArgumentException(options[i].Value());
        }
        else
        {

            c = options[i].getopt()[0];

            switch (c)
            {
                case OPTION_DISABLE:
                {
                    if (_operationType != OPERATION_TYPE_UNINITIALIZED)
                    {
                        //
                        // More than one operation option was found
                        //
                        throw UnexpectedOptionException(OPTION_DISABLE);
                    }

                    if (options.isSet(OPTION_DISABLE) > 1)
                    {
                        //
                        // More than one disable subscription option was found
                        //
                        throw DuplicateOptionException(OPTION_DISABLE);
                    }

                    _operationType = OPERATION_TYPE_DISABLE;

                    break;
                }

                case OPTION_ENABLE:
                {
                    if (_operationType != OPERATION_TYPE_UNINITIALIZED)
                    {
                        //
                        // More than one operation option was found
                        //
                        throw UnexpectedOptionException (OPTION_ENABLE);
                    }

                    if (options.isSet(OPTION_ENABLE) > 1)
                    {
                        //
                        // More than one enable option was found
                        //
                        throw DuplicateOptionException (OPTION_ENABLE);
                    }

                    _operationType = OPERATION_TYPE_ENABLE;

                    break;
                }

                case OPTION_LIST:
                {
                    if (_operationType != OPERATION_TYPE_UNINITIALIZED)
                    {
                        //
                        // More than one operation option was found
                        //
                        throw UnexpectedOptionException(OPTION_LIST);
                    }

                    if (options.isSet(OPTION_LIST) > 1)
                    {
                        //
                        // More than one list option was found
                        //
                        throw DuplicateOptionException (OPTION_LIST);
                    }
                    _operationType = OPERATION_TYPE_LIST;
                    _operationArg = options[i].Value();
                    break;
                }

                case OPTION_REMOVE:
                {
                    if (_operationType != OPERATION_TYPE_UNINITIALIZED)
                    {
                        //
                        // More than one operation option was found
                        //
                        throw UnexpectedOptionException(OPTION_REMOVE);
                    }
                    if (options.isSet(OPTION_REMOVE) > 1)
                    {
                        //
                        // More than one remove option was found
                        //
                        throw DuplicateOptionException(OPTION_REMOVE);
                    }
                    _operationType = OPERATION_TYPE_REMOVE;
                    _operationArg = options[i].Value();
                    break;
                }

                case OPTION_VERBOSE:
                {
                  if (_operationType != OPERATION_TYPE_LIST)
                    {
                        //
                        // Unexpected verbose option was found
                        //
                        throw UnexpectedOptionException(OPTION_VERBOSE);
                    }
                    if (options.isSet(OPTION_VERBOSE) > 1)
                    {
                        //
                        // More than one verbose option was found
                        //
                        throw DuplicateOptionException(OPTION_VERBOSE);
                    }

                    _verbose = true;

                    break;
                }

                case OPTION_FILTER:
                {
                    if ((_operationType == OPERATION_TYPE_HELP) ||
                        (_operationType == OPERATION_TYPE_VERSION))
                    {
                        //
                        // Help and version take no options.
                        //
                        throw UnexpectedOptionException(OPTION_FILTER);
                    }
                    if (options.isSet(OPTION_FILTER) > 1)
                    {
                        //
                        // More than one filter option was found
                        //
                        throw DuplicateOptionException(OPTION_FILTER);
                    }
                    filterNameString = options[i].Value();
                    filterSet = true;
                    break;
                }

                case OPTION_HANDLER:
                {
                    if ((_operationType == OPERATION_TYPE_HELP) ||
                        (_operationType == OPERATION_TYPE_VERSION))
                    {
                        //
                        // Help and version take no options.
                        //
                        throw UnexpectedOptionException(OPTION_HANDLER);
                    }
                    if (options.isSet(OPTION_HANDLER) > 1)
                    {
                        //
                        // More than one handler option was found
                        //
                        throw DuplicateOptionException(OPTION_HANDLER);
                    }

                    handlerNameString = options[i].Value();
                    handlerSet = true;
                    break;
                }

                case OPTION_NAMESPACE:
                {
                    if ((_operationType == OPERATION_TYPE_HELP) ||
                        (_operationType == OPERATION_TYPE_VERSION))
                    {
                        //
                        // Help and version take no options.
                        //
                        throw UnexpectedOptionException(OPTION_NAMESPACE);
                    }
                    if (options.isSet(OPTION_NAMESPACE) > 1)
                    {
                        //
                        // More than one namespace option was found
                        //
                        throw DuplicateOptionException(OPTION_NAMESPACE);
                    }

                    String nsNameValue = options[i].Value();
                    _subscriptionNamespace = nsNameValue;

                    break;
                }
                default:
                    PEGASUS_ASSERT(0);
                }
            }
        }

    //
    // Some more validations
    //
    if (_operationType == OPERATION_TYPE_UNINITIALIZED)
    {
        //
        // No operation type was specified
        // Show the usage
        //
        throw CommandFormatException(localizeMessage(
            MSG_PATH, REQUIRED_OPTION_MISSING_KEY, REQUIRED_OPTION_MISSING));
    }

    if (_operationType == OPERATION_TYPE_LIST)
    {
        if (_operationArg == ARG_FILTERS)
        {
            if (handlerSet)
            {
                //
                // Wrong option for this operation
                // was found
                //
                throw UnexpectedOptionException(OPTION_HANDLER);
            }
        }
        else if (_operationArg == ARG_HANDLERS)
        {
            if (filterSet)
            {
                //
                // Wrong option for this operation was found
                //
                throw UnexpectedOptionException
                    (OPTION_FILTER);
            }
        }
        else if (_operationArg != ARG_SUBSCRIPTIONS)
        {
            //
            // A wrong option argument for this
            // operation was found
            //
            throw InvalidOptionArgumentException(
                _operationArg, OPTION_LIST);
        }
    }

    if (_operationType == OPERATION_TYPE_DISABLE)
    {
        if (!filterSet)
        {
            throw MissingOptionException(OPTION_FILTER);
        }

        if (!handlerSet)
        {
            throw MissingOptionException(OPTION_HANDLER);
        }
    }

    if (_operationType == OPERATION_TYPE_ENABLE)
    {
        if (!filterSet)
        {
            throw MissingOptionException(OPTION_FILTER);
        }

        if (!handlerSet)
        {
            throw MissingOptionException(OPTION_HANDLER);
        }
    }

    if (_operationType == OPERATION_TYPE_REMOVE)
    {
        if (_operationArg == ARG_FILTERS)
        {
            if (handlerSet)
            {
                //
                // Wrong option for this
                // operation was found
                //
                throw UnexpectedOptionException(
                    OPTION_HANDLER);
            }
        }
        else
        {
            if (_operationArg == ARG_HANDLERS)
            {
                if (filterSet)
                {
                    //
                    // Wrong option for this operation was found
                    //
                    throw UnexpectedOptionException(OPTION_FILTER);
                }
            }
            else
            {
                if ((_operationArg != ARG_SUBSCRIPTIONS) &&
                    (_operationArg != ARG_ALL))
                {
                    //
                    // A wrong option argument for this operation
                    // was found
                    //
                    throw InvalidOptionArgumentException
                        (_operationArg, OPTION_REMOVE);
                }
            }
        }
        if ((_operationArg == ARG_SUBSCRIPTIONS) ||
            (_operationArg == ARG_ALL) ||
            (_operationArg == ARG_FILTERS))
        {
          if (!filterSet)
            {
                throw MissingOptionException(OPTION_FILTER);
            }
        }
        if ((_operationArg == ARG_SUBSCRIPTIONS) ||
            (_operationArg == ARG_ALL) ||
            (_operationArg == ARG_HANDLERS))
        {
            if (!handlerSet)
            {
                throw MissingOptionException(OPTION_HANDLER);
            }
        }
    }
    if (filterSet)
    {
        _parseFilterName(filterNameString, _filterName, _filterNamespace);
    }
    if (handlerSet)
    {
        _parseHandlerName(handlerNameString, _handlerName, _handlerNamespace,
            _handlerCreationClass);
    }
}

/**
    Executes the command and writes the results to the PrintWriters.
*/
Uint32 CIMSubCommand::execute(
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    Array<CIMNamespaceName> namespaceNames;
    //
    // The CIM Client reference
    //

    if (_operationType == OPERATION_TYPE_UNINITIALIZED)
    {
        //
        // The command was not initialized
        //
        return 1;
    }
    else if (_operationType == OPERATION_TYPE_HELP)
    {
        errPrintWriter << usage << endl;
        return (RC_SUCCESS);
    }
    else if (_operationType == OPERATION_TYPE_VERSION)
    {
         errPrintWriter << "Version " << PEGASUS_PRODUCT_VERSION << endl;
        return (RC_SUCCESS);
    }

    try
    {
        // Construct the CIMClient and set to request server messages
        // in the default language of this client process.
        _client.reset(new CIMClient);
        _client->setRequestDefaultLanguages();
    }
    catch (Exception & e)
    {
        errPrintWriter << e.getMessage() << endl;
        return (RC_ERROR);
    }

    try
    {
        //
        // Open connection with CIMSever
        //
        _client->connectLocal();

    }
    catch (const Exception&)
    {
        errPrintWriter << localizeMessage(MSG_PATH,
            CIMOM_NOT_RUNNING_KEY,
            CIMOM_NOT_RUNNING) << endl;
        return (RC_CONNECTION_FAILED);
    }
    //
    // Perform the requested operation
    //
    try
    {
        CIMNamespaceName subscriptionNS;
        CIMNamespaceName filterNS = CIMNamespaceName();
        CIMNamespaceName handlerNS = CIMNamespaceName();
        if (_subscriptionNamespace != String::EMPTY)
        {
            subscriptionNS = _subscriptionNamespace;
        }

        if (_filterNamespace != String::EMPTY)
        {
            filterNS = _filterNamespace;
        }

        if (_handlerNamespace != String::EMPTY)
        {
            handlerNS = _handlerNamespace;
        }

        switch (_operationType)
        {
            case OPERATION_TYPE_ENABLE:
                return(_findAndModifyState(STATE_ENABLED,
                    subscriptionNS, _filterName, filterNS,
                    _handlerName, handlerNS, _handlerCreationClass,
                    outPrintWriter));

            case OPERATION_TYPE_DISABLE:
                return (_findAndModifyState(STATE_DISABLED,
                    subscriptionNS, _filterName, filterNS,
                    _handlerName, handlerNS, _handlerCreationClass,
                    outPrintWriter));

            case OPERATION_TYPE_LIST:
                if (_operationArg == ARG_SUBSCRIPTIONS)
                {
                    if (subscriptionNS.isNull())
                    {
                        _getAllNamespaces(namespaceNames);
                    }
                    else
                    {
                        namespaceNames.append(subscriptionNS);
                    }
                    _listSubscriptions(namespaceNames, _filterName,
                        filterNS, _handlerName, handlerNS,
                        _handlerCreationClass, _verbose, outPrintWriter,
                        errPrintWriter);
                }
                else if (_operationArg == ARG_FILTERS)
                {
                    if (filterNS.isNull())
                    {
                        _getAllNamespaces(namespaceNames);
                    }
                    else
                    {
                        namespaceNames.append(filterNS);
                    }
                    _listFilters(_filterName, _verbose,
                        namespaceNames, outPrintWriter,
                        errPrintWriter);
                }
                else if (_operationArg == ARG_HANDLERS)
                {
                     if (handlerNS.isNull())
                     {
                          _getAllNamespaces(namespaceNames);
                     }
                     else
                     {
                          namespaceNames.append(handlerNS);
                     }
                          _listHandlers(_handlerName, namespaceNames,
                               _handlerCreationClass, _verbose,
                               outPrintWriter, errPrintWriter);
                }
            break;

        case OPERATION_TYPE_REMOVE:
            if ((_operationArg == ARG_SUBSCRIPTIONS) || (_operationArg ==
                ARG_ALL))
            {
                Boolean removeAll = false;
                if (_operationArg == ARG_ALL)
                {
                    removeAll = true;
                }
                return _removeSubscription(subscriptionNS,
                    _filterName, filterNS, _handlerName, handlerNS,
                    _handlerCreationClass, removeAll, outPrintWriter,
                    errPrintWriter);
            }
            else if (_operationArg == ARG_FILTERS)
            {
                return (_removeFilter(_filterName, filterNS,
                    outPrintWriter, errPrintWriter));
            }
            else
            {
                PEGASUS_ASSERT (_operationArg == ARG_HANDLERS);
                return _removeHandler(_handlerName,
                    handlerNS, _handlerCreationClass, outPrintWriter,
                    errPrintWriter);
            }
            break;

        default:
            PEGASUS_ASSERT(0);
            break;
        }
    }

    catch (CIMException& e)
    {
        CIMStatusCode code = e.getCode();
        if (code == CIM_ERR_NOT_FOUND)
        {
            errPrintWriter << e.getMessage() << endl;
            return RC_OBJECT_NOT_FOUND;
        }
        else if (code == CIM_ERR_INVALID_NAMESPACE)
        {
            errPrintWriter << e.getMessage() << endl;
            return RC_NAMESPACE_NONEXISTENT;
        }
        else if (code == CIM_ERR_NOT_SUPPORTED)
        {
            errPrintWriter << e.getMessage() << endl;
            return RC_OPERATION_NOT_SUPPORTED;
        }
        else if (code == CIM_ERR_ACCESS_DENIED)
        {
            errPrintWriter << e.getMessage() << endl;
            return RC_ACCESS_DENIED;
        }
        else
        {
            errPrintWriter << e.getMessage() << endl;
        }
        return (RC_ERROR);
    }
    catch (ConnectionTimeoutException& e)
    {
        errPrintWriter << e.getMessage() << endl;
        return (RC_CONNECTION_TIMEOUT);
    }
    catch (Exception& e)
    {
        errPrintWriter << e.getMessage() << endl;
        return (RC_ERROR);
    }
    return (RC_SUCCESS);
}

//
// parse the filter option string
//
void CIMSubCommand::_parseFilterName(
    const String& filterNameString,
    String& filterName,
    String& filterNamespace)
{
    Uint32 nsDelimiterIndex = filterNameString.find(
        DELIMITER_NAMESPACE);
    if (nsDelimiterIndex == PEG_NOT_FOUND)
    {
        filterName = filterNameString;
        filterNamespace.clear();
    }
    else
    {
        if((nsDelimiterIndex == 0 ) ||
            ((nsDelimiterIndex + 1) ==
            filterNameString.size()))
        {
            // Invalid - either no name or no class
            throw InvalidOptionArgumentException(
                filterNameString, OPTION_FILTER);
        }
        // Parse the filter namespace and filter name
        filterNamespace = filterNameString.subString(0,
            nsDelimiterIndex);
        filterName = filterNameString.subString(
            nsDelimiterIndex+1);
    }
}

//
// parse the handler option string
//
void CIMSubCommand::_parseHandlerName(
    const String& handlerString,
    String& handlerName,
    String& handlerNamespace,
    String& handlerCreationClass)
{
    Uint32 nsDelimiterIndex = handlerString.find (
        DELIMITER_NAMESPACE);
    if (nsDelimiterIndex == PEG_NOT_FOUND)
    {
        //
        // handler namespace was not found
        //
        handlerNamespace.clear();
        //
        // Check for handler class
        //
        Uint32 classDelimiterIndex = handlerString.find (
        DELIMITER_HANDLER_CLASS);
        if (classDelimiterIndex == PEG_NOT_FOUND)
        {
            handlerName = handlerString;
        }
        else
        {
            //
            // Parse creation class and handler name
            //
            if ((classDelimiterIndex == 0) ||
                ((classDelimiterIndex + 1) ==
                handlerString.size()))
            {
                // Invalid - either no name or no class
                throw InvalidOptionArgumentException(
                    handlerString, OPTION_HANDLER);
            }
            handlerCreationClass =
                handlerString.subString (0,
                classDelimiterIndex);
            handlerName = handlerString.subString(
                classDelimiterIndex+1);
        }
    }
    else
    {
        //
        // handler namespace was found
        //

        // Parse the handler namespace and handler name
        handlerNamespace = handlerString.subString(0,
            nsDelimiterIndex);
        if ((nsDelimiterIndex == 0) ||
            ((nsDelimiterIndex + 1) ==
            handlerString.size()))
        {
            // Invalid - either no name or no class
            throw InvalidOptionArgumentException(
                handlerString, OPTION_HANDLER);
        }
        Uint32 classDelimiterIndex = handlerString.find (
            DELIMITER_HANDLER_CLASS);
        if (classDelimiterIndex == PEG_NOT_FOUND)
        {

            // No creation class specified, just the handler name

            handlerName = handlerString.subString(nsDelimiterIndex+1);
        }
        else
        {
            if ((nsDelimiterIndex + 1 ) == classDelimiterIndex)
            {
                // Invalid - no class
                throw InvalidOptionArgumentException(
                    handlerString, OPTION_HANDLER);
            }

            if ((classDelimiterIndex + 1) ==
                handlerString.size())
            {
                // Invalid - no name
                throw InvalidOptionArgumentException(
                    handlerString, OPTION_HANDLER);
            }

            // Parse the handler class and name

            Uint32 slen = classDelimiterIndex - nsDelimiterIndex - 1;
            handlerCreationClass =
                handlerString.subString(nsDelimiterIndex+1, slen);
            handlerName = handlerString.subString(classDelimiterIndex+1);
        }
    }
}

//
// remove an existing subscription instance
//
Uint32 CIMSubCommand::_removeSubscription(
    const CIMNamespaceName& subscriptionNamespace,
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    const Boolean removeAll,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    CIMObjectPath subPathFound;
    CIMNamespaceName filterNS;
    CIMNamespaceName handlerNS;
    CIMNamespaceName subscriptionNS = _DEFAULT_SUBSCRIPTION_NAMESPACE;

    if (!subscriptionNamespace.isNull())
    {
        subscriptionNS = subscriptionNamespace;
    }

    if (!filterNamespace.isNull())
    {
        filterNS = filterNamespace;
    }
    else
    {
        filterNS = subscriptionNS;
    }

    if (!handlerNamespace.isNull())
    {
        handlerNS = handlerNamespace;
    }
    else
    {
        handlerNS = subscriptionNS;
    }

    if (_findSubscription(subscriptionNS, filterName, filterNS,
        handlerName, handlerNS, handlerCreationClass, subPathFound))
    {
        if (!removeAll)
        {
            _client->deleteInstance(subscriptionNS, subPathFound);
        }
        else
        {
            // Delete subscription, filter and handler
            CIMObjectPath filterRef, handlerRef;
            //
            //  Get the subscription Filter and Handler ObjectPaths
            //
            Array<CIMKeyBinding> keys = subPathFound.getKeyBindings();
            for( Uint32 j=0; j < keys.size(); j++)
            {
                if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_FILTER))
                {
                    filterRef = keys[j].getValue();
                }
                if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_HANDLER))
                {
                    handlerRef = keys[j].getValue();
                }
            }
            _client->deleteInstance(subscriptionNS, subPathFound);
            _client->deleteInstance(filterNS, filterRef);
            _client->deleteInstance(handlerNS, handlerRef);
        }
        return (RC_SUCCESS);
    }
    else
    {
        outPrintWriter << localizeMessage(MSG_PATH,
            SUBSCRIPTION_NOT_FOUND_KEY,
            SUBSCRIPTION_NOT_FOUND_FAILURE) << endl;
        return (RC_OBJECT_NOT_FOUND);
    }
}

//
//  remove an existing filter instance
//
Uint32 CIMSubCommand::_removeFilter
(
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    ostream& outPrintWriter,
    ostream& errPrintWriter
)
{
    CIMObjectPath filterPath;
    CIMNamespaceName filterNS = _DEFAULT_SUBSCRIPTION_NAMESPACE;
    if (!filterNamespace.isNull())
    {
        filterNS = filterNamespace;
    }

    if (_findFilter(filterName, filterNS, errPrintWriter, filterPath))
    {
        _client->deleteInstance(filterNS, filterPath);
        return (RC_SUCCESS);
    }
    else
    {
        outPrintWriter << localizeMessage(MSG_PATH,
            FILTER_NOT_FOUND_KEY,
            FILTER_NOT_FOUND_FAILURE) << endl;
        return (RC_OBJECT_NOT_FOUND);
    }
}

//
//  find a filter
//
Boolean CIMSubCommand::_findFilter(
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    ostream& errPrintWriter,
    CIMObjectPath& filterPath)
{
    Array<CIMObjectPath> filterPaths;
    Boolean status = false;

    try
    {
        filterPaths = _client->enumerateInstanceNames(
            filterNamespace,
            PEGASUS_CLASSNAME_INDFILTER);
    }
    catch (CIMException& e)
    {
        if (e.getCode() == CIM_ERR_INVALID_CLASS)
        {
            return false;
        }
        else
        {
            throw;
        }
    }

    Uint32 filterCount = filterPaths.size();
    if (filterCount > 0)
    {

        // find matching indication filter
        for (Uint32 i = 0; i < filterCount; i++)
        {
            CIMObjectPath fPath = filterPaths[i];
            Array<CIMKeyBinding> keys = fPath.getKeyBindings();
            for(Uint32 j=0; j < keys.size(); j++)
            {
                String filterNameValue;
                if(keys[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
                {
                    filterNameValue = keys[j].getValue();
                }
                if (filterNameValue == filterName)
                {
                    status = true;
                    filterPath = fPath;
                    break;
                }
          }
        }
    }
    return status;
}

////  remove an existing handler instance
//
Uint32 CIMSubCommand::_removeHandler(
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    CIMObjectPath handlerPath;
    CIMNamespaceName handlerNS = _DEFAULT_SUBSCRIPTION_NAMESPACE;
    if (!handlerNamespace.isNull())
    {
        handlerNS = handlerNamespace;
    }

    if (_findHandler(handlerName, handlerNS, handlerCreationClass,
        errPrintWriter, handlerPath))
    {
        _client->deleteInstance(handlerNS, handlerPath);
        return (RC_SUCCESS);
    }
    else
    {
        outPrintWriter << localizeMessage(MSG_PATH,
            HANDLER_NOT_FOUND_KEY,
            HANDLER_NOT_FOUND_FAILURE) << endl;
        return RC_OBJECT_NOT_FOUND;
    }
}

//
//  find a matching handler
//
Boolean CIMSubCommand::_findHandler(
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    ostream& errPrintWriter,
    CIMObjectPath& handlerPath)
{
    Array<CIMObjectPath> handlerPaths;
    String handlerCC = PEGASUS_CLASSNAME_LSTNRDST_CIMXML.getString();
    Boolean status = false;
    if (handlerCreationClass != String::EMPTY)
    {
        handlerCC = handlerCreationClass;
    }
    try
    {
        handlerPaths = _client->enumerateInstanceNames(
            handlerNamespace,
            handlerCC);
    }
    catch (CIMException& e)
    {
        if (e.getCode() == CIM_ERR_INVALID_CLASS)
        {
            return false;
        }
        else
        {
            throw;
        }
    }

    Uint32 handlerCount = handlerPaths.size();
    if (handlerCount > 0)
    {
        // find matching indication handler
        for (Uint32 i = 0; i < handlerCount; i++)
        {
            Boolean nameFound = false;
            CIMObjectPath hPath = handlerPaths[i];
            Array<CIMKeyBinding> keys = hPath.getKeyBindings();
            for( Uint32 j=0; j < keys.size(); j++)
            {
                if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
                {
                    String handlerNameValue = keys[j].getValue();
                    if (handlerNameValue == handlerName )
                    {
                        nameFound = true;
                        break;
                    }
                }
            }
            if (nameFound)
            {
                status = true;
                handlerPath = hPath;
                break;
            }
        }
    }
    return status;
}

//
//  Modify a subscription state
//
void CIMSubCommand::_modifySubscriptionState(
    const CIMNamespaceName& subscriptionNS,
    const CIMObjectPath& targetPath,
    const Uint16 newState,
    ostream& outPrintWriter)
{
    Boolean alreadySet = false;
    CIMInstance targetInstance = _client->getInstance(subscriptionNS,
        targetPath);
    Uint32 pos = targetInstance.findProperty (
        PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE);
    if (pos != PEG_NOT_FOUND)
    {
        Uint16 subscriptionState;
        if (targetInstance.getProperty(pos).getValue().isNull())
        {
            subscriptionState = STATE_UNKNOWN;
        }
        else
        {
            targetInstance.getProperty(pos).getValue().get
                (subscriptionState);
            if (subscriptionState == newState)
            {
                if (newState == STATE_ENABLED )
                {
                    outPrintWriter << localizeMessage(MSG_PATH,
                        SUBSCRIPTION_ALREADY_ENABLED_KEY,
                        SUBSCRIPTION_ALREADY_ENABLED) << endl;
                }
                else
                {
                    outPrintWriter << localizeMessage(MSG_PATH,
                        SUBSCRIPTION_ALREADY_DISABLED_KEY,
                        SUBSCRIPTION_ALREADY_DISABLED) << endl;
                }
                alreadySet = true;
            }
        }
        if (!alreadySet)
        {
            targetInstance.getProperty(pos).setValue(newState);
            Array <CIMName> propertyNames;
            propertyNames.append(PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE);
            CIMPropertyList properties(propertyNames);
            targetInstance.setPath(targetPath);
            _client->modifyInstance(subscriptionNS, targetInstance, false,
                properties);
        }
    }
}

//
//  find a subscription
//
Boolean CIMSubCommand::_findSubscription(
    const CIMNamespaceName& subscriptionNamespace,
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCC,
    CIMObjectPath& subscriptionFound)
{
    Array<CIMObjectPath> subscriptionPaths;
    String handlerCreationClass;
    if (handlerCC != String::EMPTY)
    {
        handlerCreationClass = handlerCC;
    }
    try
    {
        subscriptionPaths = _client->enumerateInstanceNames(
            subscriptionNamespace, PEGASUS_CLASSNAME_INDSUBSCRIPTION);
    }
    catch (CIMException& e)
    {
        if (e.getCode() == CIM_ERR_INVALID_CLASS)
        {
            return false;
        }
        else
        {
            throw;
        }
    }

    Uint32 subscriptionCount = subscriptionPaths.size();
    if (subscriptionCount > 0)
    {
        String handlerNameString, filterNameString;
        CIMNamespaceName handlerNS, filterNS;

        // Search the indication subscriptions instances
        for (Uint32 i = 0; i < subscriptionCount; i++)
        {
            CIMObjectPath subPath = subscriptionPaths[i];
            CIMObjectPath filterRef;
            if (_filterMatches(subPath, subscriptionNamespace,
                filterName, filterNamespace, filterNS, filterRef))
            {
                CIMObjectPath handlerRef;
                if(_handlerMatches(subPath, subscriptionNamespace,
                    handlerName, handlerNamespace, handlerCreationClass,
                    handlerNS, handlerRef))
                {
                    subscriptionFound = subPath;
                    return true;
                }
            }
        }
    }
    return false;
}

//
// Find a subscription and modify it's state
//
Uint32 CIMSubCommand::_findAndModifyState(
    const Uint16 newState,
    const CIMNamespaceName& subscriptionNamespace,
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    ostream& outPrintWriter)
{
    CIMObjectPath subscriptionFound;
    CIMNamespaceName filterNS;
    CIMNamespaceName handlerNS;
    CIMNamespaceName subscriptionNS = _DEFAULT_SUBSCRIPTION_NAMESPACE;

    if (!subscriptionNamespace.isNull())
    {
        subscriptionNS = subscriptionNamespace;
    }

    if (!filterNamespace.isNull())
    {
        filterNS = filterNamespace;
    }
    else
    {
        filterNS = subscriptionNS;
    }

    if (!handlerNamespace.isNull())
    {
        handlerNS = handlerNamespace;
    }
    else
    {
        handlerNS = subscriptionNS;
    }

    // Find subscriptions in the namespace specified by the user
    if (_findSubscription(subscriptionNS, filterName, filterNS,
        handlerName, handlerNS, handlerCreationClass, subscriptionFound))
    {
        _modifySubscriptionState(subscriptionNS, subscriptionFound,
            newState, outPrintWriter);
        return(RC_SUCCESS);
    }
    else
    {
        outPrintWriter << localizeMessage(MSG_PATH,
            SUBSCRIPTION_NOT_FOUND_KEY,
            SUBSCRIPTION_NOT_FOUND_FAILURE) << endl;
        return(RC_OBJECT_NOT_FOUND);
    }
}

//
// Get the name from a CIMObjectPath
//
String CIMSubCommand::_getNameInKey(const CIMObjectPath& r)
{
    String nameValue;
    Array<CIMKeyBinding> keys = r.getKeyBindings();
    for (Uint32 j=0; j < keys.size(); j++)
    {
        if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
        {
            nameValue = keys[j].getValue();
        }
    }
    return (nameValue);
}

//
// Get all namespaces
//
//
void CIMSubCommand::_getAllNamespaces(
    Array<CIMNamespaceName>& namespaceNames)
{
    Array<CIMObjectPath> instanceNames = _client->enumerateInstanceNames(
        PEGASUS_VIRTUAL_TOPLEVEL_NAMESPACE,
        PEGASUS_CLASSNAME___NAMESPACE);

    // for all new elements in the output array
    for (Uint32 i = 0; i < instanceNames.size(); i++)
    {
        Array<CIMKeyBinding> keys = instanceNames[i].getKeyBindings();
        for (Uint32 j=0; j < keys.size(); j++)
        {
            if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
            {
                namespaceNames.append(keys[j].getValue());
            }
        }
    }
}

//
//  get a list of all Handlers in the specified namespace(s)
//
void CIMSubCommand::_listHandlers(
    const String& handlerName,
    const Array<CIMNamespaceName>& namespaceNames,
    const String& handlerCreationClass,
    const Boolean verbose,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    Array <Uint32> maxColumnWidth;
    Array <ListColumnEntry> listOutputTable;
    Array<String> handlersFound;
    Array<String> destinationsFound;
    Array<CIMInstance> instancesFound;
    Array<Uint32> handlerTypesFound;
    const String handlerTitle = "HANDLER";
    const String destinationTitle = "DESTINATION";
    if (!verbose)
    {
        handlersFound.append(handlerTitle);
        maxColumnWidth.append(handlerTitle.size());
        destinationsFound.append(destinationTitle);
        maxColumnWidth.append(destinationTitle.size());
    }
    listOutputTable.append(handlersFound);
    listOutputTable.append(destinationsFound);
    //
    //  Find handlers in namespaces
    //
    for (Uint32 i = 0 ; i < namespaceNames.size() ; i++)
    {
        _getHandlerList(handlerName, namespaceNames[i],
            handlerCreationClass, verbose, instancesFound, handlerTypesFound,
            maxColumnWidth, listOutputTable, outPrintWriter,
            errPrintWriter);
    }
    if (verbose)
    {
        if (listOutputTable[_HANDLER_LIST_NAME_COLUMN].size() > 0 )
        {
            _printHandlersVerbose(instancesFound, handlerTypesFound,
                 listOutputTable, outPrintWriter);
        }
    }
    else
    {
        if (listOutputTable[_HANDLER_LIST_NAME_COLUMN].size() > 1 )
        {
            _printColumns(maxColumnWidth, listOutputTable, outPrintWriter);
        }
    }
}

//
//  get a list of all handlers in a specified namespace
//
void CIMSubCommand::_getHandlerList(
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    const Boolean verbose,
    Array<CIMInstance>& instancesFound,
    Array<Uint32>& handlerTypesFound,
    Array <Uint32>& maxColumnWidth,
    Array <ListColumnEntry>& listOutputTable,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    Array<CIMObjectPath> handlerPaths;
    try
    {
        handlerPaths = _client->enumerateInstanceNames(
            handlerNamespace,
            PEGASUS_CLASSNAME_LSTNRDST);
    }
    catch(CIMException& e)
    {
        if (e.getCode() == CIM_ERR_INVALID_CLASS)
        {
            return;
        }
        else
        {
            throw;
        }
    }

    Uint32 handlerCount = handlerPaths.size();
    if (handlerCount > 0)
    {
        String handlerNameValue;
        String destination;
        String creationClassValue;

        // List all the indication handlers
        for (Uint32 i = 0; i < handlerCount; i++)
        {
            Boolean isMatch = true;
            CIMObjectPath handlerPath;
            CIMObjectPath hPath = handlerPaths[i];
            Array<CIMKeyBinding> keys = hPath.getKeyBindings();
            for(Uint32 j=0; j < keys.size(); j++)
            {
                if(keys[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
                {
                    handlerNameValue = keys[j].getValue();
                }
                if(keys[j].getName().equal(
                            PEGASUS_PROPERTYNAME_CREATIONCLASSNAME))
                {
                    creationClassValue = keys[j].getValue();
                }
            }
            if (handlerName != String::EMPTY)
            {
                if (handlerNameValue == handlerName)
                {
                    if (handlerCreationClass != String::EMPTY)
                    {
                        if (handlerCreationClass !=
                            creationClassValue)
                        {
                            isMatch = false;
                        }
                    }
                }
                else
                {
                    isMatch = false;
                }
            }
            if (isMatch)
            {
                handlerPath = hPath;
                CIMInstance handlerInstance = _client->getInstance(
                    handlerNamespace, handlerPath);
                Uint32 handlerType = _HANDLER_CIMXML;
                _getHandlerDestination(handlerInstance, creationClassValue,
                    handlerType, destination);
                String handlerString = handlerNamespace.getString();
                handlerString.append(DELIMITER_NAMESPACE);
                handlerString.append(creationClassValue);
                handlerString.append(DELIMITER_HANDLER_CLASS);
                handlerString.append(handlerNameValue);
                listOutputTable[_HANDLER_LIST_NAME_COLUMN].append(
                        handlerString);
                listOutputTable[_HANDLER_LIST_DESTINATION_COLUMN].append(
                        destination);
                handlerTypesFound.append(handlerType);
                if (verbose)
                {
                    instancesFound.append(handlerInstance);
                }
                else
                {
                    if (handlerString.size() >
                        maxColumnWidth[_HANDLER_LIST_NAME_COLUMN])
                    {
                        maxColumnWidth[_HANDLER_LIST_NAME_COLUMN] =
                            handlerString.size();
                    }
                    if (destination.size() >
                        maxColumnWidth[_HANDLER_LIST_DESTINATION_COLUMN])
                    {
                        maxColumnWidth[_HANDLER_LIST_DESTINATION_COLUMN] =
                            destination.size();
                    }
                }
            }
        }
    }
}

//
//  print a verbose list of Handlers
//
void CIMSubCommand::_printHandlersVerbose(
    const Array<CIMInstance>& instancesFound,
    const Array<Uint32>& handlerTypesFound,
    const Array <ListColumnEntry>& listOutputTable,
    ostream& outPrintWriter)
{
    Uint32 maxEntries = listOutputTable[_HANDLER_LIST_NAME_COLUMN].size();
    Array <Uint32> indexes;
    for (Uint32 i = 0; i < maxEntries; i++)
    {
       indexes.append (i);
    }
    _bubbleIndexSort (listOutputTable[_HANDLER_LIST_NAME_COLUMN], 0, indexes);
    for (Uint32 i = 0; i < maxEntries; i++)
    {
        Uint32 pos;
        CIMInstance handlerInstance = instancesFound[indexes[i]];
        outPrintWriter << "Handler:           " <<
           (listOutputTable[_HANDLER_LIST_NAME_COLUMN])[indexes[i]] << endl;
        switch (handlerTypesFound[indexes[i]])
        {
            case _HANDLER_SNMP:
            {
                String targetHost;
                pos = handlerInstance.findProperty(
                    PEGASUS_PROPERTYNAME_LSTNRDST_TARGETHOST);
                if (pos != PEG_NOT_FOUND)
                {
                    handlerInstance.getProperty(pos).getValue().get
                        (targetHost);
                }
                outPrintWriter << "TargetHost:        " << targetHost
                    << endl;
                outPrintWriter << "SNMPVersion:       " <<
                    _getSnmpVersion(handlerInstance) << endl;
                break;
            }

            case _HANDLER_EMAIL:
            {
                String mailCc;
                String mailTo;
                String mailSubject;
                _getEmailInfo(handlerInstance, mailCc,
                    mailTo, mailSubject );
                outPrintWriter << "MailTo:            " <<
                    mailTo << endl;
                if (mailCc.size() > 0 )
                {
                    outPrintWriter << "MailCc:            " <<
                        mailCc << endl;
                }
                if (mailSubject.size() > 0 )
                {
                    outPrintWriter << "MailSubject:       " <<
                        mailSubject << endl;
                }
                break;
            }

            case _HANDLER_SYSLOG:
            {
                break;
            }

            case _HANDLER_CIMXML:
            {
                outPrintWriter << "Destination:       " <<
                    (listOutputTable[_HANDLER_LIST_DESTINATION_COLUMN])
                        [indexes[i]]
                    << endl;
            }
        }
        outPrintWriter << "PersistenceType:   " <<
            _getPersistenceType(handlerInstance) << endl;
        outPrintWriter << "-----------------------------------------"
            << endl;
    }
}

//
//  get a list of all filters in the specified namespace(s)
//
void CIMSubCommand::_listFilters(
    const String& filterName,
    const Boolean verbose,
    const Array<CIMNamespaceName>& namespaceNames,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    Array <Uint32> maxColumnWidth;
    Array <ListColumnEntry> listOutputTable;
    Array<String> filtersFound;
    Array<String> querysFound;
    Array<String> queryLangsFound;
    const String filterTitle = "FILTER";
    const String queryTitle = "QUERY";
    if (!verbose)
    {
        filtersFound.append(filterTitle);
        maxColumnWidth.append(filterTitle.size());
        querysFound.append(queryTitle);
        maxColumnWidth.append(queryTitle.size());
    }
    listOutputTable.append(filtersFound);
    listOutputTable.append(querysFound);

    //  Find filters in namespaces
    for (Uint32 i = 0 ; i < namespaceNames.size(); i++)
    {
        _getFilterList(filterName, namespaceNames[i], verbose,
            maxColumnWidth, listOutputTable, queryLangsFound, outPrintWriter,
            errPrintWriter);
    }
    if (verbose)
    {
        if (listOutputTable[_FILTER_LIST_NAME_COLUMN].size() > 0)
        {
           _printFiltersVerbose(listOutputTable, queryLangsFound,
               outPrintWriter);
        }
    }
    else
    {
        if (listOutputTable[_FILTER_LIST_NAME_COLUMN].size() > 1)
        {
            _printColumns(maxColumnWidth, listOutputTable, outPrintWriter);
        }
    }
}

//
//  get a list of all filters in the specified namespace(s)
//
void CIMSubCommand::_printFiltersVerbose(
    const Array <ListColumnEntry>& listOutputTable,
    const Array <String>& queryLangs,
    ostream& outPrintWriter)
{
    Uint32 maxEntries = listOutputTable[_FILTER_LIST_NAME_COLUMN].size();
    Array <Uint32> indexes;
    for (Uint32 i = 0; i < maxEntries; i++)
    {
       indexes.append(i);
    }
    _bubbleIndexSort (listOutputTable[_FILTER_LIST_NAME_COLUMN], 0, indexes);
    for (Uint32 i = 0; i < maxEntries; i++)
    {
        outPrintWriter << "Filter:           " <<
            (listOutputTable[_FILTER_LIST_NAME_COLUMN])[indexes[i]] << endl;
        outPrintWriter << "Query:            " <<
            (listOutputTable[_FILTER_LIST_QUERY_COLUMN])[indexes[i]] << endl;
        outPrintWriter << "Query Language:   " <<
            queryLangs[indexes[i]] << endl;
        outPrintWriter <<
            "-----------------------------------------" << endl;
    }
}

//
//  get a list of all filters in the specified namespace
//
void CIMSubCommand::_getFilterList(
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    const Boolean verbose,
    Array <Uint32>& maxColumnWidth,
    Array <ListColumnEntry>& listOutputTable,
    Array <String>& queryLangsFound,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    Array<CIMObjectPath> filterPaths;
    try
    {
        filterPaths = _client->enumerateInstanceNames(
            filterNamespace,
            PEGASUS_CLASSNAME_INDFILTER);
    }
    catch(CIMException& e)
    {
        if (e.getCode() == CIM_ERR_INVALID_CLASS)
        {
            return;
        }
        else
        {
            throw;
        }
    }

    Uint32 filterCount = filterPaths.size();
    if (filterCount > 0)
    {
        CIMObjectPath filterPath;
        String filterNameValue;

        // List all the indication filters
        for (Uint32 i = 0; i < filterCount; i++)
        {
            Boolean isMatch = true;
            CIMObjectPath fPath = filterPaths[i];
            Array<CIMKeyBinding> keys = fPath.getKeyBindings();
            for(Uint32 j=0; j < keys.size(); j++)
            {
                if(keys[j].getName().equal(PEGASUS_PROPERTYNAME_NAME))
                {
                    filterNameValue = keys[j].getValue();
                    filterPath = fPath;
                    if (filterName != String::EMPTY)
                    {
                        if (filterNameValue == filterName)
                        {
                            break;
                        }
                        else
                        {
                            isMatch = false;
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (isMatch)
            {
                String queryString,queryLanguageString;
                String filterString = filterNamespace.getString();
                filterString.append(DELIMITER_NAMESPACE);
                filterString.append(filterNameValue);
                _getQueryString(filterNamespace,
                    filterPath, queryString, queryLanguageString);
                listOutputTable[_FILTER_LIST_NAME_COLUMN].append(filterString);
                listOutputTable[_FILTER_LIST_QUERY_COLUMN].append(queryString);
                if (verbose)
                {
                    queryLangsFound.append(queryLanguageString);
                }
                else
                {
                    if (filterString.size () >
                        maxColumnWidth[_FILTER_LIST_NAME_COLUMN])
                    {
                        maxColumnWidth[_FILTER_LIST_NAME_COLUMN] =
                            filterString.size();
                    }
                    if (queryString.size() >
                        maxColumnWidth[_FILTER_LIST_QUERY_COLUMN])
                    {
                        maxColumnWidth[_FILTER_LIST_QUERY_COLUMN] =
                            queryString.size();
                    }
                }
           }
        }
    }
}

//
//  get the query string for a filter
//
void CIMSubCommand::_getQueryString(
    const CIMNamespaceName& filterNamespace,
    const CIMObjectPath& filterPath,
    String& queryString,
    String& queryLangString)
{
    String query;
    queryString = "\"";
    CIMInstance filterInstance = _client->getInstance(
        filterNamespace, filterPath);
    Uint32 pos = filterInstance.findProperty(
        PEGASUS_PROPERTYNAME_QUERY);
    if (pos != PEG_NOT_FOUND)
    {
        filterInstance.getProperty(pos).getValue().get(query);
        queryString.append(query);
    }
    queryString.append("\"");
    pos = filterInstance.findProperty(
        PEGASUS_PROPERTYNAME_QUERYLANGUAGE);
    if (pos != PEG_NOT_FOUND)
    {
        filterInstance.getProperty(pos).getValue().get(queryLangString);
    }
}

//
//  list  all subscriptions is the specified namespace(s)
//
void CIMSubCommand::_listSubscriptions(
    const Array<CIMNamespaceName>& namespaceNames,
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    const Boolean verbose,
    ostream& outPrintWriter,
    ostream& errPrintWriter)
{
    Array <Uint32> maxColumnWidth;
    Array <ListColumnEntry> listOutputTable;
    Array<String> namespacesFound;
    Array<String> filtersFound;
    Array<String> handlersFound;
    Array<String> statesFound;
    Array<CIMInstance> handlerInstancesFound;
    Array<Uint32> handlerTypesFound;
    Array<String> querysFound;
    const String namespaceTitle = "NAMESPACE";
    const String filterTitle = "FILTER";
    const String handlerTitle = "HANDLER";
    const String stateTitle = "STATE";
    if (!verbose)
    {
        namespacesFound.append(namespaceTitle);
        maxColumnWidth.append(namespaceTitle.size());
        filtersFound.append(filterTitle);
        maxColumnWidth.append(filterTitle.size());
        handlersFound.append(handlerTitle);
        maxColumnWidth.append(handlerTitle.size());
        statesFound.append(stateTitle);
        maxColumnWidth.append(stateTitle.size());
    }

    listOutputTable.append(namespacesFound);
    listOutputTable.append(filtersFound);
    listOutputTable.append(handlersFound);
    listOutputTable.append(statesFound);

    for (Uint32 i = 0 ; i < namespaceNames.size() ; i++)
    {
        _getSubscriptionList(namespaceNames[i], filterName,
            filterNamespace, handlerName, handlerNamespace,
            handlerCreationClass, verbose, handlerInstancesFound,
            handlerTypesFound, querysFound, maxColumnWidth,
            listOutputTable);
    }

    if (verbose)
    {
        if (listOutputTable[_SUBSCRIPTION_LIST_NS_COLUMN].size() > 0)
        {
            _printSubscriptionsVerbose(handlerInstancesFound,
               handlerTypesFound, querysFound, listOutputTable,
               outPrintWriter);
        }
    }
    else
    {
        if (listOutputTable[_SUBSCRIPTION_LIST_NS_COLUMN].size() > 1)
        {
            _printColumns(maxColumnWidth, listOutputTable, outPrintWriter);
        }
    }
}

//
//  get a list of subscriptions in the specifed namespace
//
void CIMSubCommand::_getSubscriptionList(
    const CIMNamespaceName& subscriptionNSIn,
    const String& filterName,
    const CIMNamespaceName& filterNSIn,
    const String& handlerName,
    const CIMNamespaceName& handlerNSIn,
    const String& handlerCCIn,
    const Boolean verbose,
    Array<CIMInstance>& handlerInstancesFound,
    Array<Uint32>& handlerTypesFound,
    Array<String>& querysFound,
    Array <Uint32>& maxColumnWidth,
    Array <ListColumnEntry>& listOutputTable
)
{
    Array<CIMObjectPath> subscriptionPaths;
    String query;
    String destination;
    CIMNamespaceName filterNamespace;
    CIMNamespaceName handlerNamespace;
    CIMNamespaceName subscriptionNamespace = _DEFAULT_SUBSCRIPTION_NAMESPACE;
    String handlerCreationClass;
    if (!subscriptionNSIn.isNull())
    {
        subscriptionNamespace = subscriptionNSIn;
    }
    if (!filterNSIn.isNull())
    {
        filterNamespace = filterNSIn;
    }

    else
    {
        filterNamespace = subscriptionNamespace;
    }

    if (!handlerNSIn.isNull())
    {
        handlerNamespace = handlerNSIn;
    }
    else
    {
        handlerNamespace = subscriptionNamespace;
    }

    if (handlerCCIn != String::EMPTY)
    {
        handlerCreationClass = handlerCCIn;
    }

    try
    {
        subscriptionPaths = _client->enumerateInstanceNames(
            subscriptionNamespace,
            PEGASUS_CLASSNAME_INDSUBSCRIPTION);
    }
    catch(CIMException& e)
    {
        if (e.getCode() == CIM_ERR_INVALID_CLASS)
        {
            return;
        }
        else
        {
            throw;
        }
    }
    Uint32 subscriptionCount = subscriptionPaths.size();
    if (subscriptionCount > 0)
    {
        String handlerNameString, filterNameString;
        CIMObjectPath handlerPath, filterPath;
        CIMNamespaceName handlerNS, filterNS;
        // List all the indication subscriptions
        for (Uint32 i = 0; i < subscriptionCount; i++)
        {
            CIMObjectPath subPath = subscriptionPaths[i];
            CIMObjectPath filterRef, handlerRef;
            Boolean filterMatch = true;
            Boolean handlerMatch = true;
            String creationClassName;

            //
            //  Get the subscription Filter
            //
            filterMatch = _filterMatches(subPath, subscriptionNamespace,
                filterName, filterNamespace, filterNS, filterRef);
            if (filterMatch)
            {
                filterNameString = _getNameInKey(filterRef);
                handlerMatch = _handlerMatches(subPath, subscriptionNamespace,
                    handlerName, handlerNamespace, handlerCreationClass,
                    handlerNS, handlerRef);
                if (handlerMatch)
                {
                    handlerNameString = _getNameInKey(handlerRef);
                }
            }
            if ((filterMatch) && (handlerMatch))
            {
                // Get the destination for this handler.
                CIMInstance handlerInstance = _client->getInstance(
                    handlerNS, handlerRef);
                Uint32 handlerType;
                _getHandlerDestination(handlerInstance, creationClassName,
                    handlerType, destination);
                String handlerString = handlerNS.getString();
                handlerString.append(DELIMITER_NAMESPACE);
                handlerString.append(creationClassName);
                handlerString.append(DELIMITER_HANDLER_CLASS);
                handlerString.append(handlerNameString);
                String statusValue = _getSubscriptionState(
                    subscriptionNamespace, subPath);

                // Save for columnar listing
                listOutputTable[_SUBSCRIPTION_LIST_NS_COLUMN].append
                    (subscriptionNamespace.getString());
                String filterString = filterNS.getString();
                filterString.append(DELIMITER_NAMESPACE);
                filterString.append(filterNameString);
                listOutputTable[_SUBSCRIPTION_LIST_FILTER_COLUMN].append
                    (filterString);
                listOutputTable[_SUBSCRIPTION_LIST_HANDLER_COLUMN].append
                    (handlerString);
                listOutputTable[_SUBSCRIPTION_LIST_STATE_COLUMN].append
                    (statusValue);
                if (verbose)
                {
                    String queryString, queryLangString;
                    handlerInstancesFound.append (handlerInstance);
                    handlerTypesFound.append (handlerType);
                    _getQueryString(filterNS, filterRef, queryString,
                       queryLangString);
                    querysFound.append(queryString);
                }
                else
                {
                    if (subscriptionNamespace.getString().size() >
                        maxColumnWidth[_SUBSCRIPTION_LIST_NS_COLUMN])
                    {
                        maxColumnWidth[_SUBSCRIPTION_LIST_NS_COLUMN] =
                            subscriptionNamespace.getString().size();
                    }
                    if (filterString.size() >
                        maxColumnWidth[_SUBSCRIPTION_LIST_FILTER_COLUMN])
                    {
                        maxColumnWidth[_SUBSCRIPTION_LIST_FILTER_COLUMN] =
                            filterString.size();
                    }
                    if (handlerString.size() >
                        maxColumnWidth[_SUBSCRIPTION_LIST_HANDLER_COLUMN])
                    {
                        maxColumnWidth[_SUBSCRIPTION_LIST_HANDLER_COLUMN] =
                            handlerString.size();
                    }
                    if (statusValue.size() >
                        maxColumnWidth[_SUBSCRIPTION_LIST_STATE_COLUMN])
                    {
                        maxColumnWidth[_SUBSCRIPTION_LIST_STATE_COLUMN] =
                            statusValue.size();
                    }
                }
            }
        }
    }
}

//
//  get the handler destination and type
//
void CIMSubCommand::_getHandlerDestination(
    const CIMInstance& handlerInstance,
    String& creationClassName,
    Uint32&  handlerTypeFound,
    String& destination)
{
    Uint32 pos;
    pos = handlerInstance.findProperty(
        PEGASUS_PROPERTYNAME_CREATIONCLASSNAME);
    if (pos != PEG_NOT_FOUND)
    {
        handlerInstance.getProperty(pos).getValue().get
            (creationClassName);
    }
    handlerTypeFound = _HANDLER_CIMXML;
    destination = String::EMPTY;
    if (handlerInstance.getClassName() ==
        PEGASUS_CLASSNAME_INDHANDLER_SNMP)
    {
        handlerTypeFound = _HANDLER_SNMP;
        String targetHost;
        pos = handlerInstance.findProperty(
            PEGASUS_PROPERTYNAME_LSTNRDST_TARGETHOST);
        if (pos != PEG_NOT_FOUND)
        {
            handlerInstance.getProperty(pos).getValue().get
                (targetHost);
        }
        destination = targetHost;
    }
    else
    {
        if (handlerInstance.getClassName() ==
            PEGASUS_CLASSNAME_LSTNRDST_EMAIL)
        {
            handlerTypeFound = _HANDLER_EMAIL;
            Array <String> mailTo;
            pos = handlerInstance.findProperty
                (PEGASUS_PROPERTYNAME_LSTNRDST_MAILTO);
            if (pos != PEG_NOT_FOUND)
            {
                handlerInstance.getProperty(pos).getValue().get
                    (mailTo);
            }
            for (Uint32 eIndex=0; eIndex < mailTo.size();
                eIndex++)
            {
                destination.append (mailTo[eIndex]);
                destination.append (" ");
            }
         }
        else
        {
            if (creationClassName ==
                PEGASUS_CLASSNAME_LSTNRDST_SYSTEM_LOG)
            {
                handlerTypeFound = _HANDLER_SYSLOG;
            }
            else
            {
                pos = handlerInstance.findProperty(
                    PEGASUS_PROPERTYNAME_LSTNRDST_DESTINATION);
                if (pos != PEG_NOT_FOUND)
                {
                    handlerInstance.getProperty (pos).getValue ().get
                        (destination);
                 }
            }
        }
    }
}

//
//  print a verbose list of subscriptions
//
void CIMSubCommand::_printSubscriptionsVerbose(
    const Array<CIMInstance>& handlerInstancesFound,
    const Array<Uint32>& handlerTypesFound,
    const Array<String>& querysFound,
    const Array<ListColumnEntry>& listOutputTable,
    ostream& outPrintWriter)
{
    Uint32 maxEntries = listOutputTable[_SUBSCRIPTION_LIST_NS_COLUMN].size();
    Array <Uint32> indexes;
    for (Uint32 i = 0; i < maxEntries; i++)
    {
       indexes.append (i);
    }
    _bubbleIndexSort(listOutputTable[_SUBSCRIPTION_LIST_HANDLER_COLUMN], 0,
            indexes);
    _bubbleIndexSort(listOutputTable[_SUBSCRIPTION_LIST_FILTER_COLUMN], 0,
            indexes);
    _bubbleIndexSort(listOutputTable[_SUBSCRIPTION_LIST_NS_COLUMN], 0, indexes);
    for (Uint32 i = 0; i < maxEntries; i++)
    {
        outPrintWriter << "Namespace:         " <<
            (listOutputTable[_SUBSCRIPTION_LIST_NS_COLUMN])[indexes[i]] << endl;
        outPrintWriter << "Filter:            " <<
            (listOutputTable[_SUBSCRIPTION_LIST_FILTER_COLUMN])[indexes[i]]
            << endl;
        outPrintWriter << "Handler:           " <<
            (listOutputTable[_SUBSCRIPTION_LIST_HANDLER_COLUMN])[indexes[i]]
            << endl;
        outPrintWriter << "Query:             " << querysFound[indexes[i]]
                << endl;
        CIMInstance handlerInstance = handlerInstancesFound[indexes[i]];
        switch (handlerTypesFound[indexes[i]])
        {
            case _HANDLER_SNMP:
            {
                String targetHost;
                Uint32 pos = handlerInstance.findProperty(
                    PEGASUS_PROPERTYNAME_LSTNRDST_TARGETHOST);
                if (pos != PEG_NOT_FOUND)
                {
                    handlerInstance.getProperty(pos).getValue().get
                        (targetHost);
                }
                outPrintWriter << "TargetHost:        " << targetHost
                    << endl;
                outPrintWriter << "SNMPVersion:       " <<
                    _getSnmpVersion(handlerInstance) << endl;
                break;
            }
            case _HANDLER_EMAIL:
            {
                String mailCc;
                String mailTo;
                String mailSubject;
                _getEmailInfo(handlerInstance, mailCc,
                    mailTo, mailSubject );
                outPrintWriter << "MailTo:            " <<
                    mailTo << endl;
                if (mailCc.size() > 0 )
                {
                    outPrintWriter << "MailCc:            " <<
                        mailCc << endl;
                }
                if (mailSubject.size() > 0 )
                {
                    outPrintWriter << "MailSubject:       " <<
                        mailSubject << endl;
                }
                break;
            }
            case _HANDLER_SYSLOG:
            {
                break;
            }
            case _HANDLER_CIMXML:
            {
                String destination;
                Uint32 pos = handlerInstance.findProperty(
                    PEGASUS_PROPERTYNAME_LSTNRDST_DESTINATION);
                if (pos != PEG_NOT_FOUND)
                {
                    handlerInstance.getProperty(pos).getValue().get
                        (destination);
                }
                outPrintWriter << "Destination:       " << destination << endl;
            }
        }
        outPrintWriter << "SubscriptionState: " <<
            (listOutputTable[_SUBSCRIPTION_LIST_STATE_COLUMN])[indexes[i]] <<
            endl;
        outPrintWriter <<
            "-----------------------------------------" << endl;
    }
}

//
//  check a subscription for a filter match
//
Boolean CIMSubCommand::_filterMatches(
    const CIMObjectPath& subPath,
    const CIMNamespaceName& subscriptionNS,
    const String& filterName,
    const CIMNamespaceName& filterNamespace,
    CIMNamespaceName& filterNS,
    CIMObjectPath& filterRef)
{
    Boolean filterMatch = false;
    String filterNameString;

    if (!filterNamespace.isNull())
    {
        filterNS = filterNamespace;
    }
    else
    {
        filterNS = subscriptionNS;
    }
    //
    //  Get the subscription Filter
    //
    Array<CIMKeyBinding> keys = subPath.getKeyBindings();
    for( Uint32 j=0; j < keys.size(); j++)
    {
        if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_FILTER))
        {
            filterRef = keys[j].getValue();
        }
    }

    filterNameString = _getNameInKey(filterRef);
    CIMNamespaceName instanceNS = filterRef.getNameSpace();
    if (filterName != String::EMPTY)
    {
        if (filterNameString == filterName)
        {
            if (filterNamespace.isNull())
            {
                //
                //  If the Filter reference property value includes
                //  namespace, check if it is the namespace of the Filter.
                //  If the Filter reference property value does not
                //  include namespace, check if the current subscription
                //  namespace is the namespace of the Filter.
                //
                if (((instanceNS.isNull()) &&
                    (subscriptionNS == filterNS))
                    || (instanceNS == filterNS))
                {
                    filterMatch = true;
                }
            }
            else
            {
                // No namespace was specified and the filter name matches
                filterMatch = true;
                if (instanceNS.isNull())
                {
                    filterNS = subscriptionNS;
                }
                else
                {
                    filterNS = instanceNS;
                }
            }
        }
        else
        {
            // The filter name does not match
            filterMatch = false;
        }
    }
    else
    {
        filterMatch = true;
        // No filter name was specified.
        // Use the filter namespace if specified in the reference.
        //
        if (instanceNS.isNull())
        {
            filterNS = subscriptionNS;
        }
        else
        {
            filterNS = instanceNS;
        }
    }
    return filterMatch;
}

//
//  check a subscription for a handler match
//
Boolean CIMSubCommand::_handlerMatches(
    const CIMObjectPath& subPath,
    const CIMNamespaceName& subscriptionNS,
    const String& handlerName,
    const CIMNamespaceName& handlerNamespace,
    const String& handlerCreationClass,
    CIMNamespaceName& handlerNS,
    CIMObjectPath& handlerRef)
{
    Boolean handlerMatch = false;
    String handlerNameString;
    String creationClassName;

    if (!handlerNamespace.isNull())
    {
        handlerNS = handlerNamespace;
    }
    else
    {
        handlerNS = subscriptionNS;
    }
    //
    //  Get the subscription Handler
    //
    Array<CIMKeyBinding> keys = subPath.getKeyBindings();
    for( Uint32 j=0; j < keys.size(); j++)
    {
        if (keys[j].getName().equal(PEGASUS_PROPERTYNAME_HANDLER))
        {
            handlerRef = keys[j].getValue();
        }
    }
    handlerNameString = _getNameInKey(handlerRef);
    if (handlerName != String::EMPTY)
    {
        CIMNamespaceName instanceNS = handlerRef.getNameSpace();
        if (handlerNameString == handlerName)
        {
            if (handlerNamespace.isNull())
            {
                //
                //  If the Handler reference property value includes
                //  namespace, check if it is the namespace of the Handler.
                //  If the Handler reference property value does not
                //  include namespace, check if the current subscription
                //  namespace is the namespace of the Handler.
                //
                if (((instanceNS.isNull()) &&
                   (subscriptionNS == handlerNS))
                   || (instanceNS == handlerNS))
                {
                    handlerMatch = true;
                }
            }
            else
            {
                // Handler namespace is not set and handler name matches
                handlerMatch = true;
                if (instanceNS.isNull())
                {
                    handlerNS = subscriptionNS;
                }
                else
                {
                    handlerNS = instanceNS;
                }
            }
        }
        else
        {
            // Handler name does not match
            handlerMatch = false;
        }
        if (handlerMatch)
        {
            if(handlerCreationClass != String::EMPTY)
            {
                CIMInstance handlerInstance = _client->getInstance(
                    handlerNS, handlerRef);
                Uint32 pos = handlerInstance.findProperty(
                    PEGASUS_PROPERTYNAME_CREATIONCLASSNAME);
                if (pos != PEG_NOT_FOUND)
                {
                    handlerInstance.getProperty(pos).getValue().get
                        (creationClassName);
                }
                if (handlerCreationClass != creationClassName)
                {
                    handlerMatch = false;
                }
            }
        }
    }
    else
    {
        handlerMatch = true;
        //
        // The handler was not specified.
        // Use the handler namespace if specified in the reference.
        //
        CIMNamespaceName instanceNS = handlerRef.getNameSpace();
        if (!instanceNS.isNull())
        {
            handlerNS = instanceNS;
        }
        else
        {
            handlerNS = subscriptionNS;
        }
    }
    return handlerMatch;
}

//
//  Get the subscription state string from a subscription instance
//
void CIMSubCommand::_getEmailInfo(
    const CIMInstance& handlerInstance,
    String& ccString,
    String& toString,
    String& subjectString)
{
    Array <String> mailCc, mailTo;
    subjectString = String::EMPTY;
    mailTo.append(String::EMPTY);
    Uint32 pos =
        handlerInstance.findProperty(PEGASUS_PROPERTYNAME_LSTNRDST_MAILTO);
    if( pos != PEG_NOT_FOUND)
    {
        handlerInstance.getProperty(pos).getValue().get(mailTo);
    }
    for (Uint32 eIndex=0; eIndex < mailTo.size();
        eIndex++)
    {
        toString.append(mailTo[eIndex]);
        toString.append(" ");
    }
    pos = handlerInstance.findProperty(PEGASUS_PROPERTYNAME_LSTNRDST_MAILCC);
    if (pos != PEG_NOT_FOUND)
    {
        handlerInstance.getProperty(pos).getValue().get(mailCc);
    }
    if (mailCc.size() > 0)
    {
        for (Uint32 eIndex=0; eIndex < mailCc.size();
            eIndex++)
        {
            ccString.append (mailCc[eIndex]);
            ccString.append (" ");
        }
    }
    pos = handlerInstance.findProperty(
            PEGASUS_PROPERTYNAME_LSTNRDST_MAILSUBJECT);
    if (pos != PEG_NOT_FOUND)
    {
        handlerInstance.getProperty(pos).getValue().get(subjectString);
    }
}

//
//  Get the persistence value from the handler instance
//
String CIMSubCommand::_getPersistenceType(const CIMInstance& handlerInstance)
{
    Uint16 persistenceType = 1;
    Uint32 pos =
        handlerInstance.findProperty(PEGASUS_PROPERTYNAME_PERSISTENCETYPE);
    if (pos != PEG_NOT_FOUND)
    {
        handlerInstance.getProperty(pos).getValue().get(persistenceType);
    }
    String persistenceString;
    switch (persistenceType)
    {
        case PERSISTENCE_OTHER:
        {
            persistenceString = _PERSISTENTENCE_OTHER_STRING;
            break;
        }
        case PERSISTENCE_PERMANENT:
        {
            persistenceString = _PERSISTENTENCE_PERMANENT_STRING;
            break;
        }
        case PERSISTENCE_TRANSIENT:
        {
            persistenceString = _PERSISTENTENCE_TRANSIENT_STRING;
            break;
        }
        default:
            persistenceString = _PERSISTENTENCE_UNKNOWN_STRING;
      }
      return persistenceString;
}

//
//    Get the subscription state string from a subscription instance
//
String CIMSubCommand::_getSubscriptionState(
    const CIMNamespaceName& subscriptionNamespace,
    const CIMObjectPath& subPath)
{
    CIMInstance subInstance = _client->getInstance(subscriptionNamespace,
        subPath);
    Uint32 pos = subInstance.findProperty(
            PEGASUS_PROPERTYNAME_SUBSCRIPTION_STATE);
    Uint16 subscriptionState = STATE_UNKNOWN;
    if (pos != PEG_NOT_FOUND)
    {
        if (!subInstance.getProperty(pos).getValue().isNull() )
        {
            subInstance.getProperty(pos).getValue().get(subscriptionState);
        }
    }
    String statusString;
    switch (subscriptionState)
    {
        case STATE_UNKNOWN:
        {
            statusString = _SUBSCRIPTION_STATE_UNKNOWN_STRING;
            break;
        }
        case STATE_OTHER:
        {
            statusString = _SUBSCRIPTION_STATE_UNKNOWN_STRING;
            break;
        }
        case STATE_ENABLED:
        {
            statusString = _SUBSCRIPTION_STATE_ENABLED_STRING;
            break;
        }
        case STATE_ENABLEDDEGRADED:
        {
            statusString = _SUBSCRIPTION_STATE_ENABLED_DEGRADED_STRING;
            break;
        }
        case STATE_DISABLED:
        {
            statusString = _SUBSCRIPTION_STATE_DISABLED_STRING;
            break;
        }
        default:
            statusString = _SUBSCRIPTION_STATE_NOT_SUPPORTED_STRING;
    }
    return statusString;
}

//
//  Get the SNMP version string from a handler instance
//
String CIMSubCommand::_getSnmpVersion (const CIMInstance& handlerInstance)
{
    Uint16 snmpVersion = 0;
    Uint32 pos = handlerInstance.findProperty(PEGASUS_PROPERTYNAME_SNMPVERSION);
    if (pos != PEG_NOT_FOUND)
    {
        if (!handlerInstance.getProperty(pos).getValue().isNull() )
        {
            handlerInstance.getProperty(pos).getValue().get(snmpVersion);
        }
    }

    String snmpVersionString;
    switch (snmpVersion)
    {
        case SNMPV1_TRAP:
            snmpVersionString = _SNMP_VERSION_SNMPV1_TRAP_STRING;
            break;
        case SNMPV2C_TRAP:
            snmpVersionString = _SNMP_VERSION_SNMPV2C_TRAP_STRING;
            break;
        default:
            snmpVersionString = _SNMP_VERSION_PEGASUS_RESERVED_STRING;
    }
    return snmpVersionString;
}

//
//    print data in a columnar form
//
void CIMSubCommand::_printColumns(
    const Array <Uint32>& maxColumnWidth,
    const Array <ListColumnEntry>& listOutputTable,
    ostream& outPrintWriter)
{
    Uint32 maxColumns = maxColumnWidth.size();
    Uint32 maxEntries = listOutputTable[0].size();
    Array <Uint32> indexes;
    for (Uint32 i = 0; i < maxEntries; i++)
    {
       indexes.append(i);
    }
    for (int column = maxColumns-1; column >= 0; column--)
    {
      _bubbleIndexSort(listOutputTable[column], 1, indexes);
    }
    for (Uint32 i = 0; i < maxEntries; i++)
    {
        for (Uint32 column = 0; column < maxColumns-1; column++)
        {
            Uint32 outputItemSize =
                (listOutputTable[column])[indexes[i]].size();
            Uint32 fillerLen = maxColumnWidth[column] + TITLE_SEPERATOR_LEN -
                outputItemSize;
            outPrintWriter << (listOutputTable[column])[indexes[i]];
            for (Uint32 j = 0; j < fillerLen; j++)
            {
                outPrintWriter << ' ';
            }
        }
        outPrintWriter << (listOutputTable[maxColumns-1])[indexes[i]] << endl;
    }
}

//
//    Sort a string array by indexes
//
void CIMSubCommand::_bubbleIndexSort(
    const Array<String>& x,
    const Uint32 startIndex,
    Array<Uint32>& index)
{
    Uint32 n = x.size();

    if (n < 3)
        return;

    for (Uint32 i = startIndex; i < (n-1); i++)
    {
        for (Uint32 j = startIndex; j < (n-1); j++)
        {
            if (String::compareNoCase(x[index[j]],
                                      x[index[j+1]]) > 0)
            {
                Uint32 t = index[j];
                index[j] = index[j+1];
                index[j+1] = t;
            }
        }
    }
}

PEGASUS_NAMESPACE_END

//
// exclude main from the Pegasus Namespace
//
PEGASUS_USING_PEGASUS;

PEGASUS_USING_STD;

///////////////////////////////////////////////////////////////////////////////
/**
    Parses the command line, and execute the command.

    @param   args  the string array containing the command line arguments
*/
///////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
    AutoPtr<CIMSubCommand> command;
    Uint32 retCode;

#ifdef PEGASUS_OS_ZOS
    // for z/OS set stdout and stderr to EBCDIC
    setEBCDICEncoding(STDOUT_FILENO);
    setEBCDICEncoding(STDERR_FILENO);
#endif

    MessageLoader::_useProcessLocale = true;
    MessageLoader::setPegasusMsgHomeRelative(argv[0]);

    command.reset(new CIMSubCommand());
    try
    {
        command->setCommand (cout, cerr, argc, argv);
    }
    catch (CommandFormatException& cfe)
    {
        cerr << COMMAND_NAME << ": " << cfe.getMessage() << endl;

        MessageLoaderParms parms(ERR_USAGE_KEY,ERR_USAGE);
        parms.msg_src_path = MSG_PATH;
        cerr << COMMAND_NAME <<
            ": " << MessageLoader::getMessage(parms) << endl;

        exit (Command::RC_ERROR);
    }
    retCode = command->execute(cout, cerr);
    return (retCode);
}
