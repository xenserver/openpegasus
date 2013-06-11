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


#include <Pegasus/Common/AuditLogger.h>
#include <Pegasus/Common/Logger.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/Base64.h>
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Config/ConfigManager.h>

#include "SecureBasicAuthenticator.h"
#include "PAMBasicAuthenticator.h"
#include "BasicAuthenticationHandler.h"
#include "AuthenticationManager.h"

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN


BasicAuthenticationHandler::BasicAuthenticationHandler()
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "BasicAuthenticationHandler::BasicAuthenticationHandler()");

#ifdef PEGASUS_PAM_AUTHENTICATION
    _basicAuthenticator = (BasicAuthenticator*) new PAMBasicAuthenticator();
#else
    _basicAuthenticator = (BasicAuthenticator*) new SecureBasicAuthenticator();
#endif

    PEG_METHOD_EXIT();
}

BasicAuthenticationHandler::~BasicAuthenticationHandler()
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "BasicAuthenticationHandler::~BasicAuthenticationHandler()");

    delete _basicAuthenticator;

    PEG_METHOD_EXIT();
}

Boolean BasicAuthenticationHandler::authenticate(
    const String& authHeader,
    AuthenticationInfo* authInfo)
{
    PEG_METHOD_ENTER(
        TRC_AUTHENTICATION, "BasicAuthenticationHandler::authenticate()");

    Boolean authenticated = false;

    //
    // copy userPass string to char array for decoding
    //
    Buffer userPassArray;

    Uint32 length = authHeader.size();

    userPassArray.reserveCapacity( length );
    userPassArray.clear();

    for( Uint32 i = 0; i < length; i++ )
    {
        userPassArray.append( static_cast<char>(authHeader[i]) );
    }

    //
    // base64 decode the userPass array
    //
    Buffer  decodedArray;

    decodedArray = Base64::decode( userPassArray );

    String decodedStr =
        String( (const char*)decodedArray.getData(), decodedArray.size() );

    Uint32 pos = decodedStr.find(':');

    if (pos == PEG_NOT_FOUND)
    {
        PEG_METHOD_EXIT();
        return (authenticated);
    }

    String userName = decodedStr.subString(0, pos);

    String password = decodedStr.subString(pos + 1);

    Uint32 userNameLen = userName.size();
    if (userNameLen > PEGASUS_MAX_USER_NAME_LEN)
    {
        String badUserName = userName.subString(0, PEGASUS_MAX_USER_NAME_LEN);

        Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
            Logger::INFORMATION,
            MessageLoaderParms(
                BASIC_AUTHENTICATION_FAILED_KEY,
                BASIC_AUTHENTICATION_FAILED, badUserName,
                authInfo->getIpAddress()));
        PEG_METHOD_EXIT();
        return false;
    }

    // PASE APIs require user profile to be uppercase
#ifdef PEGASUS_OS_PASE
    for (Uint32 i = 0; i < userNameLen; i++)
    {
        userName[i] = toupper(userName[i]);
    }
#endif

#ifdef PEGASUS_WMIMAPPER
    authenticated = true;

    authInfo->setAuthenticatedUser(userName);
    authInfo->setAuthenticatedPassword(password);
#else

    if (!AuthenticationManager::isRemotePrivilegedUserAccessAllowed(userName))
    {
        return false;
    }
    authInfo->setRemotePrivilegedUserAccessChecked();

    authenticated = _basicAuthenticator->authenticate(userName, password);

    // Log audit message.
    PEG_AUDIT_LOG(logBasicAuthentication(
        userName,
        authInfo->getIpAddress(),
        authenticated));

    if (authenticated)
    {
        authInfo->setAuthenticatedUser(userName);
    }
    else
    {
        //
        //  Log a message for basic authentication failure
        //
        Logger::put_l(Logger::STANDARD_LOG, System::CIMSERVER,
            Logger::INFORMATION,
            MessageLoaderParms(
                BASIC_AUTHENTICATION_FAILED_KEY,
                BASIC_AUTHENTICATION_FAILED, userName,
                authInfo->getIpAddress()));
    }
#endif

    PEG_METHOD_EXIT();

    return (authenticated);
}

Boolean BasicAuthenticationHandler::validateUser(const String& userName)
{
    return _basicAuthenticator->validateUser(userName);
}

String BasicAuthenticationHandler::getAuthResponseHeader(
    const String& authType,
    const String& userName,
    AuthenticationInfo* authInfo)
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "BasicAuthenticationHandler::getAuthResponseHeader()");

    String respHeader = _basicAuthenticator->getAuthResponseHeader();

    PEG_METHOD_EXIT();

    return (respHeader);
}

PEGASUS_NAMESPACE_END
