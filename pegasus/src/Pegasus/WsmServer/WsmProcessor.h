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

#ifndef Pegasus_WsmProcessor_h
#define Pegasus_WsmProcessor_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/MessageQueue.h>
#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Repository/CIMRepository.h>
#include <Pegasus/WsmServer/WsmRequestDecoder.h>
#include <Pegasus/WsmServer/WsmResponseEncoder.h>
#include <Pegasus/WsmServer/WsmToCimRequestMapper.h>
#include <Pegasus/WsmServer/CimToWsmResponseMapper.h>
#include <Pegasus/WsmServer/Linkage.h>

PEGASUS_NAMESPACE_BEGIN

class EnumerationContext
{
public:
    EnumerationContext() {}
    EnumerationContext(
        Uint64 contextId_,
        const String& userName_,
        WsenEnumerationMode enumerationMode_,
        CIMDateTime expiration_,
        WsmEndpointReference epr_,
        WsenEnumerateResponse* response_)
        : contextId(contextId_),
          userName(userName_),
          enumerationMode(enumerationMode_),
          expiration(expiration_),
          epr(epr_),
          response(response_) {}

    Uint64 contextId;
    String userName;
    WsenEnumerationMode enumerationMode;
    CIMDateTime expiration;
    WsmEndpointReference epr;
    WsenEnumerateResponse* response;
};


PEGASUS_TEMPLATE_SPECIALIZATION struct HashFunc<Uint64>
{
    static Uint32 hash(Uint64 x) { return (Uint32) x + 13; }
};


/**
    Processes WsmRequest messages and produces WsmResponse messages.
*/
class PEGASUS_WSMSERVER_LINKAGE WsmProcessor : public MessageQueue
{
public:

    WsmProcessor(
        MessageQueue* cimOperationProcessorQueue,
        CIMRepository* repository);

    ~WsmProcessor();

    virtual void handleEnqueue(Message *);

    virtual void handleEnqueue();

    void handleRequest(WsmRequest* wsmRequest);
    void handleResponse(CIMResponseMessage* cimResponse);

    void sendResponse(WsmResponse* wsmResponse);

    Uint32 getWsmRequestDecoderQueueId();

    void setServerTerminating(Boolean flag)
    {
        _wsmRequestDecoder.setServerTerminating(flag);
    }

    void cleanupExpiredContexts();

private:

    void _handlePullRequest(WsenPullRequest* wsmRequest);
    void _handleReleaseRequest(WsenReleaseRequest* wsmRequest);
    void _handleEnumerateResponse(
        CIMResponseMessage* cimResponse,
        WsenEnumerateRequest* wsmRequest);
    void _handleDefaultResponse(
        CIMResponseMessage* cimResponse,
        WsmRequest* wsmRequest);
    WsenEnumerateResponse* _splitEnumerateResponse(
        WsenEnumerateRequest* request,
        WsenEnumerateResponse* response,
        Uint32 num);
    WsenPullResponse* _splitPullResponse(
        WsenPullRequest* request,
        WsenEnumerateResponse* response,
        Uint32 num);
    void _getExpirationDatetime(const String& wsmDT, CIMDateTime& cimDT);

    WsmResponseEncoder _wsmResponseEncoder;
    WsmRequestDecoder _wsmRequestDecoder;

    /**
        A pointer to a CIMOperationRequestDispatcher that can be used to
        process CIM operation requests.
    */
    MessageQueue* _cimOperationProcessorQueue;

    /**
        A repository object that can be used to look up schema definitions.
    */
    CIMRepository* _repository;

    WsmToCimRequestMapper _wsmToCimRequestMapper;
    CimToWsmResponseMapper _cimToWsmResponseMapper;

    typedef HashTable<String,
        WsmRequest*, EqualFunc<String>, HashFunc<String> > RequestTable;
    /**
        The RequestTable stores the original WS-Management request until the
        operation is complete.  It is used to create an appropriate
        WS-Management response from the CIM operation response message.
        A unique message ID is used for CIM operation messages, different from
        the WS-Management request message ID.  The CIM operation message ID
        is used as the hash key.
    */
    RequestTable _requestTable;

    typedef HashTable<Uint64, EnumerationContext,
        EqualFunc<Uint64>, HashFunc<Uint64> > EnumerationContextTable;

    EnumerationContextTable _enumerationContextTable;
    Mutex _enumerationContextTableLock;
    static Uint64 _currentEnumContext;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_WsmProcessor_h */
