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
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/XmlWriter.h>
#include <Pegasus/General/MofWriter.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/StringConversion.h>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Common/PegasusVersion.h>

#include "CIMCLIClient.h"
#include "ObjectBuilder.h"
#include "CIMCLIOutput.h"
#include "CIMCLICommon.h"
#include "CIMCLIClient.h"
#include "CIMCLIOperations.h"

PEGASUS_USING_STD;
PEGASUS_NAMESPACE_BEGIN
/*****************************************************************************
**
**    Usage and Help Functions
**
******************************************************************************/
// Character sequences used in help/usage output.

static const char * usage = "This command executes single CIM Operations.\n";

static const char* optionText = "Valid options for this command are : ";
static const char* commonOptions = "    -count, -d, -delay, -p, -l, -u, -o, -x,\
 -v, --sum, --timeout, -r, --t ";

/*
    This function loads the message from resourcebundle using the key passed
*/
String loadMessage(const char* key, const char* defMessage)
{
    MessageLoaderParms parms(key, defMessage);
    parms.msg_src_path = MSG_PATH;
    return MessageLoader::getMessage(parms);
}

void showExamples()
{

    Operations operations;
    cout <<
        loadMessage(
            "Clients.cimcli.CIMCLIClient.EXAMPLES_STRING",
            "Examples : ")
         << endl;

    while(operations.more())
    {
        OperationExampleEntry example = operations.getExampleEntry();
        cout << loadMessage(example.msgKey, example.Example) << endl;


        OperationTableEntry thisOperation = operations.next();
    }
}


/* Remap a long string into a multi-line string that can be positioned on a
   line starting at pos and with length defined for each line.
   Each output line consists of fill parameter to pos and max line length
   defined by length parameter.

   The input string is recreated by tokenizing on the space character
   and filled from the left so that the returned string can be output
   as a multiline string starting at pos.
*/

String formatLongString (const char * input, Uint32 pos, Uint32 lineLength)
{
    String output;
    String work = input;
    Array<String> list;
    Uint32 textLength = lineLength - pos;
    // create the fill string starting with the newline character
    String fill;
    fill.append("\n");
    for (Uint32 i = 0; i < pos; i++)
        fill.append (" ");

    list = _tokenize(work, ' ', true);

    for (Uint32 i = 0 ; i < list.size() ; i++)
    {
        // move a single word and either a space or create new line
        if (((output.size() % (textLength)) + list[i].size()) >= (textLength))
        {
            output.append(fill);
        }
        else
        {
            output.append(" ");
        }

        output.append(list[i]);
    }
    return(output);
}

/* showOperations - Display the list of operations possible based
   on what is in the operations table.
   FUTURE: This should probably be in the Operations.cpp file
*/

void showOperations(const char* pgmName, Uint32 lineLength)
{
    Uint32 indent = 28;
    Operations operations;

    while(operations.more())
    {
        OperationTableEntry thisOperation = operations.next();

        char * opStr= new char[500];
        String txtFormat = formatLongString(
            thisOperation.UsageText,
            indent,
            lineLength);
        CString ctxtFormat=txtFormat.getCString();

        sprintf(
            opStr,
            "\n%-5s %-21s",
            thisOperation.ShortCut,
            thisOperation.OperationName);

        opStr = strcat(opStr, (const char*)ctxtFormat);

        cout << loadMessage(
            thisOperation.msgKey,
            const_cast<const char*>(opStr));

        delete[] opStr;
    }
    cout << loadMessage(
        "Clients.cimcli.CIMCLIClient.HELP_SUMMARY",
        " -h for all help, -hc for commands, -ho for options")
        << endl;
}

void showVersion(const char* pgmName, OptionManager& om)
{
    String str = "";
    str.append("Version ");
    str.append(PEGASUS_PRODUCT_VERSION);

    CString cstr = str.getCString();
    MessageLoaderParms parms(
        "Clients.cimcli.CIMCLIClient.VERSION",
        (const char*)cstr,
        PEGASUS_PRODUCT_VERSION);
    parms.msg_src_path = MSG_PATH;
    cout << MessageLoader::getMessage(parms) << endl;
}

void showOptions(const char* pgmName, OptionManager& om)
{

    String optionsTrailer = loadMessage(
        "Clients.cimcli.CIMCLIClient.OPTIONS_TRAILER",
        "Options vary by command consistent with CIM Operations");
    cout << loadMessage(
        "Clients.cimcli.CIMCLIClient.OPTIONS_HEADER",
        "The options for this command are:\n");

    String usageStr;
    usageStr = loadMessage(
        "Clients.cimcli.CIMCLIClient.OPTIONS_USAGE",
        usage);
    om.printOptionsHelpTxt(usageStr, optionsTrailer);
}

void showUsage()
{
    String usageText;
    usageText =
        "Usage: cimcli <command> <CIMObject> <Options> *<extra parameters>\n"
        "    -hc    for <command> set and <CimObject> for each command\n"
        "    -ho    for <Options> set\n"
        "    -h xx  for <command> and <Example> for <xx> operation \n"
        "    -h     for this summary\n"
        "    --help for full help\n";
    CString str = usageText.getCString();
    cout << loadMessage("Clients.cimcli.CIMCLIClient.MENU.STANDARD",
        (const char*)str);
}


/* showFullHelpMsg - Show all of the various help groups including
   usage, version, options, commands, and examples.
*/
void showFullHelpMsg(
    const char* pgmName,
    OptionManager& om,
    Uint32 lineLength)
{
    showUsage();

    showVersion(pgmName, om);

    showOptions(pgmName, om);

    showOperations(pgmName, lineLength);

    cout << endl;

    showExamples();
}

/*
    Show the usage of a single command
    cmd param is shortcut or operation name for the target command.
*/
Boolean showOperationUsage(const char* cmd, OptionManager& om,
                           Uint32 lineLength)
{
    // indent for subsequent lines for help output
    // This value is an arbitary decision.
    Uint32 indent = 28;

    if (cmd)
    {
        // Find the command or the short cut name
        Operations operations;
        if (operations.find(cmd))
        {
            OperationTableEntry thisOperation = operations.get();
            Uint32 index = operations.getIndex();

            OperationExampleEntry example = operations.getExampleEntry();

            // format the shortcut and
            // command string into a single output string.
            char * opStr= new char[1000];
            sprintf(
                opStr,
                "\n%-5s %-21s",
                thisOperation.ShortCut,
                thisOperation.OperationName);
            // Append formatted usage text to the command information
            String txtFormat = formatLongString(
                thisOperation.UsageText, indent ,lineLength);

            CString ctxtFormat=txtFormat.getCString();
            opStr = strcat(opStr, (const char*)ctxtFormat);

            // output the command and usage information
            cout << loadMessage(
                thisOperation.msgKey,
                const_cast<const char*>(opStr))
                << endl;

            delete[] opStr;

            // Output the corresponding Example and Options information
            cout << loadMessage("Clients.cimcli.CIMCLIClient.EXAMPLE_STRING",
                        "Example : ")
                << endl;
            cout << loadMessage(example.msgKey, example.Example)
                << endl;
            cout << loadMessage("Clients.cimcli.CIMCLIClient.OPTIONS_STRING",
                        optionText)
                << endl;
            cout << loadMessage( example.optionsKey, example.Options)
                << endl;

            // Output the common Options information
            char * commonOptStr = new char[800];
            sprintf(commonOptStr, "%s", "Common Options are : \n");
            commonOptStr = strcat(commonOptStr, commonOptions);
            cout << loadMessage("Clients.cimcli.CIMCLIClient."
                        "COMMON_OPTIONS_STRING",
                        commonOptStr)
                << endl;
            delete[] commonOptStr;
            return true;
        }
        else
        {
            cerr << "Command \"" << cmd
                    << "\" not legal cimcli operation name.\n"
                    " Type cimcli -hc to list valid commands."
                 << endl;
            cout << loadMessage(
                "Clients.cimcli.CIMCLIClient.HELP_SUMMARY",
                " -h for all help, -hc for commands, -ho for options")
                << endl;
            return false;
        }
    }
    else
    {
        cerr << "Error: Input Parameter with Operation Name required.\n"
             << "     ex. cimcli -h di. or \n"
             << "         cimcli -h deleteinstance"
             << endl;
        cout << loadMessage(
            "Clients.cimcli.CIMCLIClient.HELP_SUMMARY",
            " -h for all help, -hc for commands, -ho for options")
            << endl;
            return false;
    }
}

PEGASUS_NAMESPACE_END
// END_OF_FILE
