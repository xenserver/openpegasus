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

#ifndef Pegasus_ExportClient_h
#define Pegasus_ExportClient_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/Monitor.h>
#include <Pegasus/Common/HTTPConnector.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Common/HTTPConnection.h>
#include <Pegasus/Common/MessageQueue.h>
#include <Pegasus/Common/CIMObject.h>
#include <Pegasus/Common/Exception.h>
#include <Pegasus/Common/ContentLanguageList.h>
#include <Pegasus/Common/SSLContext.h>
#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Client/ClientAuthenticator.h>
#include <Pegasus/ExportClient/Linkage.h>

PEGASUS_NAMESPACE_BEGIN

class Monitor;
class CIMExportResponseDecoder;
class CIMExportRequestEncoder;

/**
    This class provides the interface that a client uses to communicate
    with a CIMOM.
*/
class PEGASUS_EXPORT_CLIENT_LINKAGE CIMExportClient : public MessageQueue
{
public:

    /**
        Constructor for a CIM Export Client object.
    */
    CIMExportClient(
        Monitor* monitor,
        HTTPConnector* httpConnector,
        Uint32 timeoutMilliseconds =
            PEGASUS_DEFAULT_CLIENT_TIMEOUT_MILLISECONDS);

    // Destructor for a CIM Export Client object.
    ~CIMExportClient();

    /**
        Gets the timeout in milliseconds for the CIMExportClient.
        Default is 20 seconds (20000 milliseconds).
    */
    Uint32 getTimeout() const
    {
        return _timeoutMilliseconds;
    }

    /**
        Sets the timeout in milliseconds for the CIMExportClient.
        @param timeoutMilliseconds Defines the number of milliseconds the
        CIMExportClient will wait for a response to an outstanding request.
        If a request times out, the connection gets reset (disconnected and
        reconnected).  Default is 20 seconds (20000 milliseconds).
    */
    void setTimeout(Uint32 timeoutMilliseconds)
    {
        _timeoutMilliseconds = timeoutMilliseconds;
        if ((_connected) && (_httpConnection != 0))
        {
            _httpConnection->setSocketWriteTimeout(_timeoutMilliseconds/1000+1);
        }
    }

    /**
        Creates an HTTP connection with the server
        defined by the host and portNumber.
        @param host String defining the server to which the client should
        connect.
        @param portNumber Uint32 defining the port number for the server
        to which the client should connect.
        @exception AlreadyConnectedException
            If a connection has already been established.
        @exception InvalidLocatorException
            If the specified address is improperly formed.
        @exception CannotCreateSocketException
            If a socket cannot be created.
        @exception CannotConnectException
            If the socket connection fails.
    */
    void connect(
        const String& host,
        const Uint32 portNumber);

    /**
        Creates an HTTP connection with a Listener defined by
        the host and portNumber.
        @param host String defining the hostname of the listener.
        @param portNumber Uint32 defining the port number of the listener.
        @param sslContext SSL context to use for this connection.
        @exception AlreadyConnectedException
            If a connection has already been established.
        @exception InvalidLocatorException
            If the specified address is improperly formed.
        @exception CannotCreateSocketException
            If a socket cannot be created.
        @exception CannotConnectException
            If the socket connection fails.
    */
    void connect(
        const String& host,
        const Uint32 portNumber,
        const SSLContext& sslContext);


    /**
        Closes the connection with the server if the connection
        was open, simply returns if the connection was not open. Clients are
        expected to use this method to close the open connection before
        opening a new connection.
    */
    void disconnect();

    /**
        Send indication message to the destination where the url input
        parameter defines the destination.

        @param url String defining the destination of the indication to be sent.
        @param instance CIMInstance is the indication instance which needs to
        be sent to the destination.
        @param contentLanguages The language of the indication
    */
    virtual void exportIndication(
        const String& url,
        const CIMInstance& instance,
        const ContentLanguageList& contentLanguages = ContentLanguageList());

private:

    void _connect();
    void _disconnect();

    Message* _doRequest(
        CIMRequestMessage* request,
        MessageType expectedResponseMessageType);

    Monitor* _monitor;
    HTTPConnector* _httpConnector;
    HTTPConnection* _httpConnection;

    Uint32 _timeoutMilliseconds;
    Boolean _connected;
    /**
        The CIMExportClient uses a lazy reconnect algorithm.  A reconnection
        is necessary when the server (listener) sends a Connection: Close
        header in the HTTP response or when a connection timeout occurs
        while waiting for a response.  In these cases, a disconnect is
        performed immediately and the _doReconnect flag is set.  The
        connection is re-established only when required to perform another
        operation.  Note that in the case of a connection timeout, the
        challenge status must be reset in the ClientAuthenticator to allow
        authentication to be performed properly on the new connection.
    */
    Boolean _doReconnect;

    CIMExportResponseDecoder* _responseDecoder;
    CIMExportRequestEncoder* _requestEncoder;
    ClientAuthenticator _authenticator;
    String _connectHost;
    Uint32 _connectPortNumber;
    AutoPtr<SSLContext> _connectSSLContext;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_ExportClient_h */
