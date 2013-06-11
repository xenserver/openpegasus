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
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/StringConversion.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/XmlWriter.h>

#include "CIMCLIClient.h"
#include "ObjectBuilder.h"
#include "CIMCLICommon.h"

PEGASUS_USING_STD;
PEGASUS_NAMESPACE_BEGIN

class csvStringParse;

// Cleans an input array by removing the { } tokens that surround
// the array.  Does nothing if they do not exist.  If there is
// an unmatched set, an error is generated.
// Allows the user to input array definitions surrounded by { }
// as an option.
void _cleanArray(String& x)
{
    if (x[0] == '{' && x[x.size()] == '}')
    {
        x.remove(0,1);
        x.remove(x.size(),1);
    }
    else if (x[0] == '{' || x[x.size()] == '}')
    {
        cerr << "Error: Parse Error: Array " << x << endl;
        exit(CIMCLI_INPUT_ERR);
    }
}

// FUTURE - Expand this for other date time literals suchs as TODAY
//
CIMDateTime _getDateTime(const String& str)
{
    if (String::equalNoCase(str,"NOW"))
    {
        return CIMDateTime::getCurrentDateTime();
    }
    return CIMDateTime(str);
}

Boolean _StringToBoolean(const String& x)
{
    if (String::equalNoCase(x,"true"))
    {
        return true;
    }
    else if (String::equalNoCase(x,"false"))
    {
        return false;
    }
    cerr << "Parse Error: Boolean Parmeter " << x << endl;
    exit(CIMCLI_INPUT_ERR);
    return false;
}

Uint32 _includesType(const String& name)
{
    // FUTURE  KS add code. For now this is null because we do not
    // support the data type option on input.
    return 0;
}

/* Parser for comma-separated-strings (csv). This parser takes into
   account quoted strings the " character and returns everything
   within a quoted block in the string in one batch.  It also
   considers the backslash "\" escape character to escape single
   double quotes.
   Example:
     csvStringParse x (inputstring, ",");
     while (x.more())
        rtnString = x.next();
*/
class csvStringParse
{
public:
    /* Define a string to parse for comma separated values and the
       separation character
    */
    csvStringParse(const String& csvString, const int separator)
    {
        _inputString = csvString;
        _separator = separator;
        _idx = 0;
        _end = csvString.size();
    }

    /* determine if there is more to parse
       @return true if there is more to parse
    */
    Boolean more()
    {
        return (_idx < _end)? true : false;
    }

    /* get next string from input. Note that this will continue to
       return empty strings if you parse past the point where more()
       returns false.
       @return String
    */
    String next()
    {
        String rtnValue;
        parsestate state = NOTINQUOTE;

        while ((_idx <= _end) && (_inputString[_idx]))
        {
            char idxchar = _inputString[_idx];
            switch (state)
            {
                case NOTINQUOTE:
                    switch (idxchar)
                    {
                        case '\\':
                            state = INSQUOTE;
                            break;

                        case '"':
                            state = INDQUOTE;
                            break;

                        default:
                            if (idxchar == _separator)
                            {
                                _idx++;
                                return rtnValue;
                            }
                            else
                                rtnValue.append(idxchar);
                            break;
                    }
                    break;

                // add next character and set NOTINQUOTE State
                case INSQUOTE:
                    rtnValue.append(idxchar);
                    state = NOTINQUOTE;
                    break;

                // append all but quote character
                case INDQUOTE:
                    switch (idxchar)
                    {
                        case '"':
                            state = NOTINQUOTE;
                            break;
                        default:
                            rtnValue.append(idxchar);
                            break;
                    }
            }
            _idx++;
        }   // end while

        return rtnValue;
    }

private:
    enum parsestate {INDQUOTE, INSQUOTE, NOTINQUOTE};
    Uint32 _idx;
    int _separator;
    Uint32 _end;
    String _inputString;
};

/* Convert a single string provided as input into a CIM variable
   and place it in a CIMValue.
   @param str char* representing string to be parsed.
   @param type CIMType expected.
   @return CIMValue with the new value. If the parse fails, this function
   terminates with an exit CIMCLI_INPUT_ERR.
*/
static CIMValue _stringToScalarValue(const char* str, CIMType type)
{
    Uint64 u64;

    switch (type)
    {
        case CIMTYPE_BOOLEAN:
            return CIMValue(_StringToBoolean(str));

        case CIMTYPE_UINT8:
            return CIMValue(Uint8(strToUint(str, type)));

        case CIMTYPE_SINT8:
            return CIMValue(Sint8(strToSint(str, type)));

        case CIMTYPE_UINT16:
            return CIMValue(Uint16(strToUint(str, type)));

        case CIMTYPE_SINT16:
            return CIMValue(Sint16(strToSint(str, type)));

        case CIMTYPE_UINT32:
            return CIMValue(Uint32(strToUint(str, type)));

        case CIMTYPE_SINT32:
            return CIMValue(Sint32(strToSint(str, type)));

        case CIMTYPE_UINT64:
            return CIMValue(Uint64(strToUint(str, type)));

        case CIMTYPE_SINT64:
            return CIMValue(Sint64(strToSint(str, type)));

        case CIMTYPE_REAL32:
            return CIMValue(Real32(strToReal(str, type)));

        case CIMTYPE_REAL64:
            return CIMValue(Real64(strToReal(str, type)));

        case CIMTYPE_STRING:
            return CIMValue(String(str));

        case CIMTYPE_DATETIME:
            return CIMValue(_getDateTime(str));

        default:
            cerr << "Error: Parser Error. Data type " << cimTypeToString(type)
                 << " not allowed" << endl;
            exit(CIMCLI_INPUT_ERR);
    }
    return CIMValue();
}
/*
    Parse out comma-separated values from input string and build the
    CIMValue array representing the input for array type input entities.
    @param str const char * containing the comma-separated values
    to be parsed.  All value elements in the CSV string must be
    parsable to the same type.
    @param v CIMValue into which the resulting values are added to
    any existing values
    @return Returns complete CIMValue.
*/
Boolean _buildArrayValue(
    const char * str,
    CIMValue& val)
{
    CIMType type = val.getType();
    Uint32 arrayDimension = val.getArraySize();
    String parseStr(str);

    csvStringParse strl(parseStr, ',');

    switch (type)
    {
        case CIMTYPE_BOOLEAN:
        {
            Array<Boolean> a;
            val.get(a);
            while(strl.more())
            {
                 a.append(_StringToBoolean(strl.next()));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_UINT8:
        {
            Array<Uint8> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToUint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_SINT8:
        {
            Array<Sint8> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToSint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_UINT16:
        {
            Array<Uint16> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToUint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_SINT16:
        {
            Array<Sint16> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToSint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_UINT32:
        {
            Array<Uint32> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToUint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_SINT32:
        {
            Array<Sint32> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToSint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_UINT64:
        {
            Array<Uint64> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToUint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_SINT64:
        {
            Array<Sint64> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToSint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_REAL32:
        {
            Array<Real32> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToUint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_REAL64:
        {
            Array<Real64> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strToSint(strl.next().getCString(), type));
            }
            val.set(a);
            break;
        }
        case CIMTYPE_STRING:
        {
            Array<String> a;
            val.get(a);
            while(strl.more())
            {
                a.append(strl.next());
            }
            val.set(a);
            break;
        }
        case CIMTYPE_DATETIME:
        {
            Array<CIMDateTime> a;
            val.get(a);
            while(strl.more())
            {
                a.append(_getDateTime(strl.next()));
            }
            val.set(a);
            break;
        }
    default:
        cout << "Error: Parse Error. Data type " << cimTypeToString(type)
        << " not allowed" << endl;
        exit(CIMCLI_INPUT_ERR);
    }
    return true;
}

/*
     Create a method parameter from the tokenized information for the
     method name, value, type, etc.
*/
CIMParamValue _createMethodParamValue(  const CIMName& name,
    const String& value,
    const CIMParameter& thisParam,
    Boolean verbose,
    const CIMNamespaceName& nameSpace)
{
    CIMType type = thisParam.getType();
    String paramName = thisParam.getName().getString();

    CIMValue vp(type,thisParam.isArray());

        if (vp.isArray())
        {
            _buildArrayValue(value.getCString(), vp );
            // set the value array into the property value
            return CIMParamValue(paramName,vp,true);
        }
        else
        {   // scalar
            CIMValue v;
            v = _stringToScalarValue(value.getCString(), vp.getType());
            return CIMParamValue(paramName,v,true);
        }

    // should never get here
    return CIMParamValue();
}

/******************************************************************
*
*   ObjectBuilder Class. Creates instances and parameters from
*     name/value pairs received and the corresponding class
*     definitions.
**
*******************************************************************/
/*
    Constructor, sets up a single object with the input pairs
    and the definition to get the class required for the
    parsing and entity building.
*/
ObjectBuilder::ObjectBuilder(
    const Array<String>& inputPairs,
    CIMClient& client,
    const CIMNamespaceName& nameSpace,
    const CIMName& className,
    const CIMPropertyList& cimPropertyList,
    Boolean verbose)
{
    // get the class. Exceptions including class_not_found are automatic
    // not localOnly (Need all properties), get qualifiers and classOrigin
    // since we can filter them out later

    _verbose = verbose;
    _className = className;

    _thisClass = client.getClass(nameSpace, className,
                        false,true,true,cimPropertyList);

    _nameSpace = nameSpace;
    if (inputPairs.size() != 0)
    {
        /* Here loop starts from 1, since the Class Name is  first parameter
           and we want only the property name and value here
        */
        for (Uint32 i = 1 ; i < inputPairs.size() ; i++)
        {
            String name;
            String value;
            Uint32 sep;

            // parse each pair and eliminate any illegal returns
            if ((sep = _parseValuePair(inputPairs[i], name, value)) != ILLEGAL)
            {
                _featureNameList.append(CIMName(name));
                _valueStringList.append(value);
                _parseType.append(sep);
            }
            else
            {
                cerr << "Parse Failed " << inputPairs[i] << endl;
            }
        }

        if (_verbose)
        {
            // This loop displays all the names and
            // property values of the instance
            for (Uint32 i=0; i < _featureNameList.size(); i++)
            {
                cout << "Name= " << _featureNameList[i].getString()
                     << ", valueString= " << _valueStringList[i] << endl;
            }
        }
    }
}

ObjectBuilder::~ObjectBuilder()
{
}

Array<CIMName> ObjectBuilder::getPropertyList()
{
    return  _featureNameList;
}

const CIMClass ObjectBuilder::getTargetClass()
{
    return _thisClass;
}

CIMObjectPath ObjectBuilder::buildCIMObjectPath()
{
    CIMInstance testInstance = buildInstance(
        true, true,
        CIMPropertyList());

    CIMObjectPath thisPath = testInstance.buildPath(_thisClass);

    return thisPath;
}
/*
    Parse the input string to separate out the strings for name and
    value components. The name component must be a CIM_Name and contain
    only alphanumerics and _ characters. The value is optional.
    The possible separators are today
    "=" and "!". and no value component is allowed after the ! separator
*/
Uint32 ObjectBuilder::_parseValuePair(const String& input,
                                         String& name,
                                         String& value)
{
    _termType terminator;
    value = String::EMPTY;

    Boolean separatorFound = false;
    for (Uint32 i = 0 ; i < input.size() ; i++)
    {
        if ((input[i] != '=') && (input[i] !=  '!'))
        {
            name.append(input[i]);
        }
        else  //separator character found
        {
            if (input[i] == '=')
            {
                terminator = VALUE;
            }
            if (input[i] == '!')
            {
                terminator = EXCLAM;
            }
            separatorFound = true;
            break;
        }
    }

    // separator must be found
    if (!separatorFound)
    {
        cerr << "Parse error: No separator " << input << endl;
        return ILLEGAL;
    }
    // There must be a name entity
    if (name.size() == 0)
    {
        cerr << "Parse error: No name component " << input << endl;
        return ILLEGAL;
    }
    // value component is optional but sets different _termType values
    if (input.size() == name.size() + 1)
    {
        if (terminator == EXCLAM)
        {
            return EXCLAM;
        }
        else
        {
            return NO_VALUE;
        }
    }
    value = input.subString(name.size() + 1, PEG_NOT_FOUND);
    return VALUE;
}
/*
    Build an instance from the information in the ObjectBuilder
    object.
*/
CIMInstance ObjectBuilder::buildInstance(
    Boolean includeQualifiers,
    Boolean includeClassOrigin,
    const CIMPropertyList& propertyList)
{
    // create the instance skeleton with the defined properties
    CIMInstance newInstance = _thisClass.buildInstance(
        includeQualifiers,
        includeClassOrigin,
        CIMPropertyList());

    Array<CIMName> myPropertyList;
    Uint32 propertyPos;

    // Set all the input property values into the instance
    for (Uint32 index = 0; index < _featureNameList.size(); index++)
    {
        if ((propertyPos = _thisClass.findProperty(
            _featureNameList[index])) == PEG_NOT_FOUND)
        {
            cerr << "Warning property Name "
                 << _featureNameList[index].getString()
                 << " Input value " <<  _valueStringList[index]
                 << " not in class: " << _thisClass.getClassName().getString()
                 << " Skipping."
                 << endl;
            continue;
        }

        // get value for this property from built instance

        CIMProperty ip = newInstance.getProperty(propertyPos);
        CIMValue iv = ip.getValue();

        // If input not NULL type (ends with =) set the value component
        // into property.  NULL input sets whatever is in the class into
        // the instance (done by buildInstance);
        if (_parseType[index] == EXCLAM)
        {
            if (iv.getType() == CIMTYPE_STRING)
            {
                iv.set(String::EMPTY);
                ip.setValue(iv);
            }
            else
            {
                cerr << "Error: " << ip.getName().getString()
                    << "! parameter terminator allowed only on String types "
                    << endl;
                exit(CIMCLI_INPUT_ERR);
            }
        }
        else if (_parseType[index] == VALUE)
        {
            if (iv.isArray())
            {
                _cleanArray(_valueStringList[index]);
                if (!_buildArrayValue(
                    _valueStringList[index].getCString(), iv))
                {
                    cerr << "Parse Error: parameter "
                       << _featureNameList[index].getString()
                       << " "
                       << _valueStringList[index] << endl;
                    exit(CIMCLI_INPUT_ERR);
                }
            }
            else  // scalar
            {
                // Replace property value in new instance
                iv = _stringToScalarValue(
                    _valueStringList[index].getCString(), ip.getType());
            }
            ip.setValue(iv);
        }

        else if (_parseType[index] == NO_VALUE)
        {
            // do nothing
        }
        else if (_parseType[index] == ILLEGAL)
        {
            // do nothing
        }

        myPropertyList.append(CIMName(_featureNameList[index]));
    }

    // Delete any properties not on the property list
    newInstance.filter(
        includeQualifiers,
        includeClassOrigin,
        CIMPropertyList(myPropertyList));

    // Display the Instance if verbose
    if (_verbose)
    {
        cout << "Instance Built" << endl;
        XmlWriter::printInstanceElement(newInstance, cout);
    }

    return newInstance;
}

Array<CIMParamValue> ObjectBuilder::buildMethodParameters()
{
    Array<CIMParamValue> params;

    // Find Method
    Uint32 methodPos;
    CIMMethod thisClassMethod;

    if ((methodPos = _thisClass.findMethod(_methodName)) == PEG_NOT_FOUND)
    {
        cerr << "Error: method " << _methodName.getString()
            << " Not method in the class " << _className.getString()
            <<  endl;
        exit(CIMCLI_INPUT_ERR);
    }
    else
    {
        thisClassMethod = _thisClass.getMethod(methodPos);
    }

    // find parameter for each input.
    for (Uint32 index = 0; index < _featureNameList.size(); index++)
    {
        Uint32 parameterPos;
        if ((parameterPos = thisClassMethod.findParameter(
            CIMName(_featureNameList[index]))) == PEG_NOT_FOUND)
        {
            cerr << "Error: parameter " << _featureNameList[index].getString()
                << " not valid method parameter in class "
                << _className.getString()
                << endl;
            exit(CIMCLI_INPUT_ERR);
        }

        CIMParameter thisParameter = thisClassMethod.getParameter(parameterPos);

        CIMParamValue localParamValue = _createMethodParamValue(
            _featureNameList[index],
            _valueStringList[index],
             thisParameter,
            _verbose,
            CIMNamespaceName());

        if (true)
        {
            // successful addition of parameter value
            params.append(localParamValue);
        }
        else
        {
            // error adding parameter value
            cerr << "Error: Parsing Error adding parameter"
                << _featureNameList[index].getString() << " "
                << _valueStringList[index] << endl;
                exit(CIMCLI_INPUT_ERR);
        }
    }
    return params;
}

void ObjectBuilder::setMethod(CIMName& name)
{
    _methodName = name;
}

PEGASUS_NAMESPACE_END

// END_OF_FILE
