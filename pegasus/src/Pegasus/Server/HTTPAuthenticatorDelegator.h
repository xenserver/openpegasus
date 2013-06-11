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

#ifndef Pegasus_HTTPAuthenticatorDelegator_h
#define Pegasus_HTTPAuthenticatorDelegator_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/MessageQueue.h>
#include <Pegasus/Common/HTTPAcceptor.h>
#include <Pegasus/Common/HTTPConnection.h>
#include <Pegasus/Common/HTTPMessage.h>
#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Security/Authentication/AuthenticationManager.h>
#include <Pegasus/Server/Linkage.h>

#include <Pegasus/Repository/CIMRepository.h>

PEGASUS_NAMESPACE_BEGIN

/**
   This class parses the HTTP header in the HTTPMessage passed to it and
   performs authentication based on the authentication specified in the
   configuration. It sends the challenge for unauthorized requests and
   validates the response.
*/
class PEGASUS_SERVER_LINKAGE HTTPAuthenticatorDelegator :
    public MessageQueue
{
public:

    typedef MessageQueue Base;

    /** Constructor. */
    HTTPAuthenticatorDelegator(
        Uint32 cimOperationMessageQueueId,
        Uint32 cimExportMessageQueueId,
        CIMRepository* repository);

    /** Destructor. */
    ~HTTPAuthenticatorDelegator();

    /**
        This method is overridden here to force the message to be handled
        by the same thread that enqueues it.  See Bugzilla 641 for details.
     */
    virtual void enqueue(Message* message);

    virtual void handleEnqueue(Message*);
    virtual void handleEnqueue();

    void handleHTTPMessage(HTTPMessage* httpMessage, Boolean& deleteMessage);

    void setWsmQueueId(Uint32 wsmanOperationMessageQueueId)
    {
        _wsmanOperationMessageQueueId = wsmanOperationMessageQueueId;
    }

private:

    void _sendResponse(
        Uint32 queueId,
        Buffer& message,
        Boolean closeConnect);

#ifdef PEGASUS_KERBEROS_AUTHENTICATION
    void _sendSuccess(
        Uint32 queueId,
        const String& authResponse,
        Boolean closeConnect);
#endif

    void _sendChallenge(
        Uint32 queueId,
        const String& authResponse,
        Boolean closeConnect);

    void _sendHttpError(
        Uint32 queueId,
        const String& status,
        const String& cimError = String::EMPTY,
        const String& pegasusError = String::EMPTY,
        Boolean closeConnect = false);

    Uint32 _cimOperationMessageQueueId;
    Uint32 _cimExportMessageQueueId;
    Uint32 _wsmanOperationMessageQueueId;

    AutoPtr<AuthenticationManager> _authenticationManager;

private:
      CIMRepository* _repository;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_HTTPAuthenticatorDelegator_h */
