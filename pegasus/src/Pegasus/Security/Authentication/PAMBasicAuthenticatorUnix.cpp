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

#include <Pegasus/Common/Executor.h>
#include <Pegasus/Config/ConfigManager.h>
#include <Pegasus/Common/Tracer.h>
#include "PAMBasicAuthenticator.h"

PEGASUS_USING_STD;

#if USEXENAPIAUTH
#include <xen/api/xen_all.h>
#include <dlfcn.h>
//
// The following is necessary to allow pegasus to authenticate against
// xapi rather than delegate to the PAM auth handlers. This will allow
// pegasus to take advantage of the AD integration features in xen and
// is necessary if 3rd party providers are installed side by side.
//
/* This comes from the XenServer-CIM source code */
extern "C" {
typedef struct 
{
    xen_session *xen;
    int socket;
    xen_host host;
} xen_utils_session;

void (*xen_utils_xen_close)();
int (*xen_utils_xen_init)();
int (*xen_utils_cleanup_session)(xen_utils_session *session);
int (*xen_utils_get_session)(xen_utils_session **session, const char *user, const char *pw);
}

bool xenapi_authenticate(const char *username, const char *password)
{
    bool authenticated = false;
    void *xenlibhandle = NULL;

    /* to prevent a circular dependency on xen-cim RPMs, load the xen-cim library dynamically */
    xenlibhandle = dlopen("libXen_Support.so", RTLD_LOCAL | RTLD_LAZY);
    if(xenlibhandle == NULL)
        return false;
    *((void **)&xen_utils_xen_init) = dlsym(xenlibhandle, "xen_utils_xen_init");
    *((void **)&xen_utils_xen_close) = dlsym(xenlibhandle, "xen_utils_xen_close");
    *((void **)&xen_utils_cleanup_session) = dlsym(xenlibhandle, "xen_utils_cleanup_session");
    *((void **)&xen_utils_get_session) = dlsym(xenlibhandle, "xen_utils_get_session");

    (*xen_utils_xen_init)();
    xen_utils_session *session = NULL;
    /* This request will fail if made to the any host other than the pool master */
    if((*xen_utils_get_session)(&session, username, password) && session) {
        int i=0, j=0;
         authenticated = true;
         (*xen_utils_cleanup_session)(session);
    }
    (*xen_utils_xen_close)();

    dlclose(xenlibhandle);
    return authenticated;
}
#endif

PEGASUS_NAMESPACE_BEGIN

PAMBasicAuthenticator::PAMBasicAuthenticator()
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "PAMBasicAuthenticator::PAMBasicAuthenticator()");

    // Build Authentication parameter realm required for Basic Challenge
    // e.g. realm="HostName"

    _realm.assign("realm=");
    _realm.append(Char16('"'));
    _realm.append(System::getHostName());
    _realm.append(Char16('"'));

    PEG_METHOD_EXIT();
}

PAMBasicAuthenticator::~PAMBasicAuthenticator()
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "PAMBasicAuthenticator::~PAMBasicAuthenticator()");

    PEG_METHOD_EXIT();
}

Boolean PAMBasicAuthenticator::authenticate(
    const String& userName,
    const String& password)
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "PAMBasicAuthenticator::authenticate()");

#if USEXENAPIAUTH
    CString usercs = userName.getCString();
    CString passcs = password.getCString();
    const char* user = (const char *)usercs;
    const char* pass = (const char *)passcs;
    //we use xenapi to do the authentication (for AD authentication support and so on).
    if(!xenapi_authenticate(user, pass))
        return false;
#else
    if (Executor::authenticatePassword(
        userName.getCString(), password.getCString()) != 0)
    {
        return false;
    }
#endif

    PEG_METHOD_EXIT();
    return true;
}

Boolean PAMBasicAuthenticator::validateUser(const String& userName)
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "PAMBasicAuthenticator::validateUser()");

    if (Executor::validateUser(userName.getCString()) != 0)
        return false;

    PEG_METHOD_EXIT();
    return true;
}


String PAMBasicAuthenticator::getAuthResponseHeader()
{
    PEG_METHOD_ENTER(TRC_AUTHENTICATION,
        "PAMBasicAuthenticator::getAuthResponseHeader()");

    // Build response header: WWW-Authenticate: Basic realm="HostName"

    String responseHeader = "WWW-Authenticate: Basic ";
    responseHeader.append(_realm);

    PEG_METHOD_EXIT();
    return responseHeader;
}

PEGASUS_NAMESPACE_END
