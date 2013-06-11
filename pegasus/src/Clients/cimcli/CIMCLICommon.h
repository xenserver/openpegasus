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

#ifndef _CLI_COMMON_H
#define _CLI_COMMON_H

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/PegasusAssert.h>
#include <Clients/cimcli/Linkage.h>
#include <Pegasus/Common/CIMPropertyList.h>

#include <Pegasus/Common/CIMInstance.h>
PEGASUS_NAMESPACE_BEGIN

String  PEGASUS_CLI_LINKAGE _toString(Boolean x);
void  PEGASUS_CLI_LINKAGE _print(Boolean x);

String  PEGASUS_CLI_LINKAGE _toString(const CIMPropertyList& pl);
void  PEGASUS_CLI_LINKAGE _print(const CIMPropertyList& pl);

String  PEGASUS_CLI_LINKAGE _toString(const Array<String>& strList);
void  PEGASUS_CLI_LINKAGE _print(const Array<String>& strList);

// Generate comma separated list of namespace names
String PEGASUS_CLI_LINKAGE _toString(const Array<CIMNamespaceName>& List);
void PEGASUS_CLI_LINKAGE _print(const Array<CIMNamespaceName>& List);

Array<String>  PEGASUS_CLI_LINKAGE _tokenize(
    const String& input,
    const Char16 separator,
    bool allTokens);


/* Build a property list from all of the property names in the input instance
   @param inst CIMInstance from which propertylist built
   @return CIMPropertyList will all names from the instance
*/
CIMPropertyList PEGASUS_CLI_LINKAGE _buildPropertyList(const CIMInstance& inst);

/*
    Common functions for conversion of char* strings to CIMTypes defined
    by the type variable.  Note that all of these functions execute an exit
    if the conversion fails with the exit code set to CIMCLI_INPUT_ERR.
    They are intended for parsing of input from command line, config files,
    etc. All allow input in binary, octal or decimal formats.
*/
Sint64 PEGASUS_CLI_LINKAGE strToSint(const char* str, CIMType type);

Uint64 PEGASUS_CLI_LINKAGE strToUint(const char* str, CIMType type);

Real64 PEGASUS_CLI_LINKAGE strToReal(const char * str, CIMType type);


PEGASUS_NAMESPACE_END

#endif  /* _CLI_COMMON_H */
