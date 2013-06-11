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
//%////////////////////////////////////////////////////////////////////////////

#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/HTTPAcceptor.h>
#include <Pegasus/Common/HTTPConnection.h>
#include <Pegasus/Common/HTTPMessage.h>
#include <Pegasus/Common/XmlWriter.h>
#include "HTTPAuthenticatorDelegator.h"

#ifdef PEGASUS_KERBEROS_AUTHENTICATION
#include <Pegasus/Common/CIMKerberosSecurityAssociation.h>
#endif

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

#ifdef PEGASUS_KERBEROS_AUTHENTICATION
/**
    Constant representing the Kerberos authentication challenge header.
*/
static const String KERBEROS_CHALLENGE_HEADER = "WWW-Authenticate: Negotiate ";
#endif

HTTPAuthenticatorDelegator::HTTPAuthenticatorDelegator(
    Uint32 cimOperationMessageQueueId,
    Uint32 cimExportMessageQueueId,
    CIMRepository* repository)
    : Base(PEGASUS_QUEUENAME_HTTPAUTHDELEGATOR, MessageQueue::getNextQueueId()),
      _cimOperationMessageQueueId(cimOperationMessageQueueId),
      _cimExportMessageQueueId(cimExportMessageQueueId),
      _wsmanOperationMessageQueueId(PEG_NOT_FOUND),
      _repository(repository)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::HTTPAuthenticatorDelegator");

    _authenticationManager.reset(new AuthenticationManager());

    PEG_METHOD_EXIT();
}

HTTPAuthenticatorDelegator::~HTTPAuthenticatorDelegator()
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::~HTTPAuthenticatorDelegator");


    PEG_METHOD_EXIT();
}

void HTTPAuthenticatorDelegator::enqueue(Message* message)
{
    handleEnqueue(message);
}

void HTTPAuthenticatorDelegator::_sendResponse(
    Uint32 queueId,
    Buffer& message,
    Boolean closeConnect)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::_sendResponse");

    MessageQueue* queue = MessageQueue::lookup(queueId);

    if (queue)
    {
        AutoPtr<HTTPMessage> httpMessage(new HTTPMessage(message));
        httpMessage->dest = queue->getQueueId();

        queue->enqueue(httpMessage.release());
    }

    PEG_METHOD_EXIT();
}

#ifdef PEGASUS_KERBEROS_AUTHENTICATION
void HTTPAuthenticatorDelegator::_sendSuccess(
    Uint32 queueId,
    const String& authResponse,
    Boolean closeConnect)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::_sendSuccess");

    //
    // build OK (200) response message
    //

    Array<Sint8> message;
    XmlWriter::appendOKResponseHeader(message, authResponse);

    _sendResponse(queueId, message);

    PEG_METHOD_EXIT();
}
#endif

void HTTPAuthenticatorDelegator::_sendChallenge(
    Uint32 queueId,
    const String& authResponse,
    Boolean closeConnect)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::_sendChallenge");

    //
    // build unauthorized (401) response message
    //

    Buffer message;
    XmlWriter::appendUnauthorizedResponseHeader(message, authResponse);

    _sendResponse(queueId, message, false);

    PEG_METHOD_EXIT();
}

void HTTPAuthenticatorDelegator::_sendHttpError(
    Uint32 queueId,
    const String& status,
    const String& cimError,
    const String& pegasusError,
    Boolean closeConnect)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::_sendHttpError");

    //
    // build error response message
    //

    Buffer message;
    message = XmlWriter::formatHttpErrorRspMessage(
        status,
        cimError,
        pegasusError);

    _sendResponse(queueId, message, false);

    PEG_METHOD_EXIT();
}


void HTTPAuthenticatorDelegator::handleEnqueue(Message *message)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::handleEnqueue");

    if (!message)
    {
        PEG_METHOD_EXIT();
        return;
    }

    // Flag indicating whether the message should be deleted after handling.
    // This should be set to false by handleHTTPMessage when the message is
    // passed as is to another queue.
    Boolean deleteMessage = true;

    if (message->getType() == HTTP_MESSAGE)
    {
        handleHTTPMessage((HTTPMessage*)message, deleteMessage);
    }

    if (deleteMessage)
    {
        delete message;
    }

    PEG_METHOD_EXIT();
}

void HTTPAuthenticatorDelegator::handleEnqueue()
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::handleEnqueue");

    Message* message = dequeue();
    if(message)
       handleEnqueue(message);

    PEG_METHOD_EXIT();
}

void HTTPAuthenticatorDelegator::handleHTTPMessage(
    HTTPMessage* httpMessage,
    Boolean & deleteMessage)
{
    PEG_METHOD_ENTER(TRC_HTTP,
        "HTTPAuthenticatorDelegator::handleHTTPMessage");

    Boolean authenticated = false;
    deleteMessage = true;

    // ATTN-RK-P3-20020408: This check probably shouldn't be necessary, but
    // we're getting an empty message when the client closes the connection
    if (httpMessage->message.size() == 0)
    {
        // The message is empty; just drop it
        PEG_METHOD_EXIT();
        return;
    }

    //
    // get the configured authentication flag
    //
    // WMI MAPPER SPECIFIC: AUTHENTICATION ALWAYS ENABLED:
    Boolean enableAuthentication = true;
    /* NORMAL METHOD OF LOOKING UP AUTHENTICATION CONFIGURATION:
    Boolean enableAuthentication = ConfigManager::parseBooleanValue(
        ConfigManager::getInstance()->getCurrentValue("enableAuthentication"));
    */

    //
    // Save queueId:
    //
    Uint32 queueId = httpMessage->queueId;

    //
    // Parse the HTTP message:
    //
    String startLine;
    Array<HTTPHeader> headers;
    Uint32 contentLength;

    httpMessage->parse(startLine, headers, contentLength);

    //
    // Parse the request line:
    //
    String methodName;
    String requestUri;
    String httpVersion;
    HttpMethod httpMethod = HTTP_METHOD__POST;

    HTTPMessage::parseRequestLine(
        startLine, methodName, requestUri, httpVersion);

    //
    //  Set HTTP method for the request
    //
    if (methodName == "M-POST")
    {
        httpMethod = HTTP_METHOD_M_POST;
    }

    if (methodName != "M-POST" && methodName != "POST")
    {
        // Only POST and M-POST are implemented by this server
        _sendHttpError(queueId,
                       HTTP_STATUS_NOTIMPLEMENTED);
    }
    else if ((httpMethod == HTTP_METHOD_M_POST) &&
             (httpVersion == "HTTP/1.0"))
    {
        //
        //  M-POST method is not valid with version 1.0
        //
        _sendHttpError(queueId,
                       HTTP_STATUS_BADREQUEST);
    }
    else
    {
        //
        // Process M-POST and POST messages:
        //
        PEG_TRACE_CSTRING(
            TRC_HTTP,
            Tracer::LEVEL3,
            "HTTPAuthenticatorDelegator - M-POST/POST processing start");

        //
        // Search for Authorization header:
        //
        String authorization;

        //
        // Do not require authentication for indication delivery
        //
        const char* cimExport;
        if (enableAuthentication &&
            HTTPMessage::lookupHeader(
                headers, "CIMExport", cimExport, true))
        {
            enableAuthentication = false;
        }

        if ( HTTPMessage::lookupHeader(
             headers, "PegasusAuthorization", authorization, false) &&
             enableAuthentication
           )
        {
            try
            {
                //
                // Do pegasus/local authentication
                //
                authenticated =
                    _authenticationManager->performPegasusAuthentication(
                        authorization,
                        httpMessage->authInfo);

                if (!authenticated)
                {
                    String authChallenge;
                    String authResp;

                    authResp =
                        _authenticationManager->getPegasusAuthResponseHeader(
                            authorization,
                            httpMessage->authInfo);

                    if (!String::equal(authResp, String::EMPTY))
                    {
                        _sendChallenge(queueId, authResp, false);
                    }
                    else
                    {
                        _sendHttpError(queueId,
                                       HTTP_STATUS_BADREQUEST,
                                       String::EMPTY,
                                       "Authorization header error");
                    }

                    PEG_METHOD_EXIT();
                    return;
                }
            }
            catch (CannotOpenFile &cof)
            {
                _sendHttpError(queueId,
                               HTTP_STATUS_INTERNALSERVERERROR);
                PEG_METHOD_EXIT();
                return;

            }
        }

        if ( HTTPMessage::lookupHeader(
             headers, "Authorization", authorization, false) &&
             enableAuthentication
           )
        {
#ifdef PEGASUS_KERBEROS_AUTHENTICATION
            // The presence of a Security Association indicates that Kerberos
            // is being used.
            CIMKerberosSecurityAssociation *sa =
                httpMessage->authInfo->getSecurityAssociation();
            if (sa)
            {
                sa->setClientSentAuthorization(true);
            }
#endif
            //
            // Do http authentication if not authenticated already
            //
            if (!authenticated)
            {
                authenticated =
                    _authenticationManager->performHttpAuthentication(
                        authorization,
                        httpMessage->authInfo);

                if (!authenticated)
                {
                    //ATTN: the number of challenges get sent for a
                    //      request on a connection can be pre-set.
#ifdef PEGASUS_KERBEROS_AUTHENTICATION
                    // Kerberos authentication needs access to the
                    // AuthenticationInfo object for this session in order
                    // to set up the reference to the
                    // CIMKerberosSecurityAssociation object for this session.

                    String authResp =
                        _authenticationManager->getHttpAuthResponseHeader(
                            httpMessage->authInfo);
#else
                    String authResp =
                        _authenticationManager->getHttpAuthResponseHeader();
#endif
                    if (!String::equal(authResp, String::EMPTY))
                    {
                        _sendChallenge(queueId, authResp, false);
                    }
                    else
                    {
                        _sendHttpError(queueId,
                                       HTTP_STATUS_BADREQUEST,
                                       String::EMPTY,
                                       "Authorization header error");
                    }

                    PEG_METHOD_EXIT();
                    return;
                }
            }  // first not authenticated check
        }  // "Authorization" header check

#ifdef PEGASUS_KERBEROS_AUTHENTICATION
        // The presence of a Security Association indicates that Kerberos is
        // being used.
        CIMKerberosSecurityAssociation *sa =
            httpMessage->authInfo->getSecurityAssociation();

        // This will process a request with no content.
        if (sa && authenticated)
        {
            if (sa->getServerToken().size())
            {
                // This will handle the scenario where client did not send
                // data (content) but is authenticated.  After the client
                // receives the success it should will send the request.
                // For mutual authentication the client may choose not to
                // send request data until the context is fully established.
                // Note:  if mutual authentication wass not requested by the
                // client then no server token will be available.
                if (contentLength == 0)
                {
                    String authResp =
                        _authenticationManager->getHttpAuthResponseHeader(
                            httpMessage->authInfo);
                    if (!String::equal(authResp, String::EMPTY))
                    {
                        _sendSuccess(queueId, authResp);
                    }
                    else
                    {
                        _sendHttpError(queueId,
                                       HTTP_STATUS_BADREQUEST,
                                       String::EMPTY,
                                       "Authorization header error");
                    }

                    PEG_METHOD_EXIT();
                    return;
                }
            }
        }

        // This will process a request without an authorization record.
        if (sa && !authenticated)
        {
            // Not authenticated can result from the client not sending an
            // authorization record due to a previous authentication.  In
            // this scenario the client was previous authenticated but
            // chose not to send an authorization record.  The Security
            // Association maintains state so a request will not be
            // processed unless the Security association thinks the client
            // is authenticated.

            // This will handle the scenario where the client was
            // authenticated in the previous request but choose not to
            // send an authorization record.
            if (sa->getClientAuthenticated())
            {
                authenticated = true;
            }
        }

        // The following is processing to unwrap (unencrypt) the message
        // received from the client when using kerberos authentication.
        // For Kerberos there should always be an "Authorization" record
        // sent with the request so the authenticated flag in the method
        // should always be checked to verify that the client is actually
        // authenticated.  If no "Authoriztion" was sent then the client
        // can't be authenticated.  The "Authorization" record was
        // processed above and if the "Authorization" record was
        // successfully processed then the client is authenticated.
        if (sa  &&  authenticated)
        {
            Uint32 rc;  // return code for the wrap
            Array<Sint8> final_buffer;
            final_buffer.clear();
            Array<Sint8> header_buffer;
            header_buffer.clear();
            Array<Sint8> inWrappedMessage;
            inWrappedMessage.clear();
            Array<Sint8> outUnwrappedMessage;
            outUnwrappedMessage.clear();

            // The message needs to be parsed in order to distinguish
            // between the headers and content. The message was parsed
            // earlier in this method so we can use the contentLength set
            // during that parse. When parsing the code breaks out of the
            // loop as soon as it finds the double separator that
            // terminates the headers so the headers and content can be
            // easily separated.

            // Extract the unwrapped headers
            for (Uint32 i = 0; i < httpMessage->message.size() - contentLength;
                 i++)
            {
                header_buffer.append(httpMessage->message[i]);
            }

            // Extract the wrapped content
            for (Uint32 i = httpMessage->message.size() - contentLength;
                 i < httpMessage->message.size(); i++)
            {
                inWrappedMessage.append(httpMessage->message[i]);
            }

            rc = sa->unwrap_message(inWrappedMessage,
                                    outUnwrappedMessage);  // ASCII

            if (rc)
            {
                // clear out the outUnwrappedMessage; if unwrapping is required
                // and it fails we need to send back a bad request.  A message
                // left in an wrapped state will be a severe failue later.
                // Reason for unwrap failing may be due to a problem with the
                // credentials or context or some other failure may have
                // occurred.
                outUnwrappedMessage.clear();
                // build a bad request.  We do not want to risk sending back
                // data that can't be authenticated.
                final_buffer =
                  XmlWriter::formatHttpErrorRspMessage(HTTP_STATUS_BADREQUEST);
                //remove the last separater; the authentication record still
                // needs to be added.
                final_buffer.remove(final_buffer.size());  // "\n"
                final_buffer.remove(final_buffer.size());  // "\r"

                // Build the WWW_Authenticate record with token.
                String authResp =
                  _authenticationManager->getHttpAuthResponseHeader(
                                httpMessage->authInfo);
                // error occurred on wrap so the final_buffer needs to be built
                // differently
                final_buffer << authResp;
                // add double separaters to end
                final_buffer << "\r\n";
                final_buffer << "\r\n";

                _sendResponse(queueId, final_buffer);
                PEG_METHOD_EXIT();
                return;
            }

            // If outUnwrappedMessage does not have any content data this
            // is a result of no data available.  This could be a result
            // of the client not sending any content data.  This is not
            // unique to Kerberos so this will not be handled here but it
            // will be handled when the content is processed. Or, the
            // client did not wrap message...this is okay.  If the unwrap
            // resulted in no data when there should have been data then
            // this should have been caught above when the unwrap returned
            // a bad return code
            if (final_buffer.size() == 0)
            {
                // if outUnwrappedMessage is empty that indicates client did
                // not wrap the message.  The original message will be used.
                if (outUnwrappedMessage.size())
                {
                    final_buffer.appendArray(header_buffer);
                    final_buffer.appendArray(outUnwrappedMessage);
                    // add back char that was appended earlier
                    final_buffer.append('\0');
                    // Note: if additional characters added
                    // in future add back here
                }
            }
            else
            {
                // The final buffer should not have any data at this point.
                // If it does end the server because something bad happened.
                PEG_TRACE_CSTRING(
                    TRC_HTTP,
                    Tracer::LEVEL2,
                    "HTTPAuthenticatorDelegator - the final buffer should "
                        "not have data");
                PEGASUS_ASSERT(0);
            }

            // replace the existing httpMessage with the headers and
            // unwrapped message
            // If the final buffer has not been set then the original
            // httpMessage will be used.
            if (final_buffer.size())
            {
                    httpMessage->message.clear();
                    httpMessage->message = final_buffer;
            }
        } // if not kerberos and client not authenticated
#endif

        if ( authenticated || !enableAuthentication )
        {
            //
            // Search for "CIMOperation" header:
            //
            const char* cimOperation;

            if (HTTPMessage::lookupHeader(
                    headers, "CIMOperation", cimOperation, true))
            {
                PEG_TRACE((
                    TRC_HTTP,
                    Tracer::LEVEL3,
                    "HTTPAuthenticatorDelegator - CIMOperation: %s",
                    cimOperation));

                MessageQueue* queue =
                    MessageQueue::lookup(_cimOperationMessageQueueId);

                if (queue)
                {
                   httpMessage->dest = queue->getQueueId();

                   try
                     {
                       queue->enqueue(httpMessage);
                     }
                   catch(exception & e)
                     {
                       delete httpMessage;
                       _sendHttpError(queueId,
                                      HTTP_STATUS_REQUEST_TOO_LARGE);
                       PEG_METHOD_EXIT();
                       deleteMessage = false;
                       return;
                     }
                 deleteMessage = false;
                }
            }
            else if (HTTPMessage::lookupHeader(
                         headers, "CIMExport", cimOperation, true))
            {
                PEG_TRACE((
                    TRC_HTTP,
                    Tracer::LEVEL3,
                    "HTTPAuthenticatorDelegator - CIMExport: $0",
                    cimOperation));

                MessageQueue* queue =
                    MessageQueue::lookup(_cimExportMessageQueueId);

                if (queue)
                {
                    httpMessage->dest = queue->getQueueId();

                    queue->enqueue(httpMessage);
                    deleteMessage = false;
                }
            }
            else
            {
                // We don't recognize this request message type

                // The Specification for CIM Operations over HTTP reads:
                //
                //     3.3.4. CIMOperation
                //
                //     If a CIM Server receives a CIM Operation request without
                //     this [CIMOperation] header, it MUST NOT process it as if
                //     it were a CIM Operation Request.  The status code
                //     returned by the CIM Server in response to such a request
                //     is outside of the scope of this specification.
                //
                //     3.3.5. CIMExport
                //
                //     If a CIM Listener receives a CIM Export request without
                //     this [CIMExport] header, it MUST NOT process it.  The
                //     status code returned by the CIM Listener in response to
                //     such a request is outside of the scope of this
                //     specification.
                //
                // The author has chosen to send a 400 Bad Request error, but
                // without the CIMError header since this request must not be
                // processed as a CIM request.

                _sendHttpError(queueId,
                               HTTP_STATUS_BADREQUEST);
                PEG_METHOD_EXIT();
                return;
            } // bad request
        } // authenticated and enableAuthentication check
        else
        {  // client not authenticated; send challenge
#ifdef PEGASUS_KERBEROS_AUTHENTICATION
            String authResp =
                _authenticationManager->getHttpAuthResponseHeader(
                    httpMessage->authInfo);
#else
            String authResp =
                _authenticationManager->getHttpAuthResponseHeader();
#endif

            if (!String::equal(authResp, String::EMPTY))
            {
                _sendChallenge(queueId, authResp, false);
            }
            else
            {
                _sendHttpError(queueId,
                               HTTP_STATUS_BADREQUEST,
                               String::EMPTY,
                               "Authorization header error");
            }
        }
    } // M-POST and POST processing

    PEG_METHOD_EXIT();
}

PEGASUS_NAMESPACE_END
