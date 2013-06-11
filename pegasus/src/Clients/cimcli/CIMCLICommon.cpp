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
//

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/StringConversion.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/Tracer.h>
#include "CIMCLICommon.h"
#include "CIMCLIClient.h"

PEGASUS_USING_STD;
PEGASUS_NAMESPACE_BEGIN

/* Convert Boolean parameter to String "true" or "false"
*/
String _toString(Boolean x)
{
    return(x ? "true" : "false");
}

void _print(Boolean x)
{
    cout << _toString(x);
}

// Convert a CIMPropertyList parameter to CIM String
String _toString(const CIMPropertyList& pl)
{
    String rtn;
    Array<CIMName> pls = pl.getPropertyNameArray();
    if (pl.isNull())
        return("NULL");

    if (pl.size() == 0)
        return("EMPTY");

    for (Uint32 i = 0 ; i < pls.size() ; i++)
    {
        if (i != 0)
            rtn.append(", ");
        rtn.append(pls[i].getString());
    }
    return(rtn);
}

// Output a CIMPropertyList to cout
void _print(const CIMPropertyList& pl)
{
    cout << _toString(pl);
}

String _toString(const Array<CIMNamespaceName>& nsList)
{
    String rtn;
    for (Uint32 i = 0 ; i < nsList.size() ; i++ )
    {
        if (i != 0)
        {
            rtn.append(", ");
        }
        rtn.append(nsList[i].getString());
    }
    return rtn;
}

void _print(const Array<CIMNamespaceName>& List)
{
    cout << _toString(List);
}

String _toString(const Array<String>& strList)
{
    String rtn;
    for (Uint32 i = 0 ; i < strList.size() ; i++)
    {
        if (i > 0)
        {
            rtn.append(", ");
        }
        rtn.append(strList[i]);
    }
    return rtn;
}
void _print(const Array<String>& strList)
{
    cout << _toString(strList);
}

/** tokenize an input string into an array of Strings,
 * separating the tokens at the separator character
 * provided
 * @param input String
 * @param separator character
 * @param all Boolean if true do multiple tokens.
 * If false, stop after first.
 * @returns Array of separated strings
 * Terminates
 *  after first if all = false.
 * */
Array<String> _tokenize(const String& input,
                        const Char16 separator,
                        bool allTokens)
{
    ///////cout << "Enter _Tokenize2" << endl;
    Array<String> tokens;
    if (input.size() != 0)
    {
        Uint32 start = 0;
        Uint32 length = 0;
        Uint32 end = 0;
        if (allTokens)
        {
            while ((end = input.find(start, separator)) != PEG_NOT_FOUND)
            {
                length = end - start;

                /////cout << "This2 Token = " << input.subString(start,
                ///  length) << endl;
                tokens.append(input.subString(start, length));
                start += (length + 1);
            }
        }
        else
        {
            if ((length = input.find(start, separator)) != PEG_NOT_FOUND)
            {

                /////cout << "This2 Token = " << input.subString(start,
                ///length) << endl;
                tokens.append(input.subString(start,length));
                start+= (length + 1);
            }
        }
        //Replaced < with <= to consider input param like A="" as valid param.
        //key in this param is 'A'and value is NULL.
        //It also takes care of A= param.
        if(start <= input.size())
        {
            tokens.append(input.subString(start));
        }
    }
    return tokens;
}

/* Build a property list from all of the property names in the input instance.
*/
CIMPropertyList _buildPropertyList(const CIMInstance& inst)
{
    Array<CIMName> tmp;
    for (Uint32 i = 0 ; i < inst.getPropertyCount() ; i++)
    {
        CIMConstProperty instProperty = inst.getProperty(i);
        tmp.append(instProperty.getName());
    }
    CIMPropertyList pl;
    pl.set(tmp);

    return(pl);
}

Sint64 strToSint(const char* str, CIMType type)
{
    Sint64 s64;
    Boolean success =
        (StringConversion::stringToSint64(
             str, StringConversion::decimalStringToUint64, s64) ||
         StringConversion::stringToSint64(
             str, StringConversion::hexStringToUint64, s64) ||
         StringConversion::stringToSint64(
             str, StringConversion::octalStringToUint64, s64) ||
         StringConversion::stringToSint64(
             str, StringConversion::binaryStringToUint64, s64)) &&
        StringConversion::checkSintBounds(s64, type);
    if (!success)
    {
        printf("Parse Error: Value conversion error. %s. type %s\n",
               str, cimTypeToString(type));
    }

    return s64;
}

Uint64 strToUint(const char* str, CIMType type)
{
    Uint64 u64;
    Boolean success =
        (StringConversion::decimalStringToUint64(str, u64) ||
         StringConversion::hexStringToUint64(str, u64) ||
         StringConversion::octalStringToUint64(str, u64) ||
         StringConversion::binaryStringToUint64(str, u64)) &&
         StringConversion::checkUintBounds(u64, type);

    if (!success)
    {
        fprintf(stderr,"Parse Error: Value conversion error. %s. type %s\n",
               str, cimTypeToString(type));
        exit(CIMCLI_INPUT_ERR);
    }

    return u64;
}

Real64 strToReal(const char * str, CIMType type)
{
    Real64 r64;

    if (!StringConversion::stringToReal64(str, r64))
    {
        fprintf(stderr, "Parse Error: Value conversion error. %s. type %s\n",
               str, cimTypeToString(type));
        exit(CIMCLI_INPUT_ERR);
    }
    return r64;
}

PEGASUS_NAMESPACE_END
// END_OF_FILE
