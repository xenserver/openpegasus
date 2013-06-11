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

#if !defined(PEGASUS_OS_ZOS) && \
    !defined(PEGASUS_OS_DARWIN)
# include <crypt.h>
#endif

PEGASUS_NAMESPACE_BEGIN

Boolean System::canRead(const char* path)
{
    return access(path, R_OK) == 0;
}

Boolean System::canWrite(const char* path)
{
    return access(path, W_OK) == 0;
}

String System::getPassword(const char* prompt)
{
#if  defined(PEGASUS_OS_PASE)

    char* umepass = umeGetPass();
    if(NULL == umepass)
    {
        return String::EMPTY;
    }
    else
    {
        return String(umepass);
    }

#else /* default */

    return String(getpass(prompt));

#endif /* default */

}

String System::encryptPassword(const char* password, const char* salt)
{
    return String(crypt(password, salt));
}

Boolean System::isPrivilegedUser(const String& userName)
{
#if defined(PEGASUS_OS_PASE)
    CString user = userName.getCString();
    // this function only can be found in PASE environment
    return umeIsPrivilegedUser((const char *)user);

#else
    struct passwd   pwd;
    struct passwd   *result;
    const unsigned int PWD_BUFF_SIZE = 1024;
    char            pwdBuffer[PWD_BUFF_SIZE];

    if (getpwnam_r(
          userName.getCString(), &pwd, pwdBuffer, PWD_BUFF_SIZE, &result) != 0)
    {
        PEG_TRACE((
            TRC_OS_ABSTRACTION,
            Tracer::LEVEL1,
            "getpwnam_r failure : %s",
            strerror(errno)));
    }

    // Check if the requested entry was found. If not return false.
    if ( result != NULL )
    {
        // Check if the uid is 0.
        if ( pwd.pw_uid == 0 )
        {
            return true;
        }
    }
    return false;
#endif
}

#if defined(PEGASUS_ENABLE_USERGROUP_AUTHORIZATION)

Boolean System::isGroupMember(const char* userName, const char* groupName)
{
    struct group grp;
    char* member;
    Boolean retVal = false;
    const unsigned int PWD_BUFF_SIZE = 1024;
    const unsigned int GRP_BUFF_SIZE = 1024;
    struct passwd pwd;
    struct passwd* result;
    struct group* grpresult;
    char pwdBuffer[PWD_BUFF_SIZE];
    char grpBuffer[GRP_BUFF_SIZE];

    // Search Primary group information.

    // Find the entry that matches "userName"

    if (getpwnam_r(userName, &pwd, pwdBuffer, PWD_BUFF_SIZE, &result) != 0)
    {
        String errorMsg = String("getpwnam_r failure : ") +
                            String(strerror(errno));
        Logger::put(Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
                                  errorMsg);
        throw InternalSystemError();
    }

    if ( result != NULL )
    {
        // User found, check for group information.
        gid_t           group_id;
        group_id = pwd.pw_gid;

        // Get the group name using group_id and compare with group passed.
        if ( getgrgid_r(group_id, &grp,
                 grpBuffer, GRP_BUFF_SIZE, &grpresult) != 0)
        {
            String errorMsg = String("getgrgid_r failure : ") +
                                 String(strerror(errno));
            Logger::put(
                Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING,
                errorMsg);
            throw InternalSystemError();
        }

        // Compare the user's group name to groupName.
        if (strcmp(grp.gr_name, groupName) == 0)
        {
             // User is a member of the group.
             return true;
        }
    }

    //
    // Search supplemental groups.
    // Get a user group entry
    //
    if (getgrnam_r((char *)groupName, &grp,
        grpBuffer, GRP_BUFF_SIZE, &grpresult) != 0)
    {
        String errorMsg = String("getgrnam_r failure : ") +
            String(strerror(errno));
        Logger::put(
            Logger::STANDARD_LOG, System::CIMSERVER, Logger::WARNING, errorMsg);
        throw InternalSystemError();
    }

    // Check if the requested group was found.
    if (grpresult == NULL)
    {
        return false;
    }

    Uint32 j = 0;

    //
    // Get all the members of the group
    //
    member = grp.gr_mem[j++];

    while (member)
    {
        //
        // Check if the user is a member of the group
        //
        if ( strcmp(userName, member) == 0 )
        {
            retVal = true;
            break;
        }
        member = grp.gr_mem[j++];
    }

    return retVal;
}

#endif /* PEGASUS_ENABLE_USERGROUP_AUTHORIZATION */


PEGASUS_NAMESPACE_END
