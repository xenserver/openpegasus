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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Pegasus/Provider/CMPI/cmpimacs.h>
#include "cmpiPerf_TestClass.h"

/* ---------------------------------------------------------------------------*/
/*                            Factory functions                               */
/* ---------------------------------------------------------------------------*/

/* ---------- method to create a CMPIObjectPath of this class ----------------*/

CMPIObjectPath * _makePath_TestClass(
    const CMPIBroker * _broker,
    const CMPIContext * ctx,
    const CMPIObjectPath * ref,
    CMPIStatus * rc,
    unsigned int theKey)
{
    CMPIObjectPath * op = NULL;

    op = CMNewObjectPath(
             _broker,
             CMGetCharsPtr(CMGetNameSpace(ref,rc), NULL),
             _ClassName, rc );

    if (CMIsNullObject(op))
    {
        CMSetStatusWithChars(
            _broker,
            rc,
            CMPI_RC_ERR_FAILED,
            "Create CMPIObjectPath failed." );
        goto exit;
    }

    CMAddKey(op, "theKey", &theKey, CMPI_uint32);

    exit:
    return op;
}

/* ----------- method to create a CMPIInstance of this class ----------------*/

CMPIInstance * _makeInst_TestClass(
    const CMPIBroker * _broker,
    const CMPIContext * ctx,
    const CMPIObjectPath * ref,
    const char ** properties,
    CMPIStatus * rc,
    unsigned int theKey)
{
    CMPIValue opstatus;
    CMPIValue status;
    CMPIObjectPath * op     = NULL;
    CMPIInstance   * ci     = NULL;
    char * theName          = NULL;
    CMPIArray       *array  = NULL;
    char     **keys      = NULL;
    int              keyCount  = 0;
    CMPIDateTime* my_dt;

    /* Unknown */
    opstatus.uint16 = 0;
    /* Enabled */
    status.uint16 = 2;

//
// Initialize Name
//
    theName = calloc(1,20);
    sprintf(theName, "%u", theKey);

//
// Construct ObjectPath
//
    op = CMNewObjectPath(
             _broker,
             CMGetCharsPtr(CMGetNameSpace(ref,rc), NULL),
             _ClassName,
             rc);
    if (CMIsNullObject(op))
    {
        CMSetStatusWithChars(
            _broker,
            rc,
            CMPI_RC_ERR_FAILED,
            "Create CMPIObjectPath failed." );
        goto exit;
    }

//
// Create a new instance and fill it's properties
//
    ci = CMNewInstance( _broker, op, rc);
    if (CMIsNullObject(ci))
    {
        CMSetStatusWithChars(
            _broker,
            rc,
            CMPI_RC_ERR_FAILED,
            "Create CMPIInstance failed." );
        goto exit;
    }

//
// Set property filter
//
    keys = calloc(2,sizeof(char*));
    keys[0] = strdup("theKey");
    CMSetPropertyFilter(ci,properties,(const char**)keys);
    for (;keys[keyCount]!=NULL;keyCount++)
    {
        free((char*)keys[keyCount]);
    }
    free(keys);

//
// Properties of CIM_ManagedElement
//
    CMSetProperty(ci,"Caption","Performance Test class",CMPI_chars);
    CMSetProperty(
        ci,
        "Description",
        "Test class used for all kinds of testing",
        CMPI_chars);
    CMSetProperty(ci,"ElementName",theName,CMPI_chars);

//
// Properties of CIM_ManagedSystemElement
//
     my_dt = CMNewDateTimeFromChars(
         _broker,
         "20090102030405.000000+120",
         rc);

     CMSetProperty( ci, "InstallDate", (CMPIValue*)&(my_dt), CMPI_dateTime);

// TBD: InstallDate
    array = CMNewArray(_broker,1,CMPI_uint16,rc);
    CMSetArrayElementAt(array,0,&opstatus,CMPI_uint16);
    CMSetProperty(ci,"OperationalStatus",(CMPIValue*)&(array),CMPI_uint16A);

//
// Properties of CIM_EnabledLogicalElement
//
    CMSetProperty( ci, "EnabledState", &status, CMPI_uint16);
    CMSetProperty( ci, "RequestedState", &status, CMPI_uint16);
    CMSetProperty( ci, "EnabledDefault", &status, CMPI_uint16);

//
// Properties of CIM_TestClass
//
    CMSetProperty(ci,"theKey",&theKey , CMPI_uint32 );
    CMSetProperty(ci,"theData",&theKey , CMPI_uint32 );
    CMSetProperty(ci,"theString0","Test Data Number Zero",CMPI_chars);
    CMSetProperty(ci,"theString1","Test Data Number One",CMPI_chars);
    CMSetProperty(ci,"theString2","Test Data Number Two",CMPI_chars);
    CMSetProperty(ci,"theString3","Test Data Number Three",CMPI_chars);
    CMSetProperty(ci,"theString4","Test Data Number Four",CMPI_chars);
    CMSetProperty(ci,"theString5","Test Data Number Five",CMPI_chars);
    CMSetProperty(ci,"theString6","Test Data Number Six",CMPI_chars);
    CMSetProperty(ci,"theString7","Test Data Number Seven",CMPI_chars);
    CMSetProperty(ci,"theString8","Test Data Number Eight",CMPI_chars);
    CMSetProperty(ci,"theString9","Test Data Number Nine",CMPI_chars);


    exit:
    free(theName);
    return ci;
}

/* ---------------------------------------------------------------------------*/
/*          end of cmpi_TestClass.c                                     */
/* ---------------------------------------------------------------------------*/
