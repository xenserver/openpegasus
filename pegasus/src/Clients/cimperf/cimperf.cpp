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

/* This is a simplistic display program for the CIMOM performance
    characteristics.
    This version simply gets the instances of the performace class and displays
    the resulting average counts.
    TODO  KS
    1. Convert to use the correct class when it is available.
    2. Get the header information from the class, not fixed.
    3. Keep history and present so that there is a total
    4. Do Total so that we have overall counts.
    5. Do percentages
*/
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <stdlib.h>
#include <Pegasus/Common/HTTPConnector.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/CIMDateTime.h>
#include <Pegasus/Common/PegasusVersion.h>
#include <Pegasus/Common/StatisticalData.h>
#include <Pegasus/Common/HostAddress.h>

#include <Pegasus/Client/CIMClient.h>

#include <Pegasus/General/OptionManager.h>
#ifdef PEGASUS_OS_ZOS
#include <Pegasus/General/SetFileDescriptorToEBCDICEncoding.h>
#endif

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

const String DEFAULT_NAMESPACE = "root/cimv2";

// The table on the right represents the mapping from the enumerated types
// in the CIM_CIMOMStatisticalDate class ValueMap versus the internal
// message type defined in Message.h.
//

const char* _OPERATION_NAME[] =
{
    //                                   Enumerated       ValueMap Value
    //                                   value from       from class
    //                                   internal         CIM_StatisticalData
    //                                   StatisticalData
    //                                   ---------------  -------------------
    "GetClass",                       //     0               3
    "GetInstance",                    //     1               4
    "IndicationDelivery",             //     2              26
    "DeleteClass",                    //     3               5
    "DeleteInstance",                 //     4               6
    "CreateClass",                    //     5               7
    "CreateInstance",                 //     6               8
    "ModifyClass",                    //     7               9
    "ModifyInstance",                 //     8              10
    "EnumerateClasses",               //     9              11
    "EnumerateClassNames",            //    10              12
    "EnumerateInstances",             //    11              13
    "EnumerateInstanceNames",         //    12              14
    "ExecQuery",                      //    13              15
    "Associators",                    //    14              16
    "AssociatorNames",                //    15              17
    "References",                     //    16              18
    "ReferenceNames",                 //    17              19
    "GetProperty",                    //    18              20
    "SetProperty",                    //    19              21
    "GetQualifier",                   //    20              22
    "SetQualifier",                   //    21              23
    "DeleteQualifier",                //    22              24
    "EnumerateQualifiers",            //    23              25
    "InvokeMethod"                    //    24              Not Present, use
                                      //                      1 "Other"
};

//------------------------------------------------------------------------------
//
// _indent()
//
//------------------------------------------------------------------------------

static void _indent(PEGASUS_STD(ostream)& os, Uint32 level, Uint32 indentSize)
{
    Uint32 n = level * indentSize;
    if (n > 50)
    {
        cout << "Jumped Ship " << level << " size " << indentSize << endl;
        exit(1);
    }

    for (Uint32 i = 0; i < n; i++)
        os << ' ';
}

void mofFormat(
    PEGASUS_STD(ostream)& os,
    const char* text,
    Uint32 indentSize)
{
    char* var = new char[strlen(text)+1];
    char* tmp = strcpy(var, text);
    Uint32 count = 0;
    Uint32 indent = 0;
    Boolean quoteState = false;
    Boolean qualifierState = false;
    char c;
    char prevchar='\0';
    while ((c = *tmp++) != '\0')
    {
        count++;
        // This is too simplistic and must move to a token based mini parser
        // but will do for now. One problem is tokens longer than 12 characters
        // that overrun the max line length.
        switch (c)
        {
            case '\n':
                os << Sint8(c);
                prevchar = c;
                count = 0 + (indent * indentSize);
                _indent(os, indent, indentSize);
                break;

            case '\"':   // quote
                os << Sint8(c);
                prevchar = c;
                quoteState = !quoteState;
                break;

            case ' ':
                os << Sint8(c);
                prevchar = c;
                if (count > 66)
                {
                    if (quoteState)
                    {
                        os << "\"\n";
                        _indent(os, indent + 1, indentSize);
                        os <<"\"";
                    }
                    else
                    {
                        os <<"\n";
                        _indent(os, indent + 1,  indentSize);
                    }
                    count = 0 + ((indent + 1) * indentSize);
                }
                break;
            case '[':
                if (prevchar == '\n')
                {
                    indent++;
                    _indent(os, indent,  indentSize);
                    qualifierState = true;
                }
                os << Sint8(c);
                prevchar = c;
                break;

            case ']':
                if (qualifierState)
                {
                    if (indent > 0)
                        indent--;
                    qualifierState = false;
                }
                os << Sint8(c);
                prevchar = c;
                break;

            default:
                os << Sint8(c);
                prevchar = c;
        }
    }
    delete [] var;
}

/* Method to build an OptionManager object - which holds and organizes options
   and the properties */

void GetOptions(
    OptionManager& om,
    int& argc,
    char** argv,
    const String& testHome)
{
    static struct OptionRow optionsTable[] =
    //The values in the OptionRows below are:
    //optionname, defaultvalue, is required, type, domain, domainsize, flag,
    //  hlpmsg
    {
        {"port", "5988", false, Option::INTEGER, 0, 0, "p",
            "specifies port"},

        {"location", "localhost", false, Option::STRING, 0, 0, "h",
                "specifies hostname of system"},

        {"version", "false", false, Option::BOOLEAN, 0, 0, "-version",
                "Displays software Version "},

        {"help", "false", false, Option::BOOLEAN, 0, 0, "-help",
                "Prints help message with command line options "},

        {"user name","",false,Option::STRING, 0, 0, "u",
                "specifies user loging in"},

        {"password","",false,Option::STRING, 0, 0, "w",
                "login password for user"},

    };
    const Uint32 NUM_OPTIONS = sizeof(optionsTable) / sizeof(optionsTable[0]);

    om.registerOptions(optionsTable, NUM_OPTIONS);

    //We want to make this code common to all of the commands

    String configFile = "/CLTest.conf";

    if (FileSystem::exists(configFile))
    {
        om.mergeFile(configFile);
    }

    om.mergeCommandLine(argc, argv);

    om.checkRequiredOptions();
}


/* Method that maps from operation type to operation name. */

const char* opTypeToOpName(Uint16 type)
{
    const char* opName = NULL;
    switch (type)
    {
        case 3:
            opName = _OPERATION_NAME[StatisticalData::GET_CLASS];
            break;

        case 4:
            opName = _OPERATION_NAME[StatisticalData::GET_INSTANCE];
            break;

        case 5:
            opName = _OPERATION_NAME[StatisticalData::DELETE_CLASS];
            break;

        case 6:
            opName = _OPERATION_NAME[StatisticalData::DELETE_INSTANCE];
            break;

        case 7:
            opName = _OPERATION_NAME[StatisticalData::CREATE_CLASS];
            break;

        case 8:
            opName = _OPERATION_NAME[StatisticalData::CREATE_INSTANCE];
            break;

        case 9:
            opName = _OPERATION_NAME[StatisticalData::MODIFY_CLASS];
            break;

        case 10:
            opName = _OPERATION_NAME[StatisticalData::MODIFY_INSTANCE];
            break;

        case 11:
            opName = _OPERATION_NAME[StatisticalData::ENUMERATE_CLASSES];
            break;

        case 12:
            opName = _OPERATION_NAME[StatisticalData::ENUMERATE_CLASS_NAMES];
            break;

        case 13:
            opName = _OPERATION_NAME[StatisticalData::ENUMERATE_INSTANCES];
            break;

        case 14:
            opName = _OPERATION_NAME[StatisticalData::ENUMERATE_INSTANCE_NAMES];
            break;

        case 15:
            opName = _OPERATION_NAME[StatisticalData::EXEC_QUERY];
            break;

        case 16:
            opName = _OPERATION_NAME[StatisticalData::ASSOCIATORS];
            break;

        case 17:
            opName = _OPERATION_NAME[StatisticalData::ASSOCIATOR_NAMES];
            break;

        case 18:
            opName = _OPERATION_NAME[StatisticalData::REFERENCES];
            break;

        case 19:
            opName = _OPERATION_NAME[StatisticalData::REFERENCE_NAMES];
            break;

        case 20:
            opName = _OPERATION_NAME[StatisticalData::GET_PROPERTY];
            break;

        case 21:
            opName = _OPERATION_NAME[StatisticalData::SET_PROPERTY];
            break;

        case 22:
            opName = _OPERATION_NAME[StatisticalData::GET_QUALIFIER];
            break;

        case 23:
            opName = _OPERATION_NAME[StatisticalData::SET_QUALIFIER];
            break;

        case 24:
            opName = _OPERATION_NAME[StatisticalData::DELETE_QUALIFIER];
            break;

        case 25:
            opName = _OPERATION_NAME[StatisticalData::ENUMERATE_QUALIFIERS];
            break;

        case  26:
            opName = _OPERATION_NAME[StatisticalData::INDICATION_DELIVERY];
            break;

        case 1:
            opName = _OPERATION_NAME[StatisticalData::INVOKE_METHOD];
            break;

        default:
            //Invalid type
            opName = "Dummy Response";
    }
    return opName;
}

int main(int argc, char** argv)
{

#ifdef PEGASUS_OS_ZOS
    // for z/OS set stdout and stderr to EBCDIC
    setEBCDICEncoding(STDOUT_FILENO);
    setEBCDICEncoding(STDERR_FILENO);
#endif

    // Get options (from command line and from configuration file); this
    // removes corresponding options and their arguments from the command
    // line.

    OptionManager om;

    try
    {
        String testHome = ".";
        GetOptions(om, argc, argv, testHome);
        // om.print();
    }
    catch (Exception& e)
    {
        cerr << argv[0] << ": " << e.getMessage() << endl;
        String header = "Usage ";
        String trailer = "";
        om.printOptionsHelpTxt(header, trailer);
        exit(1);
    }

    // Establish the namespace from the input parameters
    String nameSpace = "root/cimv2";

    // Check to see if user asked for help (--help option)
    if (om.valueEquals("help", "true"))
    {
        String header = "Usage ";
        String trailer = "";
        om.printOptionsHelpTxt(header, trailer);
        exit(0);
    }
    else if (om.valueEquals("version", "true"))
    {
        cerr << "Version " << PEGASUS_PRODUCT_VERSION << endl;
        exit(0);
    }

    //Get hostname form (option manager) command line if none use default
    String location;
    om.lookupValue("location", location);
    HostAddress addr(location);
    if (!addr.isValid())
    {
        cerr << "Invalid Locator : " << location << endl;
        exit(1);
    }

    // Get port number from (option manager) command line; if none use the
    // default.  The lookup will always be successful since the optionTable
    // has a default value for this option.
    String str_port;
    Uint32 port = 0;
    if (om.lookupValue("port", str_port))
    {
        port = (Uint32) atoi(str_port.getCString());
    }

    //Get user name and password
    String userN;
    String passW;
    om.lookupValue("user name", userN);
    om.lookupValue("pass word", passW);

    /*
    The next section of code connects to the server and enumerates all the
    instances of the CIM_CIMOMStatisticalData class. The instances are held in
    an Array named "instances". The output of cimperf is a table of averages.
    */


    String className = "CIM_CIMOMStatisticalData";
    CIMClient client;

    try
    {
        if (String::equal(location,"localhost"))
            client.connectLocal();
        else
            client.connect(location, port, userN, passW);
    }

    catch (Exception& e)
    {
        cerr << argv[0] << " Exception connecting to : " << location << endl;
        cerr << e.getMessage() << endl;
        exit(1);
    }


    try
    {
        Boolean localOnly = false;
        Boolean deepInheritance = false;
        Boolean includeQualifiers = false;
        Boolean includeClassOrigin = false;

        Array<CIMInstance> instances;
        instances = client.enumerateInstances(nameSpace,
            className,
            deepInheritance,
            localOnly,
            includeQualifiers,
            includeClassOrigin);

        // First print the header for table of values
        printf("%-25s%10s %10s %10s %10s %10s\n",
            "Operation", "Number of", "Server", "Provider", "Request",
            "Response");

        printf("%-25s%10s %10s %10s %10s %10s\n",
            "Type", "Requests", "Time", "Time", "Size", "Size");

        printf("%-25s%10s %10s %10s %10s %10s\n",
            " ", " ", "(usec)", "(usec)", "(bytes)", "(bytes)");

        printf("%-25s\n", "-------------------------------------------"
                          "------------------------------------");

        // This section of code loops through all the instances of
        // CIM_CIMOMStatisticalData (one for each intrinsic request type) and
        // gathers the NumberofOperations, CIMOMElapsedTime,
        // ProviderElapsedTime, ResponseSize and RequestSize for each instance.
        // Averages are abtained by dividing times and sizes by
        // NumberofOperatons.

        for (Uint32 inst = 0; inst < instances.size(); inst++)
        {
            CIMInstance instance = instances[inst];

            // Get the request type property for this instance.
            // Note that for the moment it is simply an integer.
            Uint32 pos;
            CIMProperty p;
            // Operation Type is decoded as const char*, hence type has
            // changed from string to const char*
            const char* statName = NULL;
            CIMValue v;
            Uint16 type;
            if ((pos = instance.findProperty("OperationType")) != PEG_NOT_FOUND)
            {
                p = (instance.getProperty(pos));
                v = p.getValue();
                if (v.getType() == CIMTYPE_UINT16)
                {
                    v.get(type);
                    statName = opTypeToOpName(type);
                }
            }
            else
            {
                statName = "UNKNOWN";
            }

            // Get number of requests property - "NumberofOperations"
            Uint64 numberOfRequests = 0;
            if ((pos = instance.findProperty("NumberOfOperations")) !=
                PEG_NOT_FOUND)
            {

                p = (instance.getProperty(pos));
                v = p.getValue();

                if (v.getType() == CIMTYPE_UINT64)
                {
                    v.get(numberOfRequests);

                }
                else
                {
                    cerr << "NumberofOperations was not a CIMTYPE_SINT64 and"
                            " should be" << endl;
                }
            }
            else
            {
                cerr << "Could not find NumberofOperations" << endl;
            }

            // Get the total CIMOM Time property "CIMOMElapsedTime"
            // in order to calculate the averageCimomTime.
            CIMDateTime totalCimomTime;
            Sint64 averageCimomTime = 0;
            Uint64 totalCT = 0;

            if ((pos = instance.findProperty("CimomElapsedTime")) !=
                PEG_NOT_FOUND)
            {
                p = (instance.getProperty(pos));
                v = p.getValue();

                if (v.getType() == CIMTYPE_DATETIME)
                {
                    v.get(totalCimomTime);
                    totalCT = totalCimomTime.toMicroSeconds();
                }
                else
                {
                    cerr << "Error Property value " << "CimomElapsedTime" <<
                        endl;
                }
            }
            else
            {
                cerr << "Error Property " << "CimomElapsedTime" << endl;
            }

            if (numberOfRequests != 0)
            {
                averageCimomTime = totalCT / numberOfRequests;
            }

            // Get the total Provider Time property "ProviderElapsedTime"
            CIMDateTime totalProviderTime;
            Uint64 averageProviderTime = 0;
            Uint64 totalPT = 0;

            if ((pos = instance.findProperty("ProviderElapsedTime")) !=
                PEG_NOT_FOUND)
            {
                p = (instance.getProperty(pos));
                v = p.getValue();
                if (v.getType() == CIMTYPE_DATETIME)
                {
                    v.get(totalProviderTime);
                    totalPT = totalProviderTime.toMicroSeconds();
                }
                else
                {
                    cerr << "Error Property Vlaue " << "ProviderElapsedTime" <<
                        endl;
                }
            }
            else
            {
                cerr << "Error Property " << "ProviderElapsedTime" << endl;
            }

            if (numberOfRequests != 0)
            {
                averageProviderTime = totalPT / numberOfRequests;
            }

            // Get the total Response size property "ResponseSize"
            Uint64 totalResponseSize = 0;
            Uint64 averageResponseSize = 0;

            if ((pos = instance.findProperty("ResponseSize")) != PEG_NOT_FOUND)
            {
                p = (instance.getProperty(pos));
                v = p.getValue();

                if (v.getType() == CIMTYPE_UINT64)
                {
                    v.get(totalResponseSize);
                }
                else
                {
                    cerr << "RequestSize is not of type CIMTYPE_SINT64" <<
                        endl ;
                }
            }
            else
            {
                cerr << "Could not find ResponseSize property" << endl;
            }

            if (numberOfRequests != 0)
            {
                averageResponseSize =  totalResponseSize / numberOfRequests;
            }

            //Get the total request size property "RequestSize"
            Uint64 totalRequestSize = 0;
            Uint64 averageRequestSize = 0;

            if ((pos = instance.findProperty("RequestSize")) != PEG_NOT_FOUND)
            {
                p = (instance.getProperty(pos));
                v = p.getValue();

                if (v.getType() == CIMTYPE_UINT64)
                {
                    v.get(totalRequestSize);
                }
                else
                {
                    cerr << "RequestSize is not of type CIMTYPE_SINT64" << endl;
                }
            }
            else
            {
                cerr << "Could not find RequestSize property" << endl;
            }

            if (numberOfRequests != 0)
            {
                averageRequestSize = totalRequestSize / numberOfRequests;
            }

            //if StatisticalData::copyGSD is FALSE this will only return 0's

            printf("%-25s"
                "%10"  PEGASUS_64BIT_CONVERSION_WIDTH "u"
                "%11" PEGASUS_64BIT_CONVERSION_WIDTH "d"
                "%11" PEGASUS_64BIT_CONVERSION_WIDTH "u"
                "%11" PEGASUS_64BIT_CONVERSION_WIDTH "u"
                "%11" PEGASUS_64BIT_CONVERSION_WIDTH "u\n",
                statName,
                numberOfRequests, averageCimomTime,
                averageProviderTime, averageRequestSize,
                averageResponseSize);
        }
    }
    catch (Exception& e)
    {
        cerr << argv[0] << "Exception : " << e.getMessage() << endl;
        exit(1);
    }

    return 0;
}


