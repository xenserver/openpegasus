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

#include "WQLOperation.h"

PEGASUS_NAMESPACE_BEGIN

const char* WQLOperationToString(WQLOperation x)
{
    switch (x)
    {
        case WQL_OR: return "WQL_OR";
        case WQL_AND: return "WQL_AND";
        case WQL_NOT: return "WQL_NOT";
        case WQL_EQ: return "WQL_EQ";
        case WQL_NE: return "WQL_NE";
        case WQL_LT: return "WQL_LT";
        case WQL_LE: return "WQL_LE";
        case WQL_GT: return "WQL_GT";
        case WQL_GE: return "WQL_GE";
        case WQL_IS_NULL: return "WQL_IS_NULL";
        case WQL_IS_TRUE: return "WQL_IS_TRUE";
        case WQL_IS_FALSE: return "WQL_IS_FALSE";
        case WQL_IS_NOT_NULL: return "WQL_IS_NOT_NULL";
        case WQL_IS_NOT_TRUE: return "WQL_IS_NOT_TRUE";
        case WQL_IS_NOT_FALSE: return "WQL_IS_NOT_FALSE";
        case WQL_LIKE: return "WQL_LIKE";
        default: return "UNKNOWN OPERATION";
    }
}

PEGASUS_NAMESPACE_END
