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
#include "CIMCLIClient.h"
#include "CIMCLIOutput.h"
#include "CIMCLICommon.h"


PEGASUS_USING_STD;
PEGASUS_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
//
// Class, Instance, Object, QualifierDecl Array Sorts
//
//------------------------------------------------------------------------------

static inline int _compareCIMNames(const CIMName& c1, const CIMName& c2)
{
    return String::compareNoCase(
        c1.getString(),
        c2.getString());
}

// Function to compare two object paths.   Returns a negative integer if p1 is
// lexographically less than p2, 0 if p1 and p2 are equal,
// and a positive integer otherwise.

// Compare values compares the value elements of two CIMObject Paths based
// on their types. Strings are compared directly, integers compared as
// integers, references are compared as CIMObjectPaths.

// Compare a single KeyBinding.  Compares names as CIMName and values based
// on type.
static int _compareObjectPaths(const CIMObjectPath& p1,
                               const CIMObjectPath& p2);

static int _compareKeyBinding(const CIMKeyBinding& kb1,
                              const CIMKeyBinding& kb2)
{
    int rtn;

    if ((rtn = _compareCIMNames(kb1.getName(), kb2.getName())) == 0)
    {
        switch (kb1.getType())
        {
            case CIMKeyBinding::REFERENCE:
                // Convert to paths and recurse through compare.
                try
                {
                    CIMObjectPath p1(kb1.getValue());
                    CIMObjectPath p2(kb2.getValue());
                    rtn = _compareObjectPaths(p1,p2);
                }
                catch (Exception&)
                {
                    // ignore if parsing fails
                    cerr << "Reference Path parsing failed" << endl;
                    rtn = 0;
                }
                break;

            case CIMKeyBinding::BOOLEAN:
                // Compare as no case strings
                rtn = String::compareNoCase(kb1.getValue(), kb1.getValue());
                break;

            case CIMKeyBinding::NUMERIC:
                // convert to numeric values and compare
                Uint64 uValue1;
                Sint64 sValue1;
                if (StringConversion::stringToUnsignedInteger(
                    kb1.getValue().getCString(),
                    uValue1))
                {
                    Uint64 uValue2;
                    if (StringConversion::stringToUnsignedInteger(
                        kb2.getValue().getCString(),
                        uValue2))
                    {
                        rtn = (uValue2 - uValue1);
                    }
                    else
                    {
                        // ignore error where we cannot convert both
                        rtn = 0;
                    }
                }
                // Next try converting to signed integer
                else if (StringConversion::stringToSignedInteger(
                    kb1.getValue().getCString(),
                    sValue1))
                {
                    Sint64 sValue2;
                    if (StringConversion::stringToSignedInteger(
                        kb2.getValue().getCString(),
                        sValue2))
                    {
                        rtn = (sValue2 - sValue1);
                    }
                    else
                    {
                        rtn = 0;
                    }
                }
                break;
            default:
                // no conversion required.  Compare as Strings.
                rtn = String::compare(kb1.getValue(), kb2.getValue());
                break;
        }
    }

    return rtn;
}

/*
    Compare two object paths. Compares by running compare on all of
    the component parts, host name, namespace name, class name, and the
    value component of each of the keybindings.
    NOTE: This assumes that the keybindings are previously sorted which is
    the case today for the Pegasus CIMObjectPath constructor.
    @return int negative if considered lt, 0 if eaual and positive if
    the compares result and of the components og p2 gt p1.
*/
static int _compareObjectPaths(const CIMObjectPath& p1, const CIMObjectPath& p2)
{
    int rtn;
    if ((rtn = String::compareNoCase(p1.getHost(),
                                     p2.getHost())) != 0)
    {
        return rtn;
    }
    if ((rtn = String::compareNoCase(p1.getNameSpace().getString(),
                                     p2.getNameSpace().getString())) != 0)
    {
        return rtn;
    }
    if ((rtn = _compareCIMNames(p1.getClassName(),
                                p2.getClassName())) != 0)
    {
        return rtn;
    }

    Array<CIMKeyBinding> kb1 = p1.getKeyBindings();
    Array<CIMKeyBinding> kb2 = p2.getKeyBindings();

    for (Uint32 i = 0 ; i < kb1.size() ; i++)
    {
        int rtn1;

        if ((rtn1 = _compareKeyBinding(kb1[i], kb2[i])) != 0)
        {
            return rtn1;
        }
    }
    // success, return successful
    return 0;
}
// Sort an array of object paths
static inline int _compareObjectPaths(const void* p1, const void* p2)
{
    const CIMObjectPath* pa = (const CIMObjectPath*)p1;
    const CIMObjectPath* pb = (const CIMObjectPath*)p2;
    return _compareObjectPaths(*pa , *pb);
}
static void _Sort(Array<CIMObjectPath>& x)
{
    CIMObjectPath* data = (CIMObjectPath*)x.getData();
    Uint32 size = x.size();

    if (size > 1)
    {
        qsort((void*)data, size, sizeof(CIMObjectPath), _compareObjectPaths);
    }
}

// Sort an array of classes using the class names as the comparator
static inline int _compareClasses(const void* p1, const void* p2)
{
    const CIMClass* c1 = (const CIMClass*)p1;
    const CIMClass* c2 = (const CIMClass*)p2;
    return String::compareNoCase(
        c1->getClassName().getString(),
        c2->getClassName().getString());
}
static void _Sort(Array<CIMClass>& x)
{
    CIMClass* data = (CIMClass*)x.getData();
    Uint32 size = x.size();

    if (size > 1)
    {
        qsort((void*)data, size, sizeof(CIMClass), _compareClasses);
    }
}

// Sort an Array of instances using the paths as the compare data.
static int _compareInstances(const void* p1, const void* p2)
{
    const CIMInstance* c1 = (const CIMInstance*)p1;
    const CIMInstance* c2 = (const CIMInstance*)p2;

    return _compareObjectPaths(c1->getPath() , c2->getPath());
}
static void _Sort(Array<CIMInstance>& x)
{
    CIMInstance* data = (CIMInstance*)x.getData();
    Uint32 size = x.size();

    if (size > 1)
    {
        qsort((void*)data, size, sizeof(CIMInstance), _compareInstances);
    }
}

// Sort array of CIMObjects.  If the object is a class, compare the
// class names.  If it is an instance, compare the paths.
static int _compareObjects(const void* p1, const void* p2)
{
    const CIMObject* o1 = (const CIMObject*)p1;
    const CIMObject* o2 = (const CIMObject*)p2;
    if (o1->isClass())
    {
        return String::compareNoCase(
            o1->getClassName().getString(),
            o2->getClassName().getString());
    }
    else if (o1->isInstance())
    {
        return _compareObjectPaths(o1->getPath() , o2->getPath());
    }
    else
    {
        cout << "ERROR object not class or instance" << endl;
        return 0;
    }
}
static void _Sort(Array<CIMObject>& x)
{
    CIMObject* data = (CIMObject*)x.getData();
    Uint32 size = x.size();

    if (size > 1)
    {
        qsort((void*)data, size, sizeof(CIMInstance), _compareObjects);
    }
}

// Sort array of qualifier decls based on the name
static int _compareQualDecls(const void* p1, const void* p2)
{
    const CIMQualifierDecl* qd1 = (const CIMQualifierDecl*)p1;
    const CIMQualifierDecl* qd2 = (const CIMQualifierDecl*)p2;
    return String::compareNoCase(
        qd1->getName().getString(),
        qd2->getName().getString());
}
static void _Sort(Array<CIMQualifierDecl>& x)
{
    CIMQualifierDecl* data = (CIMQualifierDecl*)x.getData();
    Uint32 size = x.size();

    if (size > 1)
    {
        qsort((void*)data, size, sizeof(CIMQualifierDecl), _compareQualDecls);
    }
}

// Sort array of qualifier decls based on the name
static int _compareParamValues(const void* p1, const void* p2)
{
    const CIMParamValue* pv1 = (const CIMParamValue*)p1;
    const CIMParamValue* pv2 = (const CIMParamValue*)p2;
    return String::compareNoCase(
        pv1->getParameterName(),
        pv2->getParameterName());
}
static void _Sort(Array<CIMParamValue>& x)
{
    CIMParamValue* data = (CIMParamValue*)x.getData();
    Uint32 size = x.size();

    if (size > 1)
    {
        qsort((void*)data, size, sizeof(CIMQualifierDecl), _compareParamValues);
    }
}

//------------------------------------------------------------------------------
//
// mofFormat
//
//------------------------------------------------------------------------------

static void _indent(PEGASUS_STD(ostream)& os,
                    Uint32 level,
                    Uint32 indentSize)
{
    Uint32 n = level * indentSize;
    if (n > 50)
    {
        cout << "Jumped Ship " << level << " size " << indentSize << endl;
        exit(1);
    }

    for (Uint32 i = 0; i < n; i++)
    {
        os << ' ';
    }
}

/* Format the output stream for indented MOF format
*/
void mofFormat(PEGASUS_STD(ostream)& os,
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
    char prevchar = 0;
    while ((c = *tmp++))
    {
        count++;
        // This is too simplistic and must move to a token based mini parser
        // but will do for now. One problem is tokens longer than 12
        // characters that overrun the max line length.
        switch (c)
        {
            case '\n':
                os << c;
                prevchar = c;
                count = 0 + (indent * indentSize);
                _indent(os, indent, indentSize);
                break;

            case '\"':   // quote
                os << c;
                prevchar = c;
                quoteState = !quoteState;
                break;

            case ' ':
                os << c;
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
                os << c;
                prevchar = c;
                break;

            case ']':
                if (qualifierState)
                {
                    if (indent > 0)
                        indent--;
                    qualifierState = false;
                }
                os << c;
                prevchar = c;
                break;

            default:
                os << c;
                prevchar = c;
        }

    }
    delete [] var;
}

/*************************************************************
*
*       Common print functions for all CIM Objects Used
*
***************************************************************/

// output CIMProperty
static void _print(const CIMProperty& property,
    const OutputType format)
{
    if (format == OUTPUT_XML)
    {
        XmlWriter::printPropertyElement(property,cout);
    }
    else if (format == OUTPUT_MOF)
    {
        CIMProperty pt = property.clone();
        pt.setPropagated(false);
        Buffer x;
        MofWriter::appendPropertyElement(false, x, pt);
        mofFormat(cout, x.getData(), 4);
    }
}

// output CIMValue in accord with output format definition
//NOTE: DUPLICATES OUTPUT FORMATT FUNCTION
static void _print(const CIMValue& v,
    const OutputType format)
{
    if (format == OUTPUT_XML)
    {
        XmlWriter::printValueElement(v,cout);
    }
    else if (format == OUTPUT_MOF)
    {
        Buffer x;
        MofWriter::appendValueElement(x, v);
        mofFormat(cout, x.getData(), 4);
    }
    else
    {
        cout << " Format type error" << endl;
    }
}

/*****************************************************************************
*
*       Formatting and print functions for table output of instances
*
******************************************************************************/
//
//  Definition for String entries for each column in the output
//
typedef Array <String> ColumnEntry;

/* Output a single table String entry filling to ColSize or adding an
   eol if last specified
*/
static void _printTableEntry(const String& entryStr,
    const Uint32 colSize,
    Boolean last,
    PEGASUS_STD(ostream)& outPrintWriter)
{
    Uint32 fillerLen = colSize - entryStr.size() + 2;

    outPrintWriter << entryStr;
    if (last)
    {
        outPrintWriter << endl;
    }
    else
    {
        for (Uint32 j = 0; j < fillerLen; j++)
        {
             outPrintWriter << ' ';
        }
    }
}

/* Print the formatted table form of the instances as defined by
   the parameters for the column width for each column and the array
   of column entries (outputTable).
*/
static void _printTables(const Array<Uint32>& maxColumnWidth,
    const Array<ColumnEntry>& outputTable,
    PEGASUS_STD(ostream)& outPrintWriter)
{
    for (Uint32 i = 0; i < outputTable[0].size(); i++)
    {
        for (Uint32 column = 0; column < maxColumnWidth.size(); column++)
        {
            Boolean last = (column == maxColumnWidth.size() - 1);
            _printTableEntry(outputTable[column][i],
                maxColumnWidth[column],
                last,
                outPrintWriter);
        }
    }
}

/* Format the output stream to be a table with column for each property
   and row for the properties in each instance.
*/
static void tableDisplay(PEGASUS_STD(ostream)& outPrintWriter,
    const Array<CIMInstance>& instances)
{
    Array<ColumnEntry> outputTable;
    Array<Uint32> maxColumnWidth;
    Array<String> propertyNameArray;

    // find set of all properties returned for all instances
    for (Uint32 i = 0; i < instances.size(); i++)
    {
        for (Uint32 j = 0; j < instances[i].getPropertyCount(); j++)
        {
            String propertyNameStr =
                instances[i].getProperty(j).getName().getString();

            // Add to outputTable if not already there
            if (!(Contains(propertyNameArray, propertyNameStr)))
            {
                //outputTable.append(propertyNameStr);
                maxColumnWidth.append(propertyNameStr.size());
                propertyNameArray.append(propertyNameStr);
            }
        }
    }

    // Build the complete table output in ascii.  We must build the
    // complete table to determine column widths.
    // NOTE: This code creates tables with column width to match the
    // maximum width of the string representation of the property name or
    // string representation of the value.  This can create REALLY
    // REALLY wide columns for Strings and for Array properties.
    //
    // FUTURE: Add code to create multiline colums for things like array
    // entries or possibly long strings.

    for (Uint32 i = 0; i < propertyNameArray.size(); i++)
    {
        // array for a single column of property values
        Array<String> propertyValueArray;

        String propertyNameStr = propertyNameArray[i];

        // First entry in propertyValueArray array is the propery name
        propertyValueArray.append(propertyNameStr);

        // for all instances get value for the property in
        // propertyNameArray
        for (Uint32 j = 0; j < instances.size(); j++)
        {
            Uint32 pos = instances[j].findProperty(propertyNameStr);

            // Get the value or empty string if there is no property
            // with this name
            String propertyValueStr = (pos != PEG_NOT_FOUND) ?
                    instances[j].getProperty(pos).getValue().toString()
                :
                    String::EMPTY;

            propertyValueArray.append(propertyValueStr);

            if (propertyValueStr.size() > maxColumnWidth[i])
            {
                maxColumnWidth[i] = propertyValueStr.size();
            }
        }

        // Append the value array for this property to the outputTable
        outputTable.append(propertyValueArray);
    }
    _printTables(maxColumnWidth, outputTable, outPrintWriter);
}

/*****************************************************************************
*
*    local Format and output functions called by the public interface
*    functions.  These functions each output a single object of the
*    defined basic type (path, instance, object, class, parameter, etc.)
*
******************************************************************************/
/*
    Output the path.  This includes setting the host component
    to a value determined by the cimcli input paramter --setrthhostname
    if that parameter was in the command line.
*/
static void _outputPath(Options& opts, const CIMObjectPath& path,
                 const String description = "")
{
    CIMObjectPath tmpPath = path;

    // if boolean set and there is a host name, replace it with
    // the substitute name.
    if (opts.setRtnHostNames && (tmpPath.getHost().size() != 0))
    {
        // Show the change made to host name if verbose output mode.
        if (opts.verboseTest)
        {
            cout << "Modify Host name from "
                 <<  path.getHost()
                 << " to "
                 << opts.rtnHostSubstituteName
                 << endl;
        }
        tmpPath.setHost(opts.rtnHostSubstituteName);
    }

    if (description.size() != 0)
    {
        cout << description;
    }
    cout << tmpPath.toString() << endl;
}

/*
    Output for xml and mof formats for instance.  Note that the
    table form has its own functions driven directly from the
    displayInstances because it handles the entire array of instances.
*/
static void _outputFormatInstance(Options& opts,
    CIMInstance& instance)
{
    // display the path element
    _outputPath(opts, instance.getPath(), "path= ");

    // Display the instance based on the format type.
    if (opts.outputType == OUTPUT_XML)
    {
        XmlWriter::printInstanceElement(instance, cout);
    }
    else if (opts.outputType == OUTPUT_MOF)
    {
        CIMInstance temp = instance.clone();
        // Reset the propagated flag to assure that these entities
        // are all shown in the MOF output.
        for (Uint32 i = 0 ; i < temp.getPropertyCount() ; i++)
        {
            CIMProperty p = temp.getProperty(i);
            p.setPropagated(false);
        }

        Buffer x;
        MofWriter::appendInstanceElement(x, temp);
        mofFormat(cout, x.getData(), 4);
    }
}

static void _outputFormatParamValue(Options& opts,
    const CIMParamValue& pv)
{
    if (opts.outputType == OUTPUT_XML)
    {
        XmlWriter::printParamValueElement(pv, cout);
    }
    else if (opts.outputType == OUTPUT_MOF)
    {
        if (!pv.isUninitialized())
        {
           CIMValue v =  pv.getValue();
           CIMType type = v.getType();
           if (pv.isTyped())
               cerr << cimTypeToString (type) << " ";
           else
               cerr << "UnTyped ";

           cerr << pv.getParameterName() << "="
                << v.toString() << endl;
        }
        else
            cerr << "ParamValue not initialized" << endl;
    }
    else
        cerr << "Error, Format Definition Error" << endl;
}

static void _outputFormatClass(Options& opts,
    const CIMClass& myClass)
{
    if (opts.outputType == OUTPUT_XML)
    {
        XmlWriter::printClassElement(myClass, cout);
    }

    else if (opts.outputType == OUTPUT_MOF)
    {
        // Reset the propagated flag to assure that these entities
        // are all shown in the MOF output.

        CIMClass temp = myClass.clone();
        for (Uint32 i = 0 ; i < temp.getPropertyCount() ; i++)
        {
            CIMProperty p = temp.getProperty(i);
            p.setPropagated(false);
        }
        for (Uint32 i = 0 ; i < temp.getMethodCount() ; i++)
        {
            CIMMethod m = temp.getMethod(i);
            m.setPropagated(false);
        }
        Buffer x;
        MofWriter::appendClassElement(x, temp);
        mofFormat(cout, x.getData(), 4);
    }
    else
        cerr << "Error, Format Definition Error" << endl;
}

static void _outputFormatObject(Options& opts,
    CIMObject& myObject)
{

    if (myObject.isClass())
    {
        CIMClass c(myObject);
        _outputFormatClass(opts, c);
    }
    else if (myObject.isInstance())
    {
        CIMInstance i(myObject);
        _outputFormatInstance(opts, i);
    }
    else
    {
        cerr << "Error: Object is neither class or instance" << endl;
    }
}

static void _outputFormatQualifierDecl(const Options& opts,
    const CIMQualifierDecl& myQualifierDecl)
{
    if (opts.outputType == OUTPUT_XML)
    {
        XmlWriter::printQualifierDeclElement(myQualifierDecl, cout);
    }
    else if (opts.outputType == OUTPUT_MOF)
    {
        Buffer x;
        MofWriter::appendQualifierDeclElement(x, myQualifierDecl);
        mofFormat(cout, x.getData(), 4);
    }
    else
    {
        cerr << "Error: Format type error" << endl;
    }
}

// KS_TODO This not used today
static void _outputFormatCIMValue(Options& opts,
    const CIMValue& myValue)
{
    if (opts.outputType == OUTPUT_XML)
    {
        XmlWriter::printValueElement(myValue, cout);
    }
    else if (opts.outputType == OUTPUT_MOF)
    {
        Buffer x;
        MofWriter::appendValueElement(x, myValue);
        mofFormat(cout, x.getData(), 4);
    }
    else
    {
        cerr << "Error: Format type error" << endl;
    }
}


// displaySummary for results of an operation. Display only if count != 0

static void _displayOperationSummary(
    const Options& opts,
    Uint32 count,
    const String& description,
    const String item)
{
    if (count != 0)
    {
        cout << count << " " << description
            << " " << item << " returned.";

        if (opts.repeat > 0)
        {
            cout << " " << opts.repeat;
        }
        if(opts.time && opts.repeat > 0)
        {
            cout << " " << opts.saveElapsedTime;
        }
        cout << endl;
    }
}

/*
    Function to test return count of objects against --count cimcli
    input parameter. If the count test fails, message output and
    termCondition set to count failed status.
    This should be called by all display functions outputting
    multiple entities.
*/
void CIMCLIOutput::testReturnCount(Options& opts,
    Uint32 rcvdCount,
    const String& description)
{
    if (opts.executeCountTest && (opts.expectedCount != rcvdCount))
    {
        cerr << "Failed "
             << description
             << " count test. Expected= " << opts.expectedCount
             << " Received= " << rcvdCount << endl;

        opts.termCondition = CIMCLI_RTN_COUNT_TEST_FAILED;

        // Exit failed immediatly if the count test fails
        exit(opts.termCondition);
    }
}

/******************************************************************************
**
**    The following are the public interfaces for the CIM entity displays
**    Includes displays for CIMInstances, CIMObjects, CIMClasses,
**    CIMObjectPaths, ParameterValues, QualifierDecls, etc.  For most
**    types this includes both the display of a single object and
**    of arrays of that object type.
**    Generally the Array displays allow:
**    1.  Testing against the Options for the --count input parameter
**    2.  Sorting of the array based on the sort option.
**    3.  Display of either summary information or the complete objects
**
******************************************************************************/

void CIMCLIOutput::displayOperationSummary(Options& opts,
    Uint32 count,
    const String& description,
    const String& item)
{
    _displayOperationSummary(opts, count, description, item);

    testReturnCount(opts, count, description);
}

void CIMCLIOutput::displayInstance(Options& opts,
    CIMInstance& instance)
{
    if (opts.summary)
    {
        if (opts.time)
        {
            cout << opts.saveElapsedTime << endl;
        }
    }
    else
    {
        if (opts.outputType == OUTPUT_TABLE)
        {
            Array<CIMInstance> instances;
            instances.append(instance);
            tableDisplay(cout, instances);
        }
        else
        {
            _outputFormatInstance(opts, instance);
        }
    }
}

void CIMCLIOutput::displayInstances(Options& opts,
    Array<CIMInstance>& instances)
{
    String description = "instances of class";
    if (opts.summary)
    {
        _displayOperationSummary(opts, instances.size(), description,
                                 opts.className.getString());
    }
    else
    {
        if (opts.sort)
        {
            _Sort(instances);
        }
        if (instances.size() > 0 && opts.outputType == OUTPUT_TABLE)
        {
            tableDisplay(cout, instances);
            return;
        }

        // Output the returned instances
        for (Uint32 i = 0; i < instances.size(); i++)
        {
            CIMInstance instance = instances[i];
            // Check Output Format to print results
            _outputFormatInstance(opts, instance);
        }
    }
    testReturnCount(opts, instances.size(), description);
}


void CIMCLIOutput::displayPath(Options& opts,
    CIMObjectPath& path,
    const String& description)
{
    if (opts.summary)
    {
        if (opts.time)
        {
            cout << opts.saveElapsedTime << endl;
        }
    }
    else
    {
        if (description.size() != 0)
        {
            cout << description << " ";
        }
        _outputPath(opts,path);
    }
}

void CIMCLIOutput::displayPaths(Options& opts,
    Array<CIMObjectPath>& paths,
    const String& description)
{
    // KS-TODO: Don't show anything if size = 0 and have the caller
    // setup the class somewhere external.
    if (opts.summary)
    {
        _displayOperationSummary(opts, paths.size(), description,
                               opts.className.getString());
    }
    else
    {
        if (opts.sort)
        {
            // Commented code is timer since this could be a long sort
            //Stopwatch x;
            //x.start();
            _Sort(paths);
            //x.stop();
            //cout << "SORT TIME = " << x.getElapsed()
            //     << " for " << paths.size() << " paths." << endl;
        }
        if (description.size() != 0 && paths.size() != 0)
        {
            cout << paths.size() << " " << description << endl;
        }
        //Output the list of paths.
        for (Uint32 i = 0; i < paths.size(); i++)
        {
            _outputPath(opts,paths[i]);
        }
    }
    testReturnCount(opts, paths.size(), description);
}

void CIMCLIOutput::displayClass(Options& opts,
    const CIMClass& cimClass)
{
    if (opts.summary)
    {
        if (opts.time)
        {
            cout << opts.saveElapsedTime << endl;
        }
    }
    else
    {
        _outputFormatClass(opts, cimClass);
    }
}

void CIMCLIOutput::displayClasses(Options& opts,
    Array<CIMClass>& classes)
{
    String description = "classes";
    if (opts.summary)
    {
        _displayOperationSummary(opts, classes.size(), description,
                                 opts.className.getString());
    }
    else
    {
        // Output the returned classes
        if (opts.sort)
        {
            _Sort(classes);
        }
        for (Uint32 i = 0; i < classes.size(); i++)
        {
            CIMClass myClass = classes[i];
            _outputFormatClass(opts, myClass);
        }
    }
    testReturnCount(opts, classes.size(), description);
}
void CIMCLIOutput::displayClassName(const Options& opts,
    const CIMName& className)
{
    cout << className.getString() << endl;
}

void CIMCLIOutput::displayClassNames(Options& opts,
    Array<CIMName>& classNames)
{
    String description = "class names";
    if (opts.summary)
    {
        _displayOperationSummary(opts, classNames.size(), description,
             opts.className.getString());
    }
    else
    {
        if (opts.sort)
        {
            BubbleSort(classNames);
        }
        //Output the list one per line.
        for (Uint32 i = 0; i < classNames.size(); i++)
        {
            displayClassName(opts, classNames[i]);
            //cout << classNames[i].getString() << endl;
        }
    }
    testReturnCount(opts, classNames.size(), description);
}


void CIMCLIOutput::displayObjects(Options& opts,
    Array<CIMObject>& objects,
    const String& description)
{
    if (opts.summary)
    {
      String s = "instances of class";
        _displayOperationSummary(opts, objects.size(), description,
                                  opts.className.getString());
    }
    else
    {
        // Output the returned instances
        for (Uint32 i = 0; i < objects.size(); i++)
        {
            _outputFormatObject(opts, objects[i]);
        }
    }
    testReturnCount(opts, objects.size(), description);
}

void CIMCLIOutput::displayNamespaceNames(Options& opts,
    Array<CIMNamespaceName>& ns,
    const String& description)
{
    if (opts.summary)
    {
        cout << ns.size() << " namespaces " << " returned."
            << endl;
    }
    else
    {
        if (description.size() != 0)
        {
            cout << description;
        }

        for( Uint32 cnt = 0 ; cnt < ns.size(); cnt++ )
        {
            cout << ns[cnt].getString() << endl;;
        }
    }

    // Test for number returned. Only applys if -count option set
    CIMCLIOutput::testReturnCount(opts, ns.size(), "Namespaces");
}

void CIMCLIOutput::displayProperty(Options& opts,
    const CIMProperty& property)
{
    _print(property, opts.outputType);
}

void CIMCLIOutput::displayValue(Options& opts,
    const CIMValue& value)
{
    _print(value, opts.outputType);
}

void CIMCLIOutput::display(Options& opts,
    const String& desription)
{
    if (opts.summary)
    {
        if (opts.time)
        {
            cout << opts.saveElapsedTime << endl;
        }
    }
    else
    {
        cout << desription << endl;
    }
}

void CIMCLIOutput::display(Options& opts,
    const char* desription)
{
    display(opts, String(desription));
}

void CIMCLIOutput::displayParamValues(Options& opts,
    Array<CIMParamValue>& params)
{
    if (opts.sort)
    {
        _Sort(params);
    }
    for (Uint32 i = 0; i < params.size() ; i++)
    {
        _outputFormatParamValue(opts, params[i]);
    }
}

void CIMCLIOutput::displayQualDecl(const Options& opts,
    const CIMQualifierDecl& qualifierDecl)
{
    _outputFormatQualifierDecl(opts, qualifierDecl);
}


void CIMCLIOutput::displayQualDecls(Options& opts,
    Array<CIMQualifierDecl>& qualifierDecls)
{
    if (opts.summary)
    {
        cout << qualifierDecls.size() << " returned." << endl;
    }
    else
    {
        if (opts.sort)
        {
            _Sort(qualifierDecls);
        }
        // Output the returned instances
        for (Uint32 i = 0; i < qualifierDecls.size(); i++)
        {
            CIMQualifierDecl myQualifierDecl = qualifierDecls[i];
            _outputFormatQualifierDecl(opts, myQualifierDecl);
        }
    }
    testReturnCount(opts,
                     qualifierDecls.size(),
                     "Qualifier Decls");
}


PEGASUS_NAMESPACE_END
// END_OF_FILE
