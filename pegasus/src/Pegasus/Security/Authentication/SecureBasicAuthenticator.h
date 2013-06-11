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

#ifndef Pegasus_SecureBasicAuthenticator_h
#define Pegasus_SecureBasicAuthenticator_h

#include <Pegasus/Security/UserManager/UserManager.h>

#include "BasicAuthenticator.h"

#include <Pegasus/Security/Authentication/Linkage.h>

PEGASUS_NAMESPACE_BEGIN

/** This class provides Secure basic authentication implementation by extending
    the BasicAuthenticator.
*/
class PEGASUS_SECURITY_LINKAGE SecureBasicAuthenticator
    : public BasicAuthenticator
{
public:

    /** constructor. */
    SecureBasicAuthenticator();

    /** destructor. */
    ~SecureBasicAuthenticator();

    /** Verify the authentication of the requesting user.
        @param userName String containing the user name
        @param password String containing the user password
        @return true on successful authentication, false otherwise
    */
    Boolean authenticate(
        const String& userName,
        const String& password);

    /**
        Verify whether the user is valid.
        @param userName String containing the user name
        @return true on successful validation, false otherwise
    */
    Boolean validateUser(const String& userName);

    /** Construct and return the HTTP Basic authentication challenge header
        @return A string containing the authentication challenge header.
    */
    String getAuthResponseHeader();

private:

#ifdef PEGASUS_OS_ZOS
#if (__TARGET_LIB__ >= 0x410A0000)

    String        _zosAPPLID;

#elif defined(PEGASUS_PLATFORM_ZOS_ZSERIES_IBM)

    /** Set the applicatoin ID to CFZAPPL for validatition of passtickes.
        This function is only needed if the compile target system is z/OS R9
        or earlier and 32 Bit !
         @return true on success.
    */
    Boolean set_ZOS_ApplicationID( void );
#endif // end __TARGET_LIB__
#endif // end PEGASUS_OS_ZOS

    String        _realm;
    UserManager*  _userManager;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_SecureBasicAuthenticator_h */
