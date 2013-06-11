//%2006////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, 2001, 2002 BMC Software; Hewlett-Packard Development
// Company, L.P.; IBM Corp.; The Open Group; Tivoli Systems.
// Copyright (c) 2003 BMC Software; Hewlett-Packard Development Company, L.P.;
// IBM Corp.; EMC Corporation, The Open Group.
// Copyright (c) 2004 BMC Software; Hewlett-Packard Development Company, L.P.;
// IBM Corp.; EMC Corporation; VERITAS Software Corporation; The Open Group.
// Copyright (c) 2005 Hewlett-Packard Development Company, L.P.; IBM Corp.;
// EMC Corporation; VERITAS Software Corporation; The Open Group.
// Copyright (c) 2006 Hewlett-Packard Development Company, L.P.; IBM Corp.;
// EMC Corporation; Symantec Corporation; The Open Group.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN
// ALL COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//==============================================================================
//
//%/////////////////////////////////////////////////////////////////////////////
#ifndef _CLI_ObjectBuilder_h
#define _CLI_ObjectBuilder_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <Clients/cimcli/Linkage.h>
#include <Pegasus/Client/CIMClient.h>

/*
    Class and functions to build instances and parameters from
    token pairs (key=value) strings input.
    The constructor sets up the token pairs and separates out
    keys and values (and optionally types defined by the keys.
    It also gets any metadata (i.e. Classes)

    The build methods use this to build the objects.

*/

PEGASUS_NAMESPACE_BEGIN

class PEGASUS_CLI_LINKAGE ObjectBuilder
{
public:
    // not used
    ObjectBuilder();

    /** create the arrays for the defined input
    @param inputs Name/value pairs representing the input
    properties
    @param class the CIMClass for which we are creating something
    */
    ObjectBuilder(const Array<String>& inputPairs,
        CIMClient& client,
        const CIMNamespaceName& nameSpace,
        const CIMName& className,
        const CIMPropertyList& cimPropertyList,
        Boolean verbose);

    ~ObjectBuilder();

    // get next evaluated property
    Array<CIMName> getPropertyList();

    // Create an instance with the properties define
    // in the create
    CIMInstance buildInstance(Boolean includeQualifiers,
                              Boolean includeClassOrigin,
                              const CIMPropertyList& propertyList);

    //Create a CIMParamValue for the defined parameter name
    Array<CIMParamValue> buildMethodParameters();

    void setMethod(CIMName& name);

    // Returns the class definition that was acquired during the
    // construction.  This is provided so that other functions do
    // not have to repeat getting the class.
    const CIMClass getTargetClass();

    // Build the CIMObjectPath from the information provided with
    // the constructor
    CIMObjectPath buildCIMObjectPath();

private:

    // Enumeration of the valuePair parsing return types
    enum _termType { ILLEGAL, VALUE, NO_VALUE, EXCLAM, NAME_ONLY };

    Uint32 _parseValuePair(
        const String& input,
        String& name,
        String& value);

    // The following Arrays are a common
    // structure representing the input key/value
    // parameters with an entry in each for each
    // input parameter
    Array<CIMName> _featureNameList;
    Array<String> _valueStringList;
    Array<Uint32> _CIMTypeList;
    Array<Uint32> _parseType;

    CIMName _methodName;
    CIMClass _thisClass;
    CIMName _className;
    Boolean _verbose;
    CIMNamespaceName _nameSpace;
};

PEGASUS_NAMESPACE_END
#endif
