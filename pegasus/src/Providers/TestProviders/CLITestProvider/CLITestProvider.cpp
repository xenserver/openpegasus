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
Test Provider for cimcli. This provider is intended to be used as a
test driver only for cimcli.

This provider is based on the Test_CLITestProviderClass class and the
Test_CLITestProviderLinkClass. The Test_CLITestProviderClass class
includes all of the CIM Data types in both scalar and array form
and provides the following operations:

1. Initialize - Creates a single instance of the class with all of the
properties initialized. this is placed in an array.

2. EnumerateInstances - Returns all instances in the instances array

3. GetInstance - Returns the single instance found in the array with the
input object path if found

4. Create instance - Creates a new instance of the target class and puts it
into the array

5. ModifyInstance - Attempts to modify an instance of the target class if one
is found in the instances array.

6. DeleteInstance - Delete an instance of the test class if found in the
instances array.

6. Invoke method: Includes several methods as follows:
   InOutParamTest - Returns input parameters to the caller
   setProviderParameters - Single method to allow setting parameters that
   would be used to modify the provider capabilities for testing.
   Parameters:
      substituteHostName - String parameter that provides an alternate name
      to be used as host name on all responses that include host name.
   resetProviderParameters - resets all parameters set by the
   setProviderParamters method.

The Test_CLITestProviderLinkClassProvides a means to test the reference and
associator operations.  Note that the instances do not really represent
understandable associations, simply syntatically correct associations.

LIMITATIONS: The provider is intended to be used in a single namespace and so
does not include the namespace in the instances placed in the array. Therefore
if it is enabled for multiple namespaces, a user from some other namespace could
remove, get, or enumerate an instance created in another namespace.

It is intended to run a set of tests fairly rapidly so does not hold itself in
memory.  Therefore, if instances are put into the array after the normal Pegasus
timeout of providers for unload, they will be discarded.
*/

#include "CLITestProvider.h"
#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/Tracer.h>
// Required because of some malignent link betweeh MessageLoader and Thread.h
#include <Pegasus/Common/MessageLoader.h>
#include <Pegasus/Common/Thread.h>
#include <Pegasus/Common/Mutex.h>

PEGASUS_USING_STD;
PEGASUS_USING_PEGASUS;

String _toString(Boolean x)
{
    return((x)?"true" : "false");
}

static String _toString(const CIMPropertyList& pl)
{
    String s;

    if (pl.isNull())
    {
        s = "NULL";
    }
    else if (pl.size() == 0)
    {
        s = "EMPTY";
    }
    else
    {
        for (Uint32 i = 0; i < pl.size(); i++)
        {
            if (i > 0)
            {
                s.append(", ");
            }
            s.append(pl[i].getString());
        }
    }
    return s;
}
/*
    Add the name value pair to the String target.  The result is the pair
    added to target in the form
    [; ]name "=" value
*/
void _addParam(String& target, const String& name, const String& value)
{
    if (target.size() != 0)
    {
        target.append("; ");
    }
    target.append(name);
    target.append("=");
    target.append(value);
}/*
    Complete the host and namespace fields of an object path if there are
    empty.
*/
void _completePath(const String& hostName,
                            const CIMNamespaceName& nameSpace,
                            CIMObjectPath& objectPath)
{
    if (objectPath.getHost().size() == 0)
        objectPath.setHost(hostName);

    if (objectPath.getNameSpace().isNull())
        objectPath.setNameSpace(nameSpace);
}

// convert a fully qualified reference into a local reference
// (class name and keys only).
CIMObjectPath _localPath(const CIMObjectPath& inputPath)
{
    CIMObjectPath localPath = CIMObjectPath(
        String(),
        CIMNamespaceName(),
        inputPath.getClassName(),
        inputPath.getKeyBindings());
    return localPath;
}
// Gets a host name, either the real host name or a substitute for testing.
// The substitute name  option allows CLI test program to get a dependable
// host name for result comparisons.
String CLITestProvider::_getHostName()
{
    if (!_useSubstituteHostName)
    {
        return System::getHostName();
    }
    else
    {
        return _substituteHostName;
    }
}

// Serializes access to the instances array during the CIM requests
static Mutex instanceArrayMutex;

CLITestProvider::CLITestProvider()
{
    _initialized = false;
}

CLITestProvider::~CLITestProvider()
{
}

void CLITestProvider::initialize(CIMOMHandle & cimom)
{
    _cimom = cimom;
}

void CLITestProvider::terminate()
{
    delete this;
}

void CLITestProvider::invokeMethod(
    const OperationContext & context,
    const CIMObjectPath & objectReference,
    const CIMName & methodName,
    const Array<CIMParamValue> & inParameters,
    MethodResultResponseHandler & handler)
{
    initializeProvider(objectReference.getNameSpace());

    // convert a fully qualified reference into a local reference
    // (class name and keys only).

    CIMObjectPath localReference = CIMObjectPath(
        String(),
        CIMNamespaceName(),
        objectReference.getClassName(),
        objectReference.getKeyBindings());

    handler.processing();

    String outString = "CLITestProvider  Tests : ";

    if (objectReference.getClassName().equal("Test_CLITestProviderClass"))
    {
        if (methodName.equal("ReferenceParamTest"))
        {
            if (inParameters.size() > 0)
            {
                for(Uint32 i = 0; i < inParameters.size(); ++i)
                {
                    CIMValue paramVal = inParameters[i].getValue();
                    if (!paramVal.isNull())
                    {
                        if(paramVal.getType() == CIMTYPE_REFERENCE)
                        {
                            CIMObjectPath cop,cop1(
                                "test/Testprovider:class.k1="
                                "\"v1\",k2=\"v2\",k3=\"v3\"");
                            paramVal.get(cop);
                            PEGASUS_TEST_ASSERT(cop.identical(cop1) == true);
                            outString.append(
                                "\n Passed Reference params Test1 ");
                            PEGASUS_TEST_ASSERT(!cop.identical(cop1) == false);
                            outString.append(
                                "\n Passed Reference params Test2    ");
                        }
                        else
                        {
                            //This code gets excuted for non reference
                            //parameters.
                            String replyName;
                            paramVal.get(replyName);
                            if (replyName != String::EMPTY)
                            {
                                outString.append(replyName);
                                outString.append("\n");
                                outString.append("Passed String Param Test\n");
                            }
                        }
                        outString.append("\n");
                    }
                    else
                    {
                        outString.append("Param Value is NULL");
                    }
                }

                handler.deliver(CIMValue(outString));
            }
            else
            {
                outString.append("Empty Parameters");
                handler.deliver(CIMValue(outString));
            }
        }

        // This simply returns all parameters and
        // sets return value set to zero. This should provide a complete
        // test of all input and output parameter types for cimcli
        else if(methodName.equal("InOutParamTest"))
        {
            if (inParameters.size() > 0)
            {
                //Returns all input parameters
                handler.deliverParamValue(inParameters);
            }
        handler.deliver(Uint32(0));
        }

        // This simply returns all parameters and
        // sets return value set to zero. This should provide a complete
        // test of all input and output parameter types for cimcli
        else if(methodName.equal("setProviderParameters"))
        {
            Uint32 rtnCode = 0;
            if (inParameters.size() > 0)
            {
                for(Uint32 i = 0; i < inParameters.size(); ++i)
                {
                    String paramName = inParameters[i].getParameterName();
                    CIMValue paramVal = inParameters[i].getValue();

                    if (paramName == "substituteHostName")
                    {
                        if (paramVal.getType() == CIMTYPE_STRING)
                        {
                            String sTmp = _substituteHostName;
                            // set the capability from the input parameter
                            paramVal.get(_substituteHostName);
                            _useSubstituteHostName = true;

                            // return the parameter with the original value
                            CIMParamValue outParam = inParameters[i].clone();
                            CIMValue v = outParam.getValue();

                            v.set(sTmp);
                            outParam.setValue(v);

                            handler.deliverParamValue(outParam);
                        }
                        else
                        {
                            // return error. Incorrect type on parameter
                            rtnCode = 1;
                        }
                    }
                    // NOTE: Add new parameters here with else if
                    // not a valid parameter. Return error
                    else
                    {
                        rtnCode = 1;
                    }
                }
            }
            handler.deliver(rtnCode);
        }
        else if(methodName.equal("resetProviderParameters"))
        {
            _useSubstituteHostName = false;
            _substituteHostName = String();
            handler.deliver(0);
        }
    }
    handler.complete();
}

void CLITestProvider::getInstance(
    const OperationContext & context,
    const CIMObjectPath & instanceReference,
    const Boolean includeQualifiers,
    const Boolean includeClassOrigin,
    const CIMPropertyList & propertyList,
    InstanceResponseHandler & handler)
{
    initializeProvider(instanceReference.getNameSpace());

    handler.processing();

    AutoMutex autoMut(instanceArrayMutex);

    Uint32 index;
    if ((index = findInstance(instanceReference)) != PEG_NOT_FOUND)
    {
        // Put input parameters into the requestInputParameters property so
        // they can be tested on by the client.
        String text;
        _addParam(text, "propertyList", _toString(propertyList));
        _addParam(text, "includeQualifiers", _toString(includeQualifiers));
        _addParam(text, "includeClassOrigin", _toString(includeClassOrigin));

        // clone and filter the returned instance. Clone so the original
        // not modified by filter.
        try
        {
            CIMInstance temp = _instances[index].clone();
            // Required because not assured all instances will have this
            // property
            if (temp.findProperty("requestInputParameters") == PEG_NOT_FOUND)
            {
                CIMClass tmpClass = _getClass(CIMName(
                    "Test_CLITestProviderClass"),
                    instanceReference.getNameSpace());

                Uint32 pos = tmpClass.findProperty("requestInputParameters");

                temp.addProperty(tmpClass.getProperty(pos));
            }

            temp.getProperty(temp.findProperty("requestInputParameters"))
                .setValue(text);

            temp.filter(includeQualifiers,includeClassOrigin, propertyList);
            handler.deliver(temp);
        }
        catch(CIMException& e)
        {
            cerr << "CIMCLITestProvider: Exception Occured on deliver : "
                << e.getMessage() << endl;
            throw CIMException(e);
        }
    }
    else
    {
        throw CIMException(CIM_ERR_NOT_FOUND);
    }

    handler.complete();
}

void CLITestProvider::enumerateInstances(
    const OperationContext & context,
    const CIMObjectPath & ref,
    const Boolean includeQualifiers,
    const Boolean includeClassOrigin,
    const CIMPropertyList & propertyList,
    InstanceResponseHandler & handler)
{
    initializeProvider(ref.getNameSpace());

    handler.processing();

    AutoMutex autoMut(instanceArrayMutex);

    CIMName reqClassName = ref.getClassName();

    // Puts input parameters into the requestInputParameters property so
    // they can be tested on by the client.
    String text;
    _addParam(text, "propertyList", _toString(propertyList));
    _addParam(text, "includeQualifiers", _toString(includeQualifiers));
    _addParam(text, "includeClassOrigin", _toString(includeClassOrigin));

    for (Uint32 i = 0, n = _instances.size(); i < n; i++)
    {
        if (reqClassName == _instances[i].getClassName())
        {
            try
            {
                CIMInstance temp = _instances[i].clone();
                if (temp.findProperty(
                    "requestInputParameters") == PEG_NOT_FOUND)
                {
                    CIMClass tmpClass = _getClass(CIMName(
                        "Test_CLITestProviderClass"),
                        ref.getNameSpace());

                    Uint32 pos = tmpClass.findProperty(
                        "requestInputParameters");

                    temp.addProperty(tmpClass.getProperty(pos));
                }

                temp.getProperty(
                    temp.findProperty("requestInputParameters"))
                    .setValue(text);

                temp.filter(includeQualifiers,includeClassOrigin,
                            propertyList);
                handler.deliver(temp);
            }
            catch(CIMException& e)
            {
                cerr << "CIMCLITestProvider:Exception Occured : "
                    << e.getMessage() << endl;
                throw CIMException(e);
            }
        }
    }
    handler.complete();
}

void CLITestProvider::enumerateInstanceNames(
    const OperationContext & context,
    const CIMObjectPath & classReference,
    ObjectPathResponseHandler & handler)
{
    initializeProvider(classReference.getNameSpace());

    handler.processing();

    AutoMutex autoMut(instanceArrayMutex);

    CIMName reqClassName = classReference.getClassName();

    for (Uint32 i = 0, n = _instances.size(); i < n; i++)
    {
        if (reqClassName == _instances[i].getClassName())
        {
            try
            {
                handler.deliver(_instances[i].getPath());
            }
            catch(CIMException& e)
            {
                cerr << "CIMCLITestProvider:Exception Occured : "
                    << e.getMessage() << endl;
                throw CIMException(e);
            }
        }
    }
    handler.complete();
}


void CLITestProvider::modifyInstance(
    const OperationContext & context,
    const CIMObjectPath & instanceReference,
    const CIMInstance & instanceObject,
    const Boolean includeQualifiers,
    const CIMPropertyList & propertyList,
    ResponseHandler & handler)
{
    initializeProvider(instanceReference.getNameSpace());

    handler.processing();

    // convert a fully qualified reference into a local reference
    // (class name and keys only).

    CIMClass mc = _getClass(instanceReference.getClassName(),
                            instanceReference.getNameSpace());

    // TODO confirm the correctness of allowing either the input path or
    //      the build of the path from the instance to get the instance name
    //      to modify.  Works but need to check DMTF specs.

    // Get path from input instanceReference OR build it

    CIMObjectPath localRef = (instanceReference.getKeyBindings().size() == 0)?
                                instanceObject.buildPath(mc)
                                :
                                _localPath(instanceReference);

    AutoMutex autoMut(instanceArrayMutex);

    // Find the proper instance.
    Uint32 index;
    if ((index = findInstance(localRef)) != PEG_NOT_FOUND)
    {
        // Modify the existing instance
        Uint32 pos;
        for (Uint32 j = 0 ;  j < instanceObject.getPropertyCount() ; j++)
        {
            CIMConstProperty r1 = instanceObject.getProperty(j);
            CIMProperty r2 = r1.clone();
            Uint32 pos;
            try
            {
                if ((pos = _instances[index].findProperty(r2.getName()))
                     != PEG_NOT_FOUND)
                {
                    _instances[index].removeProperty(pos);
                    _instances[index].addProperty(r2);
                }
                else   // simply add the property since not in instance
                {
                    // test if property is in class
                    if (mc.findProperty(r2.getName()) != PEG_NOT_FOUND)
                    {
                        _instances[index].addProperty(r2);
                    }
                    else
                    {
                        throw CIMException(CIM_ERR_INVALID_PARAMETER,
                              "Property Not in class " +
                              r2.getName().getString());
                    }
                }
            }  // end try block
            catch(CIMException& e)
            {
                throw CIMException(CIM_ERR_FAILED,
                                   " Updating Property " + e.getMessage());
            }
        }  // for loop processing properties

        handler.complete();
    } // end if found
    else
    {
        throw CIMException(CIM_ERR_NOT_FOUND);
    }
}

void CLITestProvider::createInstance(
    const OperationContext & context,
    const CIMObjectPath & instanceReference,
    const CIMInstance & instanceObject,
    ObjectPathResponseHandler & handler)
{
    initializeProvider(instanceReference.getNameSpace());

    handler.processing();

    CIMObjectPath newInstanceRef = _localPath(instanceReference);

    AutoMutex autoMut(instanceArrayMutex);
    // If there are no properties in the reference, try to get the
    // key properties and their values from the instanceObject
    if (instanceReference.getKeyBindings().size() == 0)
    {
        Array<CIMKeyBinding> keys;

        Uint32 pos = instanceObject.findProperty("Id");

        if (pos != PEG_NOT_FOUND)
        {
            CIMConstProperty cimProperty = instanceObject.getProperty(pos);

            keys.append(CIMKeyBinding(cimProperty.getName(),
                                      cimProperty.getValue()));

            newInstanceRef.setKeyBindings(keys);
        }
        else
        {
            throw CIMPropertyNotFoundException("Id");
        }
    }

    // If the instance exists, throw already_exists exception
    // Note: instances in the array do not have path component

    Uint32 index;
    if ((index = findInstance(newInstanceRef)) == PEG_NOT_FOUND)
    {
        // add the instance to the set of instances saved in the provider.
        CIMInstance myInstance = instanceObject.clone();
        myInstance.setPath(newInstanceRef);
        _instances.append(myInstance);

        // Deliver path of new instance
        handler.deliver(newInstanceRef);
        handler.complete();
    }

    else
    {
        throw CIMException(CIM_ERR_ALREADY_EXISTS);
    }
}

void CLITestProvider::deleteInstance(
    const OperationContext & context,
    const CIMObjectPath & instanceReference,
    ResponseHandler & handler)
{
    initializeProvider(instanceReference.getNameSpace());

    handler.processing();

    // convert a fully qualified reference into a local reference
    // (class name and keys only).

    AutoMutex autoMut(instanceArrayMutex);

    Uint32 index;
    if ((index = findInstance(instanceReference)) != PEG_NOT_FOUND)
    {
        _instances.remove(index);
    }
    else
    {
        throw CIMException(CIM_ERR_NOT_FOUND);
    }

    handler.complete();
    return;
}

/*
    Processing of associator/Reference Operation Requests

    NOTE: This code is not based on any clear definition of the
    relationship between objects but simply returning information
    on instances that exist in the repository. Thus typically it returns
    the target instance itself (i.e. association of an instance with
    itself).  This works since the only goal of this provider
    is a syntatic test of cimcli, not any association structure. So our
    association is that every instance is associated with itself. Note that
    this removes any meaning from the role and assoc/result class parameters
    but we test the validity of these by returning a property in the
    returned instances containing all of these values so that the client
    can test to determine if the provider received what was input.
*/
void CLITestProvider::associators(
    const OperationContext& context,
    const CIMObjectPath& objectName,
    const CIMName& associationClass,
    const CIMName& resultClass,
    const String& role,
    const String& resultRole,
    const Boolean includeQualifiers,
    const Boolean includeClassOrigin,
    const CIMPropertyList& propertyList,
    ObjectResponseHandler& handler)
{
    initializeProvider(objectName.getNameSpace());

    // Get the namespace and host names to create the CIMObjectPath
    CIMNamespaceName nameSpace = objectName.getNameSpace();
    String host = _getHostName();

    handler.processing();
    // complete processing the request
    // Puts input parameters into the requestInputParameters property so that
    // they can be tested on by the client.
    String text;
    _addParam(text, "role", role);
    _addParam(text, "resultRole", resultRole);
    _addParam(text, "associationClass", associationClass.getString());
    _addParam(text, "resultClass", resultClass.getString());
    _addParam(text, "includeQualifiers", _toString(includeQualifiers));
    _addParam(text, "includeClassOrigin", _toString(includeClassOrigin));
    _addParam(text, "propertyList", _toString(propertyList));

    // Return an instance of the associated class for every instance
    // currently in the local list.
    Uint32 index;
    if ((index = findInstance(objectName)) != PEG_NOT_FOUND)
    {
        try
        {
            CIMInstance temp = _instances[index].clone();
            if (temp.findProperty("requestInputParameters") == PEG_NOT_FOUND)
            {
                CIMClass tmpClass = _getClass(CIMName(
                    "Test_CLITestProviderClass"),
                    nameSpace);
                Uint32 pos = tmpClass.findProperty("requestInputParameters");
                temp.addProperty(tmpClass.getProperty(pos));
            }

            temp.getProperty(temp.findProperty("requestInputParameters"))
                .setValue(text);

            CIMObjectPath rtnObjectName = objectName;
            if (_useSubstituteHostName)
            {
                rtnObjectName.setHost(_getHostName());
            }
            temp.setPath(rtnObjectName);
            temp.filter(includeQualifiers,
                        includeClassOrigin, propertyList);
            handler.deliver(temp);
        }
        catch(CIMException& e)
        {
            cerr << "CIMCLITestProvider:Exception Occured : "
                << e.getMessage() << endl;
            throw CIMException(e);
        }
    }
    handler.complete();
}

void CLITestProvider::associatorNames(
    const OperationContext& context,
    const CIMObjectPath& objectName,
    const CIMName& associationClass,
    const CIMName& resultClass,
    const String& role,
    const String& resultRole,
    ObjectPathResponseHandler& handler)
{
    initializeProvider(objectName.getNameSpace());
    // Get the namespace and host names to create the CIMObjectPath
    CIMNamespaceName nameSpace = objectName.getNameSpace();
    String host = _getHostName();

    handler.processing();

    // Return an instance of the associated class for every instance
    // currently in the local list. Simple since we just return the
    // input path if the instance exists.
    Uint32 index;
    if ((index = findInstance(objectName)) != PEG_NOT_FOUND)
    {
        try
        {
            CIMObjectPath rtnObjectName = objectName;
            if (_useSubstituteHostName)
            {
                rtnObjectName.setHost(_getHostName());
            }
            handler.deliver(rtnObjectName);
        }
        catch(CIMException& e)
        {
            cerr << "CIMCLITestProvider:Exception Occured : "
                << e.getMessage() << endl;
            throw CIMException(e);
        }
    }
    handler.complete();
}

void CLITestProvider::references(
    const OperationContext& context,
    const CIMObjectPath& objectName,
    const CIMName& resultClass,
    const String& role,
    const Boolean includeQualifiers,
    const Boolean includeClassOrigin,
    const CIMPropertyList& propertyList,
    ObjectResponseHandler& handler)
{
    initializeProvider(objectName.getNameSpace());
    // Get the namespace and host names to create the CIMObjectPath
    CIMNamespaceName nameSpace = objectName.getNameSpace();
    String host = _getHostName();

    handler.processing();

    CIMName objectClassName = objectName.getClassName();

    // if the target instance exists in the local storage, build the
    // association class instance
    AutoMutex autoMut(instanceArrayMutex);
    Uint32 index;
    if ((index = findInstance(objectName)) != PEG_NOT_FOUND)
    {
        Array<CIMName> refClassArray;
        refClassArray.append(CIMName("Test_CLITestProviderLinkClass"));

        // Create a single instance of the Test_CLITestProviderLinkClass
        // This creates a single fixed instance simply to allow the
        // cimcli client to test results.  It also places the input parameters
        // into the text result so that the client can confirm that the
        // input parameters were passed to the provider.
        // Create path for assoc instance.

        CIMClass assocClass = _getClass(CIMName(
            "Test_CLITestProviderLinkClass"),
            nameSpace);

        // Create a single instance with all properties and with path
        // independent of namespace or hostname

        CIMInstance assocInstance = assocClass.buildInstance(
            true, true, CIMPropertyList());

        CIMObjectPath rtnObjectName = objectName;
        if (_useSubstituteHostName)
        {
            rtnObjectName.setHost(_getHostName());
        }

        assocInstance.getProperty(
            assocInstance.findProperty("parent"))
            .setValue(rtnObjectName);

        assocInstance.getProperty(
            assocInstance.findProperty("child"))
            .setValue(rtnObjectName);

        // Put input parameters into the requestInputParameters property so
        // they can be confirmed by the client.
        String text;
        _addParam(text, "role", role);
        _addParam(text, "resultClass", resultClass.getString());
        _addParam(text, "includeQualifiers", _toString(includeQualifiers));
        _addParam(text, "includeClassOrigin", _toString(includeClassOrigin));
        _addParam(text, "propertyList", _toString(propertyList));

        assocInstance.getProperty(
            assocInstance.findProperty("requestInputParameters"))
            .setValue(text);

        CIMObjectPath objectPath =
            assocInstance.buildPath(assocClass);

        _completePath(host, nameSpace, objectPath);

        assocInstance.setPath(objectPath);

        // complete processing the request
        assocInstance.filter(includeQualifiers,
                    includeClassOrigin, propertyList);

        handler.deliver(assocInstance);
    }
    handler.complete();
}

// Return all references (association instance names) in which the given
// object is involved.

void CLITestProvider::referenceNames(
    const OperationContext& context,
    const CIMObjectPath& objectName,
    const CIMName& resultClass,
    const String& role,
    ObjectPathResponseHandler& handler)
{
    CIMNamespaceName nameSpace = objectName.getNameSpace();
    initializeProvider(nameSpace);
    // Get the namespace and host names to create the CIMObjectPath

    String host = _getHostName();

    // If the objectName exists in the local list, build the instance
    // of the association and then build the path for this instance.

    AutoMutex autoMut(instanceArrayMutex);
    Uint32 index;

    if ((index = findInstance(objectName)) != PEG_NOT_FOUND)
    {
        Array<CIMName> refClassArray;
        refClassArray.append(CIMName("Test_CLITestProviderLinkClass"));

        CIMInstance assocInstance("Test_CLITestProviderLinkClass");

        CIMObjectPath rtnObjectName = objectName;
        if (_useSubstituteHostName)
        {
            rtnObjectName.setHost(_getHostName());
        }

        assocInstance.addProperty(CIMProperty(CIMName("parent"),
            rtnObjectName,
            0,
            CIMName("Test_CLITestProviderClass")));

        assocInstance.addProperty(CIMProperty(CIMName("child"),
            rtnObjectName,
            0,
            CIMName("Test_CLITestProviderClass")));

        CIMClass assocClass = _getClass(
            CIMName("Test_CLITestProviderLinkClass"),
            nameSpace);

        // build path for this instance
        CIMObjectPath objectPath =
            assocInstance.buildPath(assocClass);

        _completePath(host, nameSpace, objectPath);

        handler.deliver(objectPath);
    }
    // complete processing the request
    handler.complete();
}


/* get the defined class from the repository.
    @param className CIMName name of the class to get
    @return CIMClass with the class or unitialized if
    there was an error in the getClass
*/
CIMClass CLITestProvider::_getClass(const CIMName& className,
                                    const CIMNamespaceName& ns)
{
    CIMClass c;
    try
    {
        c = _cimom.getClass(
            OperationContext(),
            ns,
            className,
            false,
            true,
            true,
            CIMPropertyList());
    }
    catch (CIMException& e)
    {
        PEG_TRACE((TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "CLITestProvider GetClass operation failed: Class %s. Msg %s",
            (const char*) className.getString().getCString(),
            (const char*) e.getMessage().getCString()));
        throw CIMException(CIM_ERR_FAILED);
    }
    return c;
}

/*
    Find an instance in the instance array with path defined by
    the input parameter. returns the index of the instance or PEG_NOT_FOUND
*/
Uint32 CLITestProvider::findInstance(const CIMObjectPath& path)
{
    CIMObjectPath localPath = _localPath(path);

    for (Uint32 i = 0; i < _instances.size(); i++)
    {
        if(localPath == _instances[i].getPath())
        {
            return i;
        }
    }
    return PEG_NOT_FOUND;
}
// Create the instances that will be considered inherently part of the
// provider for these tests.  This includes one instance of each class
// This was done with namespace input so that we could build association
// instances that require namespace.  It later turned out to be easier
// to build them on the fly so that the namespace parameter and the
// corresponding dynamic initialization of the provider (initializeProvider)
// are not really necessary.

void CLITestProvider::createInstances(const CIMNamespaceName& ns)
{

    CIMClass theClass = _getClass(
        CIMName("Test_CLITestProviderClass"),
        ns);
    // Create a single instance with all properties and with path
    // independent of namespace or hostname

    CIMInstance instance = theClass.buildInstance(
        true, true, CIMPropertyList());

    instance.getProperty(instance.findProperty("Id")).setValue(
        CIMValue(String("Mike")));
    instance.getProperty(instance.findProperty("Name")).setValue(
        CIMValue(String("Bob")));
    instance.getProperty(instance.findProperty("scalBool")).setValue(
        CIMValue(Boolean(true)));
    instance.getProperty(instance.findProperty("scalUint8")).setValue(
        CIMValue(Uint8(220)));
    instance.getProperty(instance.findProperty("scalSint8")).setValue(
        CIMValue(Sint8(124)));
    instance.getProperty(instance.findProperty("scalUint16")).setValue(
        CIMValue(Uint16(100)));
    instance.getProperty(instance.findProperty("scalSint16")).setValue(
        CIMValue(Sint16(100)));
    instance.getProperty(instance.findProperty("scalUint32")).setValue(
        CIMValue(Uint32(100)));
    instance.getProperty(instance.findProperty("scalSint32")).setValue(
        CIMValue(Sint32(100)));
    instance.getProperty(instance.findProperty("scalUint64")).setValue(
        CIMValue(Uint64(100)));
    instance.getProperty(instance.findProperty("scalReal32")).setValue(
        CIMValue(Real32(100)));
    instance.getProperty(instance.findProperty("scalReal64")).setValue(
        CIMValue(Real64(100)));
    instance.getProperty(instance.findProperty("scalString")).setValue(
        CIMValue(String("teststring")));
    instance.getProperty(instance.findProperty("scalDateTime")).setValue(
        CIMValue(CIMDateTime("19991224120000.000000+360")));

    // set Values into the corresponding array properties
    Array<Boolean> ab;
    ab.append(true); ab.append(false); ab.append(true);
    instance.getProperty(instance.findProperty("arrayBool")).setValue(
        CIMValue(ab));

    Array<Uint8> auint8;
    auint8.append(4); auint8.append(128); auint8.append(240);
    instance.getProperty(instance.findProperty("arrayUint8")).setValue(
        CIMValue(auint8));

    Array<Sint8> asint8;
    asint8.append(4); asint8.append(126); asint8.append(-126);
    instance.getProperty(instance.findProperty("arraySint8")).setValue(
        CIMValue(asint8));

    Array<Uint16> auint16;
    auint16.append(4); auint16.append(128); auint16.append(240);
    instance.getProperty(instance.findProperty("arrayUint16")).setValue(
        CIMValue(CIMValue(auint16)));

    Array<Sint16> asint16;
    asint16.append(4); asint16.append(126); asint16.append(-126);
    instance.getProperty(instance.findProperty("arraySint16")).setValue(
        CIMValue(asint16));

    Array<Uint32> auint32;
    auint32.append(4); auint32.append(128); auint32.append(240);
    instance.getProperty(instance.findProperty("arrayUint32")).setValue(
        CIMValue(auint32));

    Array<Sint32> asint32;
    asint32.append(4); asint32.append(126); asint32.append(-126);
    instance.getProperty(instance.findProperty("arraySint32")).setValue(
        CIMValue(asint32));

    Array<Uint64> auint64;
    auint64.append(4); auint64.append(128); auint64.append(240);
    instance.getProperty(instance.findProperty("arrayUint64")).setValue(
         CIMValue(auint64));

    Array<Real32> aReal32;
    aReal32.append(4); aReal32.append(128); aReal32.append(240);
    instance.getProperty(instance.findProperty("arrayReal32")).setValue(
        CIMValue(aReal32));

    Array<Real64> aReal64;
    aReal64.append(4); aReal64.append(128); aReal64.append(240);
    instance.getProperty(instance.findProperty("arrayReal64")).setValue(
        CIMValue(aReal64));

    Array<String> aString;
    aString.append("First"); aString.append("Second"); aString.append("Third");
    instance.getProperty(instance.findProperty("arrayString")).setValue(
        CIMValue(aString));

    Array<CIMDateTime> aCIMDateTime;
    aCIMDateTime.append(CIMDateTime("19991224120000.000000+360"));
    aCIMDateTime.append(CIMDateTime("19991224120000.000000+360"));
    aCIMDateTime.append(CIMDateTime("19991224120000.000000+360"));
    instance.getProperty(instance.findProperty("arrayDateTime")).setValue(
        CIMValue(aCIMDateTime));

    CIMObjectPath p("Test_CLITestProviderClass.Id=\"Mike\"");

    instance.setPath(p);

    _instances.append(instance);
}

void CLITestProvider::initializeProvider(const CIMNamespaceName& ns)
{
    if (!_initialized)
    {
        AutoMutex autoMut(instanceArrayMutex);
        if (!_initialized)
        {
            createInstances(ns);
            _initialized = true;
            _useSubstituteHostName = false;
        }
    }
}

