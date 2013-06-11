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

/*
    This file defines the cicml operations(action functions).
    Each function is called from a specific cimcli input parameter opcode.
    The parameters for each operation are defined in the.
    options structure.
*/
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/XmlWriter.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/StringConversion.h>
#include <Pegasus/Common/ArrayInternal.h>

#include <Pegasus/Common/PegasusVersion.h>

#include <Pegasus/General/MofWriter.h>

#include "CIMCLIClient.h"

#include "ObjectBuilder.h"
#include "CIMCLIOutput.h"
#include "CIMCLIHelp.h"
#include "CIMCLIOptions.h"
#include "CIMCLICommon.h"
#include "CIMCLIOperations.h"

PEGASUS_USING_STD;
PEGASUS_NAMESPACE_BEGIN

const String DEFAULT_NAMESPACE = "root/cimv2";


/************************************************************************
*
*   Start and Stop timer to provide execution times for
*   operation action functions.
*
*************************************************************************/

/* Common function for all command action functions to start the
   elapsed timer that will time command execution
*/
void _startCommandTimer(Options& opts)
{
    if (opts.time)
    {
        opts.elapsedTime.reset();
        opts.elapsedTime.start();
    }
}

/* Common function for all command action functions to
   Stop and save the command timer if it was started
*/
void _stopCommandTimer(Options& opts)
{
    if (opts.time)
    {
        opts.elapsedTime.stop();
        opts.saveElapsedTime = opts.elapsedTime.getElapsed();
    }
}

/************************************************************************
*
*   Display functions to support the verbose display of input parameters
*
*************************************************************************/

void _showValueParameters(const Options& opts)
{
    for (Uint32 i = 0; i < opts.valueParams.size(); i++)
    {
        cout << opts.valueParams[i] << " ";
    }
    cout << endl;
}

/*************************************************************
*
*  Functions for interactive selection from the console
*
*************************************************************/
/** Select one item from an array of items presented to
    the user. This prints the list and requests user input for
    the response.
    @param selectList Array<String> list of items from which the
    user has to select one.  Each item should be a printable string.
    @param what String that defines for the output string what type
    of items the select is based on (ex: "Instance Names");
    @return Uint32 representing the item to be selected.
    TODO: Find a way to do a reject.
*/
Uint32 _selectStringItem(const Array<String>& selectList, const String& what)
{
    Uint32 rtn = 0;
    Uint32 listSize = selectList.size();

    for (Uint32 i = 0 ; i < listSize; i++)
    {
        cout << i + 1 << ": " << selectList[i] << endl;
    }

    while (rtn < 1 || rtn > listSize)
    {
        cout << "Select " << what
             << " (1.." << listSize << ")? " << flush;

        // if input is not a valid integer, cin will be set to fail status.
        // and rtn will retain its previous value, so the loop could continue.
        cin >> rtn;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(0x7fffffff, '\n');
        }
    }

    return rtn-1;
}

/** Allow user to select one instance name. Do server
    EnumerateNames for input className and ask user to select a
    singe result from the enumerates returned.
    @param className CIMName for the class to enumerate.
    @param instancePath CIMObjectPath of instance selected
    @return True if at least one instance returned by server.
    Else False and there is nothing in the instancePath

    NOTE: There is no clean way for the user to respond "none of
    the above" to the request to select a single item from the
    list.  They must select one or execute a program kill (ex.
    Ctrl C)
*/
Boolean _selectInstance(Options& opts,
    const CIMName& className,
    CIMObjectPath & instancePath)
{
    // Enumerate instance Names based on input class to get list
    Array<CIMObjectPath> instanceNames =
        opts.client.enumerateInstanceNames(opts.nameSpace,
                                      className);
    // create a corresponding String list
    Array<String> list;
    for (Uint32 i = 0 ; i < instanceNames.size() ; i++)
    {
        list.append(instanceNames[i].toString());
    }

    // return false if nothing in list
    if (list.size() == 0)
    {
        return false;
    }

    // ask user to select a single entry
    Uint32 rtn = _selectStringItem(list, "an Instance");

    instancePath = instanceNames[rtn];

    return true;
}

/** Use the interactive selection mechanism to get the instance if
    the input object is a class AND if opts.interactive flag is
    set.  This function is used by the associator/reference
    functions because just the existence of the object as class
    is insufficient since these functions accept both class and
    instance input for processing. If the tests are passed this
    function calls the server to enumerate the instance names
    possible and displays them for the user to select one.
    @param opts the context structure for this operaiton
    @param instancePath CIMObjectPath of instance selected if return
    is true.  Else, unchanged.
    @return Boolean True if an instance path is to be returned. If nothing
    is selected, returns False.
*/
Boolean _conditionalSelectInstance(Options& opts,
    CIMObjectPath & instancePath)
{
    // if class level and interactive set.
    if ((instancePath.getKeyBindings().size() == 0) && opts.interactive)
    {
        // Ask the user to select an instance. returns instancePath
        // with selected path

        return _selectInstance(opts, opts.getTargetObjectNameClassName(),
                               instancePath);
    }

    return true;
}

/*
    Compare two instances for equality in terms of number and names of
    properties and property values
*/
Boolean _compareInstances(CIMInstance& inst1,
                          CIMInstance& inst2,
                          Boolean verbose,
                          Options& opts)
{
    Boolean returnValue = true;

    //CIMCLIOutput::displayInstance(opts, inst1);
    //CIMCLIOutput::displayInstance(opts, inst2);

    for (Uint32 i = 0 ; i < inst1.getPropertyCount(); i++)
    {
        CIMProperty inst1Property = inst1.getProperty(i);
        CIMName testName = inst1Property.getName();
        //cout << "test property " << testName.getString() << endl;
        Uint32 pos;
        if ((pos = inst2.findProperty(testName)) != PEG_NOT_FOUND)
        {
            CIMProperty inst2Property = inst2.getProperty(pos);

            if (!inst1Property.identical(inst2Property))
            {
                returnValue = false;
                if (verbose)
                {
                    cout << "Error in Property. "<< testName.getString()
                         << "Test Instance Property ";
                    CIMCLIOutput::displayProperty(opts,inst1Property);

                    cout << endl <<"Returned instance Property";

                    CIMCLIOutput::displayProperty(opts,inst2Property);
                    cout << endl;
                }

            }
        }
        else   // Property not found in second instance
        {
            returnValue = false;
            if (verbose)
            {
                cout << "Error: Property " << testName.getString()
                    << "not found in second instance" << endl;
            }
        }

        // If the number of properties not the same in the two instances
        // inst2 must have more than inst1.
        if (inst1.getPropertyCount() != inst2.getPropertyCount())
        {
            returnValue = false;
            if (verbose)
            {
                for (Uint32 i = 0 ; i < inst2.getPropertyCount() ; i++)
                {
                    CIMProperty inst2Property = inst2.getProperty(i);
                    CIMName testName = inst2Property.getName();
                    if (inst1.findProperty(testName) == PEG_NOT_FOUND)
                    {
                        cout << "Error: property " << testName.getString()
                            << " not found in first instance" << endl;
                    }
                }
            }
        }
    }
    return returnValue;
}
/******************************************************************************
//
//  Functions to get the interop namespace and the namespaces in the
//  target cimserver.
//
******************************************************************************/
/*
    Find the most likely candidate for the interop namespace using the class
    CIM_Namespace which should exist in the Interop namespace.  This function
    tests the standard expected inputs and appends namespaces input in the
    nsList input. It returns the namespace found and the instances of the
    CIM_Namespace class in that namespace.
    If the interop namespace found, the instances of CIM_Namespace are
    returned in the instances parameter.
    TODO: Determine a more complete algorithm for determining the
    interop namespace.  Simply the existence of this class may not always
    be sufficient.
*/
Boolean _findInteropNamespace(Options& opts,
                              const Array<CIMNamespaceName> & nsList,
                              Array<CIMInstance>& instances,
                              CIMNamespaceName& nsSelected)
{
    CIMName className = PEGASUS_CLASSNAME_CIMNAMESPACE;
    Array<CIMNamespaceName> interopNs;

    interopNs.appendArray(nsList);
    interopNs.append(PEGASUS_NAMESPACENAME_INTEROP);
    interopNs.append("interop");
    interopNs.append("root/interop");
    Boolean nsFound = false;

    for (Uint32 i = 0 ; i < interopNs.size() ; i++)
    {
        try
        {
            instances = opts.client.enumerateInstances(interopNs[i],
                                                       className);

            nsFound = true;

            if (opts.verboseTest)
            {
                cout << "Found CIM_NamespaceName in namespace "
                    << interopNs[i].getString()
                    << " with " << instances.size() << " instances "
                    << endl;
            }
            nsSelected = interopNs[i];
            break;
        }
        catch(CIMException & e)
        {
            /* If exceptions caught here for all namespaces tested assume that
               target CIMOM does not support CIM_Namespace class.
               Therefore we have to revert to the __namespaces class to
               get namespace information. (Which may only retrun a subset of
               namspaces.
               NOTE: Possible exceptions include namespace does not exist
                     and class does not exist.
            */
            cout << "Info: CIMException return to CIM_NamespaceName enumerate"
                " request. "
                << e.getMessage() << endl;
        }
    }
    return nsFound;
}
/*
    Use the __namespace class to attempt to get namespace names.  Returns
    an array containing namespaces found. Used by _getNameSpaceNames(...)
*/
Array<CIMNamespaceName> _getNameSpacesWith__namespace(Options& opts)
{
    Array<CIMNamespaceName> namespaceNames;
    CIMName nsClassName = CIMName("__namespace");

    // TODO Determine if we really need this statement
    opts.nameSpace = PEGASUS_NAMESPACENAME_INTEROP.getString();

    // Build the namespaces incrementally starting at the root
    // ATTN: 20030319 KS today we start with the "root" directory but
    // this is wrong. We should be
    // starting with null (no directory) but today we get an xml error
    // return in Pegasus
    // returned for this call. Note that the specification requires
    // that the root namespace be used
    // when __namespace is defined but does not require that it be
    // the root for all namespaces. That  is a hole is the spec,
    // not in our code.

    // TODO: Determine why we need the following statement
    namespaceNames.append(opts.nameSpace);

    Uint32 start = 0;
    Uint32 end = namespaceNames.size();

    do
    {
        // for all new elements in the output array
        for (Uint32 range = start; range < end; range ++)
        {
            // Get the next increment in naming for all a name element
            // in the array
            Array<CIMInstance> instances = opts.client.enumerateInstances(
                namespaceNames[range],nsClassName);
            for (Uint32 i = 0 ; i < instances.size(); i++)
            {
                Uint32 pos;
                // if we find the property and it is a string, use it.
                if ((pos = instances[i].findProperty("name"))
                        != PEG_NOT_FOUND)
                {
                    CIMValue value;
                    String namespaceComponent;
                    value = instances[i].getProperty(pos).getValue();
                    if (value.getType() == CIMTYPE_STRING)
                    {
                        value.get(namespaceComponent);

                        String ns = namespaceNames[range].getString();
                        ns.append("/");
                        ns.append(namespaceComponent);
                        namespaceNames.append(ns);
                    }
                }
            }
            start = end;
            end = namespaceNames.size();
        }
    }
    while (start != end);

    return namespaceNames;
}

/*
    List the namespaces in the target host CIMObjectManager.
    This function tries several options to generate a list of the
    namespaces in the target CIMOM including:
    1. Try to list a target Class in the interop namespace.  Note that
    it tries several different guesses to get the target namespace
    2. If a class name is provided as opts.Classname (i.e typically as
    argv2 in the direct call operation, that class is substituted for
    the CIM Namespace class.
    3. If a namespace is provided in opts.namespace (typically through
    the -n input option) that namespace is used as the target namespace
    4. If an asterick "*" is found in opts.namespace, a selection of
    possible namespaces is used including the pegasus default, interop,
    and root/interop.
    5. Finally, if no namespace can be found with the namespace class
    an attempt is made to get the namespace with the __namespace class
    and its incremental descent algorithm.
*/

Array<CIMNamespaceName> _getNameSpaceNames(Options& opts)
{
    //CIMName className = PEGASUS_CLASSNAME_CIMNAMESPACE;

    Array<CIMNamespaceName> namespaceNames;
    Array<CIMInstance> instances;
    Array<CIMNamespaceName> interopNs;

    // if there is a name in the input namespace, use it.
    if (opts.nameSpace != "*" && opts.nameSpace.size() != 0)
    {
        interopNs.append(opts.nameSpace);
    }

    // if a namespace with the CIM_Namespace class is found and instances are
    // returned, we can simply list information from the instances.
    // Assumption: all namespaces containing this class will return the
    // same information.

    CIMNamespaceName interopNamespaceFnd;
    if (_findInteropNamespace(opts, interopNs, instances, interopNamespaceFnd))
    {
        for (Uint32 i = 0 ; i < instances.size(); i++)
        {
            Uint32 pos;
            // If we find the property and it is a string, use it.
            if ((pos = instances[i].findProperty("name")) != PEG_NOT_FOUND)
            {
                CIMValue value;
                String namespaceComponent;
                value = instances[i].getProperty(pos).getValue();
                if (value.getType() == CIMTYPE_STRING)
                {
                    value.get(namespaceComponent);
                    namespaceNames.append(CIMNamespaceName(
                        namespaceComponent));
                }
            }
        }
    }
    else  // No  namespace with CIM_Namespace class found.
    {
        if (opts.verboseTest)
        {
            cout << "Using __namespace class to find namespaces"
                << endl;
        }
        namespaceNames = _getNameSpacesWith__namespace(opts);
    }

    // Validate that all of the returned entities are really namespaces.
    // It is legal for us to have a name component that is really not a
    // namespace (ex. root/fred/john is a namespace  but root/fred is not.
    // There is no clearly defined test for this so we will simply try to
    // get something, in this case a well known assoication

    Array<CIMNamespaceName> rtns;

    for (Uint32 i = 0 ; i < namespaceNames.size() ; i++)
    {
        try
        {
            CIMQualifierDecl cimQualifierDecl;
            cimQualifierDecl = opts.client.getQualifier(namespaceNames[i],
                                           "Association");

            rtns.append(namespaceNames[i]);
        }
        catch(CIMException& e)
        {
            if (e.getCode() != CIM_ERR_INVALID_NAMESPACE)
            {
                rtns.append(namespaceNames[i]);
            }
            else
            {
                cerr << "Warning: " << namespaceNames[i].getString()
                     << " Apparently not a real namespace. Ignored"
                     << endl;
            }
        }
    }
    return rtns;
}

/*
    Determine whether cimcli sets includequalifiers true or false for
    operation based on default (i.e. what is default for this operation)
    and the includeQualifiersRequest, notIncludeQualifiersRequest
    parameters.
    Result put into incudeQualifiers and returned as String for
    display.  This required because some operations have default true
    and other false.
    */
void _resolveIncludeQualifiers(Options& opts, Boolean defaultValue)
{
    // Assure niq and iq not both supplied. They are incompatible
    if (opts.includeQualifiersRequested && opts.notIncludeQualifiersRequested)
    {
        cerr << "Error: -niq and -iq parameters cannot be used together"
             << endl;
        exit(CIMCLI_INPUT_ERR);
    }
    // if default is true (ex class operations), we test for -niq received
    // depend only on the -niq input.
    if (defaultValue)
    {
        opts.includeQualifiers = opts.notIncludeQualifiersRequested ?
                                    false : true;
    }
    else
    {
        opts.includeQualifiers = opts.includeQualifiersRequested ?
                                    true : false;
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//     The following code section defines the action functions             //
//     These functions are a combination of                                //
//     a. the CIM Operations as defined by the DMTF specification          //
//       ex. getInstance implemented for cimcli.                           //
//     b. Other operations such as ns for get namespaces that              //
//        might be useful to CIMOM testers                                 //
//     Input parameters are defined in the                                 //
//     opts structure.  There are no exception catches.                    //
//     exception handling is in the main path.                             //
/////////////////////////////////////////////////////////////////////////////

/*********************** enumerateAllInstanceNames ***************************/

/* This command searches an entire namespace and displays names of
   all instances.
   It is in effect enumerate classes followed by enumerate instances.
   The user may either provide a starting class or not, in which case
   it searches the complete namespace, not simply the defined class.
*/

int enumerateAllInstanceNames(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "EnumerateClasseNames "
            << "Namespace = " << opts.nameSpace
            << ", Class = " << opts.className.getString()
            << ", deepInheritance = " << _toString(opts.deepInheritance)
            << endl;
    }

    CIMName myClassName = CIMName();

    Array<CIMNamespaceName> nsList;
    if (opts.nameSpace != "*")
    {
        nsList.append(opts.nameSpace);
    }
    else
    {
        nsList = _getNameSpaceNames(opts);
    }
    if (opts.verboseTest)
    {
        cout << "Namespaces List for getNamespaces "
             << _toString(nsList)
             << endl;
    }

    CIMName saveClassName = opts.className;
    for (Uint32 i = 0 ; i < nsList.size() ; i++)
    {
        opts.nameSpace = nsList[i].getString();
        Array<CIMName> classNames;

        // The timer really has no meaning for this operation since
        // we merge output and acquisition over multiple operations
        _startCommandTimer(opts);

        // Get all class names in namespace
        opts.className = saveClassName;
        if (opts.verboseTest)
        {
            cout << "EnumerateClassNames for namespace "
                << opts.nameSpace << endl;
        }
        classNames = opts.client.enumerateClassNames(opts.nameSpace,
                                            opts.className,
                                            opts.deepInheritance);

        _stopCommandTimer(opts);

        // Enumerate instance names for all classes returned
        Uint32 totalInstances = 0;
        for (Uint32 iClass = 0; iClass < classNames.size(); iClass++)
        {
            if (opts.verboseTest)
            {
                cout << "EnumerateInstanceNames "
                    << "Namespace = " << opts.nameSpace
                    << ", Class = " << classNames[iClass].getString()
                    << endl;
            }
            Array<CIMObjectPath> instanceNames;
            try
            {
                instanceNames =
                    opts.client.enumerateInstanceNames(opts.nameSpace,
                                                       classNames[iClass]);
            }
            catch(CIMException& e )
            {
                cerr << "Warning: Error in niall for enumerateInstanceNames "
                    << " Namespace = " << opts.nameSpace
                    << " Class = " << classNames[iClass].getString()
                     << ".  " << e.getMessage() << ". Continuing." << endl;
                continue;
            }

            totalInstances += instanceNames.size();

            String s = "instances of class";
            opts.className = classNames[iClass];
            CIMCLIOutput::displayPaths(opts, instanceNames, s);
        }

        cout << "Total Instances in " << opts.nameSpace
             << " = " << totalInstances
             << " which contains " << classNames.size() << " classes"
             << endl;
    }
    return CIMCLI_RTN_CODE_OK;
}


/*********************** enumerateInstanceNames  ***************************/
/*
    This action function executes the client enumerateInstanceNames
    client operation.  Inputs are the namespace and classname
*/
int enumerateInstanceNames(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "EnumerateInstanceNames "
            << "Namespace= " << opts.nameSpace
            << ", Class= " << opts.className.getString()
            << endl;
    }

    _startCommandTimer(opts);

    Array<CIMObjectPath> instanceNames =
        opts.client.enumerateInstanceNames(opts.nameSpace,
                                      opts.className);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayPaths(opts,instanceNames);

    return CIMCLI_RTN_CODE_OK;
}


/************************** enumerateInstances  ***************************/
/*
    This action function executes the enumerateInstances
    client operation. Inputs are the parameters for the CIMCLient call
*/

int enumerateInstances(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, false);

    if (opts.verboseTest)
    {
        cout << "EnumerateInstances "
            << "Namespace = " << opts.nameSpace
            << ", Class = " << opts.className.getString()
            << ", deepInheritance = " << _toString(opts.deepInheritance)
            << ", localOnly = " << _toString(opts.localOnly)
            << ", includeQualifiers = " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin = " << _toString(opts.includeClassOrigin)
            << ", PropertyList = " << _toString(opts.propertyList)
            << endl;
    }

    Array<CIMInstance> instances;

    _startCommandTimer(opts);

    instances = opts.client.enumerateInstances( opts.nameSpace,
                                           opts.className,
                                           opts.deepInheritance,
                                           opts.localOnly,
                                           opts.includeQualifiers,
                                           opts.includeClassOrigin,
                                           opts.propertyList );

    _stopCommandTimer(opts);

    CIMCLIOutput::displayInstances(opts, instances);

    return CIMCLI_RTN_CODE_OK;
}


/************************** executeQuery  ***************************/
/*
    Execute the client ExecQuery function. The parameters are:
    namespace, queryLanguage, and the query string
*/
int execQuery(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "ExecQuery "
            << "Namespace = " << opts.nameSpace
            << ", queryLanguage = " << opts.queryLanguage
            << ", query = " << opts.query
            << endl;
    }

    Array<CIMObject> objects;

    _startCommandTimer(opts);

    objects = opts.client.execQuery(opts.nameSpace,
                                opts.queryLanguage,
                                opts.query );

    _stopCommandTimer(opts);

    String s = "instances of class";
    CIMCLIOutput::displayObjects(opts, objects, s);

    return CIMCLI_RTN_CODE_OK;
}

/* local function to get the object path for the target defined by input.
    The path is built or acquired from information provided by input as
    follows:
    The InstanceName/Class parameter is special in that it has several options:
       - objectPath(Class plus keys) - Use the object path directly
       - Class only (No keys) -  cimcli uses interactive mode to list instances
         of class for selection
       - Class only in objectName plus entries in extra parameters - cimcli
         builds instance from extra parameters and then builds path from
         instance to retrieve.
    This function is used by all of the action functions that require
    cimObjectPath input BUT do not utilize the -i (interactive option) to
    make the decision.
    @return Returns a CIMObjectPath which either contains the path to be used
    or an empty path if there is no path for the operation.
*/

CIMObjectPath _getObjectPath(Options& opts)
{
    // try to build path from input objectName property
    // Uses try block because this input generates an exception based on
    // input syntax and we can use this to more clearly tell the user
    // what the issue is than the text of the standard malformed object
    // exception

    CIMObjectPath thisPath = opts.getTargetObjectName();

    // If there are no keybindings and there are extra input parameters,
    // build path from input arguments. If there are no keybindings
    // and no extra parameters do the select instance.
    if (opts.targetObjectName.getKeyBindings().size() == 0)
    {
        if (opts.valueParams.size() > 1)
        {
            ObjectBuilder ob(
                opts.valueParams,
                opts.client,
                opts.nameSpace,
                opts.targetObjectName.getClassName(),
                CIMPropertyList(),
                opts.verboseTest);

           thisPath = ob.buildCIMObjectPath();
        }
        else  // no extra parameters.
        {
            // get the instance from a console request
            if (!_selectInstance(opts, opts.getTargetObjectNameClassName(),
                                thisPath))
            {
                return CIMObjectPath();
            }
        }
    }
    return thisPath;
}


/************************** deleteInstance  ***************************/
/*
    Execute the client operation deleteInstance with the parameters
    namespace and object or classname.  If only the classname is provided
    an interactive operation is executed and the user is presented with
    a list of instances in the namespace/class from which they can select
    an instance to delete.
*/
int deleteInstance(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "deleteInstance "
            << "Namespace = " << opts.nameSpace
            << ", ObjectName/ClassName = " << opts.getTargetObjectNameStr()
            << endl;
        _showValueParameters(opts);
    }

    // build or get path based in info in opts.  This function returns NULL
    // cimobject path if there is an error.
    CIMObjectPath thisPath = _getObjectPath(opts);

    _startCommandTimer(opts);

    opts.client.deleteInstance(opts.nameSpace, thisPath);

    _stopCommandTimer(opts);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** getInstance  ******************************/
/*  Execute the CIMCLient getInstance function with the parameters provided.
    The majority of the parameters are a direct interpretation of the
    client getInstance input parameters
    The InstanceName/Class parameter is special in that it has several options:
       - objectPath form - Use the object path directly
       - Class only -  cimcli uses interactive mode to list instances of class
         for selection
       - Class only in objectName plus entries in extra parameters - cimcli
         builds instance from extra parameters and then builds path from
         instance to retrieve.
         TODO: Test if properties suppled include all key properties.
*/
int getInstance(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, false);
    if (opts.verboseTest)
    {
        cout << "getInstance "
            << "Namespace = " << opts.nameSpace
            << ", InstanceName/class = " << opts.getTargetObjectNameStr()
            << ", localOnly = " << _toString(opts.localOnly)
            << ", includeQualifiers = " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin = " << _toString(opts.includeClassOrigin)
            << ", PropertyList = " << _toString(opts.propertyList)
            << endl;
        _showValueParameters(opts);
    }

    // build or get path based in info in opts.  This function returns NULL
    // cimobject path if there is an error.
    CIMObjectPath thisPath = _getObjectPath(opts);

    _startCommandTimer(opts);

    CIMInstance cimInstance = opts.client.getInstance(opts.nameSpace,
                                                 thisPath,
                                                 opts.localOnly,
                                                 opts.includeQualifiers,
                                                 opts.includeClassOrigin,
                                                 opts.propertyList);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayInstance(opts, cimInstance);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** createInstance  ******************************/
/****
    This action function executes a create instance.

    The CIM Client operation is:
        CIMObjectPath createInstance(
            const CIMNamespaceName& nameSpace,
            const CIMInstance& newInstance
        );

    The input parameters are the classname and the name/value pairs
    that are used to build properties of the instance.
***/
int createInstance(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "createInstance "
            << "Namespace = " << opts.nameSpace
            << ", ClassName = " << opts.className.getString()
            << endl;
        _showValueParameters(opts);
    }

    ObjectBuilder ob(opts.valueParams,
            opts.client,
            opts.nameSpace,
            opts.className,
            CIMPropertyList(),
            opts.verboseTest);

    // create the instance with the defined properties
    CIMInstance newInstance = ob.buildInstance(
        opts.includeQualifiers,
        opts.includeClassOrigin,
        CIMPropertyList());

    if (opts.verboseTest)
    {
        CIMCLIOutput::displayInstance(opts, newInstance);
    }

    _startCommandTimer(opts);

    CIMObjectPath rtnPath = opts.client.createInstance(opts.nameSpace,
                                                 newInstance);

    _stopCommandTimer(opts);

    // Check Output Format to print results
    String description = "Returned Path ";
    CIMCLIOutput::displayPath(opts, rtnPath, description);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** testInstance  ******************************/
/*  Test the instance defined by the input parameters
    against the same instance in the target system.
    1. Get class from classname input
    2. Build the test instance from input parameters
    3. Build path from input and class
    4. getInstance from system using property list from test instance unless
       there is a list provided with the input.
    5. Compare properties in testInstance against the
       same named properties in returned instance
    6 If there is an error, display differences (if verbose set)
    returns 0 if all properties are the same. Else returns
    CIMCLI_RTN_CODE_ERR_COMPARE_FAILED as an error
    NOTE: Only does exact property compare. Today this function DOES NOT
    have ability to do logical compares such as < >, etc.  Also cannot
    test parameters against an input object name.  MUST BE class name
    on input.
*/
int testInstance(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, false);

    if (opts.verboseTest)
    {
        cout << "testInstance "
            << "Namespace = " << opts.nameSpace
            << ", InstanceName/ClassName = " << opts.getTargetObjectNameStr()
            << ", includeQualifiers = " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin = " << _toString(opts.includeClassOrigin)
            << ", PropertyList = " << _toString(opts.propertyList)
            << endl;
        _showValueParameters(opts);
    }

    // build the instance from all input properties. It is allowable
    // to build an instance with no properties.
    ObjectBuilder ob(
        opts.valueParams,
        opts.client,
        opts.nameSpace,
        opts.getTargetObjectNameClassName(),
        CIMPropertyList(),
        opts.verboseTest);

    CIMInstance testInstance = ob.buildInstance(
        opts.includeQualifiers,
        opts.includeClassOrigin,
        CIMPropertyList());

    // If the objectName keybindings are zero create the path from the
    // built instance unless the interactive bit is set. Then ask the
    // select from existing instances.
    // Else use the path built above from the objectName

    if (opts.targetObjectNameClassOnly())
    {
        if (!_conditionalSelectInstance(opts, opts.targetObjectName) ||
            !opts.interactive)
        {
            CIMClass thisClass =
            opts.client.getClass(opts.nameSpace,
                                 opts.getTargetObjectNameClassName(),
                                 false,true,true,CIMPropertyList());
            opts.targetObjectName = testInstance.buildPath(
                thisClass);
        }
    }

    // If there is no input property list substitute a list created from
    // the test instance. This means we acquire only the properties that were
    // defined on input as part of the test instance.
    if (opts.propertyList.size() == 0)
    {
        opts.propertyList = _buildPropertyList(testInstance);
    }

    _startCommandTimer(opts);

    CIMInstance rtndInstance = opts.client.getInstance(opts.nameSpace,
                                        opts.targetObjectName,
                                        opts.localOnly,
                                        opts.includeQualifiers,
                                        opts.includeClassOrigin,
                                        opts.propertyList);

    // Compare created and returned instances
    if (!_compareInstances(testInstance, rtndInstance, true, opts))
    {
        cerr << "Error:Test Instance differs from Server returned Instance."
            << "Rtn Code " << CIMCLI_RTN_CODE_ERR_COMPARE_FAILED << endl;

        //FUTURE: Create a cleaner display that simply shows the
        //differences not all of the two instances
        if (opts.verboseTest)
        {
            cout << "Test Instance =" << endl;
            CIMCLIOutput::displayInstance(opts, testInstance);
            cout << "Returned Instance =" << endl;
            CIMCLIOutput::displayInstance(opts, rtndInstance);
        }
        return CIMCLI_RTN_CODE_ERR_COMPARE_FAILED;
    }
    else
        cout << "test instance " << opts.targetObjectName.toString()
             << " OK" << endl;

    _stopCommandTimer(opts);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** modifyInstance  ******************************/
/****
    The function executes the CIM Operation modify instance.
    CIMObjectPath modifyInstance(
        const CIMNamespaceName& nameSpace,
        const CIMInstance& modifiedInstance,
        Boolean includeQualifiers = true,
        const CIMPropertyList& propertyList = CIMPropertyList());

    NOTE: We do not support the includequalifiers option so this
    is always set to false.
    This command is similar to create instance but more complex in that
    it is based on an existing instance name and the creation of a
    possibly incomplete instance.

    Therefore, it takes as input an object name which may be just a
    class name and the extra parameters to build an instance.

    This operation differes from the create instance in that the CIM Operation
    input requires a namedInstance rather than simply an instance.  It is the
    name that is used to identify the instance to be modified.  Therefore
    the operation must allow for the name component of the instance to
    be created independently from the input instance

    If the input object name is just a class name, the parameters are used to
    build an instance which MUST include all of the key properties.  Then the
    instance is used to build a path which becomes the path in the
    input instance.

    If the input includes the keys component of a
    cim object path, the logic uses that as the instance name and the
    extra parameters to build the instance.

    Note that there is NO mode in this function to interactively input the
    object path except use of the interactive flag similar to the reference
    and association functions because the extra parameters are used to
    actually build an instance rather than building a path as in the
    getInstance, etc. commands.
***/
int modifyInstance(Options& opts)
{
    // FUTURE - TODO add flag for interactive operation.
    if (opts.verboseTest)
    {
        cout << "modifyInstance "
            << "Namespace = " << opts.nameSpace
            << ", InstanceName/ClassName = " << opts.getTargetObjectNameStr()
            << ", Property List = " <<
                _toString(opts.propertyList)
            << endl;
        _showValueParameters(opts);
    }

    // build the instance from all input properties. It is allowable
    // to build an instance with no properties.
    ObjectBuilder ob(
        opts.valueParams,
        opts.client,
        opts.nameSpace,
        opts.getTargetObjectNameClassName(),
        CIMPropertyList(),
        opts.verboseTest);

    CIMInstance modifiedInstance = ob.buildInstance(
        opts.includeQualifiers,
        opts.includeClassOrigin,
        CIMPropertyList());

    const CIMClass thisClass = ob.getTargetClass();

    // If the objectName keybindings are zero create the path from the
    // built instance unless the interactive bit is set. Then ask the
    // user to select from existing instances.
    // Else use the path built above from the objectName

    if (opts.targetObjectNameClassOnly())
    {
        if (!_conditionalSelectInstance(opts, opts.targetObjectName))
        {
            opts.targetObjectName = modifiedInstance.buildPath(thisClass);
        }
    }
    // put the path into the modifiedInstance
    modifiedInstance.setPath(opts.targetObjectName);

    _startCommandTimer(opts);

    opts.client.modifyInstance(opts.nameSpace,
                         modifiedInstance,
                         false,
                         opts.propertyList);

    // Need to put values into the parameters.
    _stopCommandTimer(opts);

    CIMCLIOutput::display(opts, "modified");

    return CIMCLI_RTN_CODE_OK;
}

/***************************** enumerateClassNames  **************************/
/*
    Execute the client operation enumerateClassNames with the input parameters
    Namespace, ClassName, and the DeepInheritance option.
*/
int enumerateClassNames(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "EnumerateClasseNames "
            << "Namespace= " << opts.nameSpace
            << ", Class= " << opts.className.getString()
            << ", deepInheritance= " << _toString(opts.deepInheritance)
            << endl;
    }
    Array<CIMName> classNames;

    _startCommandTimer(opts);

    classNames = opts.client.enumerateClassNames(opts.nameSpace,
                                        opts.className,
                                        opts.deepInheritance);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayClassNames(opts, classNames);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** enumerateClasses  ******************************/
/*
    Execute the client operation enumerateClasses with the input parameters
    Namespace, ClassName, and the DeepInheritance localOnly,
    includeQualifiers, and includeClassOrigin option.
*/
int enumerateClasses(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, true);
    if (opts.verboseTest)
    {
        cout << "EnumerateClasses "
            << "Namespace= " << opts.nameSpace
            << ", Class= " << opts.className.getString()
            << ", deepInheritance= " << _toString(opts.deepInheritance)
            << ", localOnly= " << _toString(opts.localOnly)
            << ", includeQualifiers= " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin= " << _toString(opts.includeClassOrigin)
            << endl;
    }

    _startCommandTimer(opts);

    Array<CIMClass> classes = opts.client.enumerateClasses(opts.nameSpace,
                                        opts.className,
                                        opts.deepInheritance,
                                        opts.localOnly,
                                        opts.includeQualifiers,
                                        opts.includeClassOrigin);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayClasses(opts, classes);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** deleteClass  ******************************/
/*
    Execute the client operation deleteClass with the input parameters
    Namespace and ClassName.
*/
int deleteClass(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "deleteClasses "
            << "Namespace = " << opts.nameSpace
            << ", Class = " << opts.className.getString()
            << endl;
    }

    _startCommandTimer(opts);

    opts.client.deleteClass(opts.nameSpace, opts.className);

    _stopCommandTimer(opts);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** getClass  ******************************/
/*
    Execute the client operation getClass with the input parameters
    Namespace and ClassName and the options deepInheritance, localOnly,
    includeQualifiers, includeClassOrigin, and a possible propertyList
*/
int getClass(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, true);

    if (opts.verboseTest)
    {
        cout << "getClass "
            << "Namespace= " << opts.nameSpace
            << ", Class= " << opts.className.getString()
            << ", deepInheritance= " << _toString(opts.deepInheritance)
            << ", localOnly= " << _toString(opts.localOnly)
            << ", includeQualifiers= " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin= " << _toString(opts.includeClassOrigin)
            << ", PropertyList= " << _toString(opts.propertyList)
            << endl;
    }

    _startCommandTimer(opts);

    CIMClass cimClass = opts.client.getClass(opts.nameSpace,
                                        opts.className,
                                        opts.localOnly,
                                        opts.includeQualifiers,
                                        opts.includeClassOrigin,
                                        opts.propertyList);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayClass(opts, cimClass);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** getProperty  ******************************/
/*
    Execute the client operation getProperty with the input parameters
    Namespace, InstanceName, and propertyName
*/
int getProperty(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "getProperty "
            << "Namespace= " << opts.nameSpace
            << ", InstanceName= " << opts.getTargetObjectNameStr()
            << ", propertyName= " << opts.propertyName
            << endl;
        _showValueParameters(opts);
    }

    // build or get path based in info in opts.  This function returns NULL
    // cimobject path if there is an error.
    CIMObjectPath thisPath = _getObjectPath(opts);

    CIMValue cimValue;

    _startCommandTimer(opts);

    cimValue = opts.client.getProperty( opts.nameSpace,
                                   thisPath,
                                   opts.propertyName);
    _stopCommandTimer(opts);

    if (opts.summary)
    {
        if (opts.time)
        {
            cout << opts.saveElapsedTime << endl;
        }
    }
    else
    {
        cout << opts.propertyName << " = " << cimValue.toString() << endl;
    }

    return CIMCLI_RTN_CODE_OK;
}

/***************************** setProperty  ******************************/
/*
    Execute the client operation setProperty with the input parameters
    Namespace, InstanceName, propertyName and new property value
*/
int setProperty(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "setProperty "
            << "Namespace= " << opts.nameSpace
            << ", InstanceName= " << opts.getTargetObjectNameStr()
            << ", propertyName= " << opts.propertyName
            << ", newValue= " << opts.newValue.toString()
            << endl;
        _showValueParameters(opts);
    }

    // build or get path based in info in opts.  This function returns NULL
    // cimobject path if there is an error.
    CIMObjectPath thisPath = _getObjectPath(opts);

    _startCommandTimer(opts);

    opts.client.setProperty( opts.nameSpace,
                                   thisPath,
                                   opts.propertyName,
                                   opts.newValue);

    _stopCommandTimer(opts);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** getQualifier  ******************************/
/*
    Execute the client operation getQualifier with the input parameters
    Namespace and qualifierName.
*/
int getQualifier(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "getQualifier "
            << "Namespace= " << opts.nameSpace
            << ", Qualifier= " << opts.qualifierName
            << endl;
    }

    CIMQualifierDecl cimQualifierDecl;

    _startCommandTimer(opts);

    cimQualifierDecl = opts.client.getQualifier( opts.nameSpace,
                                   opts.qualifierName);

    _stopCommandTimer(opts);

    // display received qualifier

    CIMCLIOutput::displayQualDecl(opts, cimQualifierDecl);

    return CIMCLI_RTN_CODE_OK;
}

int setQualifier(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "setQualifiers "
            << "Namespace= " << opts.nameSpace
            // KS add the qualifier decl here.
            << endl;
    }

    _startCommandTimer(opts);

    opts.client.setQualifier(opts.nameSpace, opts.qualifierDeclaration);

    _stopCommandTimer(opts);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** deleteQualifier  ******************************/
/*
    Execute the client operation deleteQualifier with the input parameters
    Namespace and qualifierName.
*/
int deleteQualifier(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "deleteQualifiers "
            << "Namespace= " << opts.nameSpace
            << " QualifierName= " << opts.qualifierName
            << endl;
    }

    _startCommandTimer(opts);

    opts.client.deleteQualifier(opts.nameSpace, opts.qualifierName);

    _stopCommandTimer(opts);

    return CIMCLI_RTN_CODE_OK;
}

/***************************** enumerateQualifiers  **************************/
/*
    Execute the client operation enumerateQualifiers with the input parameters
    Namespace.
*/
int enumerateQualifiers(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "enumerateQualifiers "
            << "Namespace= " << opts.nameSpace
            << endl;
    }

    Array<CIMQualifierDecl> qualifierDecls;

    _startCommandTimer(opts);

    qualifierDecls = opts.client.enumerateQualifiers(opts.nameSpace);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayQualDecls(opts, qualifierDecls);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** referenceNames  ******************************/
/*
    Execute CIM Operation referencenames.  The signature of the
    client CIM Operation is:

    Array<CIMObjectPath> referenceNames(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY
*/
int referenceNames(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "ReferenceNames "
            << "Namespace= " << opts.nameSpace
            << ", ObjectPath= " << opts.getTargetObjectNameStr()
            << ", resultClass= " << opts.resultClass.getString()
            << ", role= " << opts.role
            << endl;
    }
    // do conditional select of instance if params properly set.

    CIMObjectPath thisObjectPath(opts.getTargetObjectName());

    if (!_conditionalSelectInstance(opts, thisObjectPath))
    {
        return CIMCLI_RTN_CODE_OK;
    }

    _startCommandTimer(opts);

    Array<CIMObjectPath> referenceNames =
        opts.client.referenceNames( opts.nameSpace,
                               thisObjectPath,
                               opts.resultClass,
                               opts.role);

    _stopCommandTimer(opts);

    String s = "referenceNames";
    opts.className = thisObjectPath.getClassName();
    CIMCLIOutput::displayPaths(opts, referenceNames, s);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** references  ******************************/
/****
  get references for the target input object using the client references
  operation. This operation uses the interactive option to determine if
  the input target is to be selected interactively.  This is required
  because the reference operation can have either a class or path as
  input (returns either classes or instances depending on this input).
  The interactive option requests that the instance interactive selector
  be used to allow the user to select instances for a particular class
  input.
     Array<CIMObject> references(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY,
        Boolean includeQualifiers = false,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList()
    );
*/
int references(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, false);
    if (opts.verboseTest)
    {
        cout << "References "
            << "Namespace= " << opts.nameSpace
            << ", ObjectName = " << opts.getTargetObjectNameStr()
            << ", resultClass= " << opts.resultClass.getString()
            << ", role= " << opts.role
            << ", includeQualifiers= " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin= " << _toString(opts.includeClassOrigin)
            << ", CIMPropertyList= " << _toString(opts.propertyList)
            << endl;
    }

    // do conditional select of instance if params properly set.
    CIMObjectPath thisObjectPath(opts.getTargetObjectName());

    if (!_conditionalSelectInstance(opts, thisObjectPath))
    {
        return CIMCLI_RTN_CODE_OK;
    }

    _startCommandTimer(opts);

    Array<CIMObject> objects =
        opts.client.references(  opts.nameSpace,
                            thisObjectPath,
                            opts.resultClass,
                            opts.role,
                            opts.includeQualifiers,
                            opts.includeClassOrigin,
                            opts.propertyList);

    _stopCommandTimer(opts);

    String s = "references";
    CIMCLIOutput::displayObjects(opts,objects,s);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** associatorNames  ******************************/
/*
    Uaw the client associatorNames operation to return associated classes
    or instances for the target inputs. Note that this operation uses the
    interactive option.

    Array<CIMObjectPath> associatorNames(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& assocClass = CIMName(),
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY,
        const String& resultRole = String::EMPTY
    );

*/
int associatorNames(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "associatorNames "
            << "Namespace= " << opts.nameSpace
            << ", ObjectName= " << opts.getTargetObjectNameStr()
            << ", assocClass= " << opts.assocClass.getString()
            << ", resultClass= " << opts.resultClass.getString()
            << ", role= " << opts.role
            << ", resultRole= " << opts.resultRole
            << endl;
    }

    // do conditional select of instance if params properly set.
    CIMObjectPath thisObjectPath(opts.getTargetObjectName());

    if (!_conditionalSelectInstance(opts, thisObjectPath))
    {
        return CIMCLI_RTN_CODE_OK;
    }

    _startCommandTimer(opts);

    Array<CIMObjectPath> associatorNames =
        opts.client.associatorNames( opts.nameSpace,
                                    thisObjectPath,
                                    opts.assocClass,
                                    opts.resultClass,
                                    opts.role,
                                    opts.resultRole);

    _stopCommandTimer(opts);

    String s = "associator names";
    opts.className = thisObjectPath.getClassName();
    CIMCLIOutput::displayPaths(opts, associatorNames, s);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** associators  ******************************/
/*
    Execute the CIM Client Operation associators. Note that this function
    uses the interactive operation to allow the user to select the
    object to be the target objecName. The signature of
    the function is:

    Array<CIMObject> associators(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& assocClass = CIMName(),
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY,
        const String& resultRole = String::EMPTY,
        Boolean includeQualifiers = false,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList()
    );
 */
int associators(Options& opts)
{
    // Resolve the IncludeQualifiers -iq vs -niq qualifiers default is true.
    _resolveIncludeQualifiers(opts, false);

    if (opts.verboseTest)
    {
        cout << "Associators "
            << "Namespace= " << opts.nameSpace
            << ", Object= " << opts.getTargetObjectNameStr()
            << ", assocClass= " << opts.assocClass.getString()
            << ", resultClass= " << opts.resultClass.getString()
            << ", role= " << opts.role
            << ", resultRole= " << opts.resultRole
            << ", includeQualifiers= " << _toString(opts.includeQualifiers)
            << ", includeClassOrigin= " << _toString(opts.includeClassOrigin)
            << ", propertyList= " << _toString(opts.propertyList)
            << endl;
    }

    // do conditional select of instance if params properly set.
    CIMObjectPath thisObjectPath(opts.getTargetObjectName());

    if (!_conditionalSelectInstance(opts, thisObjectPath))
    {
        return CIMCLI_RTN_CODE_OK;
    }

    _startCommandTimer(opts);

    Array<CIMObject> objects =
        opts.client.associators( opts.nameSpace,
                            thisObjectPath,
                            opts.assocClass,
                            opts.resultClass,
                            opts.role,
                            opts.resultRole,
                            opts.includeQualifiers,
                            opts.includeClassOrigin,
                            opts.propertyList);

    _stopCommandTimer(opts);

    String s = "associators";
    CIMCLIOutput::displayObjects(opts,objects,s);

    return CIMCLI_RTN_CODE_OK;
}


/***************************** invokeMethod  ******************************/
/*
    CIMValue invokeMethod(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& instanceName,
        const CIMName& methodName,
        const Array<CIMParamValue>& inParameters,
        Array<CIMParamValue>& outParameters
*/


/***************************** invokeMethod  ******************************/
 int invokeMethod(Options& opts)
 {
     {
         if (opts.verboseTest)
         {
             cout << "invokeMethod"
                 << " Namespace= " << opts.nameSpace
                 << ", ObjectName= " << opts.getTargetObjectNameStr()
                 << ", methodName= " << opts.methodName.getString()
                 << ", inParams Count= " << opts.inParams.size()
                 << endl;

             CIMCLIOutput::displayParamValues(opts, opts.inParams);

             _showValueParameters(opts);
        }

        ObjectBuilder ob(
            opts.valueParams,
            opts.client,
            opts.nameSpace,
            opts.getTargetObjectNameClassName(),
            CIMPropertyList(),
            opts.verboseTest);

        ob.setMethod(opts.methodName);

        Array<CIMParamValue> params = ob.buildMethodParameters();

         // Create array for output parameters
        CIMValue retValue;
        Array<CIMParamValue> outParams;

        _startCommandTimer(opts);

        // Call invoke method with the parameters
        retValue = opts.client.invokeMethod(opts.nameSpace,
                                            opts.getTargetObjectName(),
                                            opts.methodName,
                                            params,
                                            outParams);

        _stopCommandTimer(opts);

        // Display the return value CIMValue
        cout << "Return Value= ";
        if (opts.outputType == OUTPUT_XML)
        {
            XmlWriter::printValueElement(retValue, cout);
        }
        else
        {
            cout << retValue.toString() << endl;
        }

        // Display any outparms

        CIMCLIOutput::displayParamValues(opts, outParams);
     }

    return CIMCLI_RTN_CODE_OK;
 }

/************************ enumerateNamespace names **********************/
/* Enumerate the Namespace names.  This function is based on using either
    the CIM_Namespace class or if this does not exist the
    __Namespace class and either returns all namespaces or simply the ones
    starting at the namespace input as the namespace variable.
    It assumes that the input classname is __Namespace.
*/

int enumerateNamespaceNames(Options& opts)
{
    if (opts.verboseTest)
    {
        cout << "EnumerateNamespaces "
            << "Namespace= " << opts.nameSpace
            << ", Class= " << opts.className.getString()
            << endl;
    }

    _startCommandTimer(opts);

    Array<CIMNamespaceName> ns = _getNameSpaceNames(opts);

    _stopCommandTimer(opts);

    CIMCLIOutput::displayNamespaceNames(opts, ns);

    return CIMCLI_RTN_CODE_OK;
}


/************************ setObjectManagerStatistics **********************/
/*
    Set the statistics on/off flag in the objectmanager Class.  This should
    be considered temporary code pending a more general solution for
    setting many of these attributes. Do not count on this being in
    future versions of cimcli.
    DEPRECATED - This should be replaced with a special function but since
    the whole use of the statistics setting functions is in question in
    the DMTF we left it for now.
*/
int setObjectManagerStatistics(Options& opts, Boolean newState,
                                   Boolean& stateAfterMod)
{
    CIMName gathStatName ("GatherStatisticalData");

    Array<CIMInstance> instancesObjectManager;
    CIMInstance instObjectManager;
    Uint32 prop_num;
    Array<CIMName> plA;
    plA.append(gathStatName);
    CIMPropertyList statPropertyList(plA);

    // Create property list that represents correct request
    // get instance.  Get only the gatherstatitistics property
    instancesObjectManager  =
        opts.client.enumerateInstances(PEGASUS_NAMESPACENAME_INTEROP,
            "CIM_ObjectManager",
            true, false, false, false, statPropertyList);
    PEGASUS_TEST_ASSERT(instancesObjectManager.size() == 1);
    instObjectManager = instancesObjectManager[0];

    // set correct path into instance
    instObjectManager.setPath(instancesObjectManager[0].getPath());

    prop_num = instObjectManager.findProperty(gathStatName);
    PEGASUS_TEST_ASSERT(prop_num != PEG_NOT_FOUND);

    instObjectManager.getProperty(prop_num).setValue(CIMValue(newState));

    opts.client.modifyInstance(PEGASUS_NAMESPACENAME_INTEROP, instObjectManager,
         false, statPropertyList);

    // get updated instance to confirm change made
    CIMInstance updatedInstance =
        opts.client.getInstance(PEGASUS_NAMESPACENAME_INTEROP,
        instObjectManager.getPath(),
        false, false, false, statPropertyList);

    prop_num = updatedInstance.findProperty(gathStatName);
    PEGASUS_TEST_ASSERT(prop_num != PEG_NOT_FOUND);
    CIMProperty p = updatedInstance.getProperty(prop_num);
    CIMValue v = p.getValue();
    v.get(stateAfterMod);

    cout << "Updated Status= " << ((stateAfterMod)? "true" : "false") << endl;

    if (stateAfterMod != newState)
    {
        cerr << "Error: State change error. Expected: "
            << ((newState)? "true" : "false")
            << " Rcvd: " << ((stateAfterMod)? "true" : "false") << endl;
    }

    return CIMCLI_RTN_CODE_OK;
}

PEGASUS_NAMESPACE_END
// END_OF_FILE

