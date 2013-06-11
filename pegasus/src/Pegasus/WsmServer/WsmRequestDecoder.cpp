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

#include <cctype>
#include <cstdio>
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/XmlParser.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/CommonUTF.h>
#include <Pegasus/Common/MessageLoader.h>
#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Common/SharedPtr.h>

#include "WsmConstants.h"
#include "WsmReader.h"
#include "WsmWriter.h"
#include "WsmProcessor.h"
#include "WsmRequestDecoder.h"

PEGASUS_NAMESPACE_BEGIN

static bool _parseInvokeAction(
    const String& action,
    String& className,
    String& methodName)
{
    // Parse the action as though it is a method invocation. If so, set
    // className and methodName and return true. Else return false. Invoke
    // actions have the following form:
    //
    //     http://schemas.dmtf.org/wbem/wscim/1/cim-schema/2/<CLASS>/<METHOD>

    // Expect "http://" prefix.

    CString cstr = action.getCString();
    const char* p = cstr;

    if (strncmp(p, "http://", 7) != 0)
        return false;

    p += 7;

    // Find slash that terminates the host name.

    if (!(p = strchr(p, '/')))
        return false;

    p++;

    // Expect "wbem/wscim/1/cim-schema/2/" sequence.

    if (strncmp(p, "wbem/wscim/1/cim-schema/2/", 26) != 0)
        return false;

    p += 26;

    // Get classname:

    char* slash = strchr(const_cast<char*>(p), '/');

    if (!slash)
        return false;

    *slash = '\0';
    className = p;
    *slash = '/';
    p = slash + 1;

    // Get methodname:

    methodName = p;

    // If we got this far, then action refers to a method.
    return true;
}

WsmRequestDecoder::WsmRequestDecoder(WsmProcessor* wsmProcessor)
    : MessageQueue(PEGASUS_QUEUENAME_WSMREQDECODER),
      _wsmProcessor(wsmProcessor),
      _serverTerminating(false)
{
}

WsmRequestDecoder::~WsmRequestDecoder()
{
}

void WsmRequestDecoder::sendResponse(
    Uint32 queueId,
    Buffer& message,
    Boolean httpCloseConnect)
{
    MessageQueue* queue = MessageQueue::lookup(queueId);

    if (queue)
    {
        AutoPtr<HTTPMessage> httpMessage(new HTTPMessage(message));
        httpMessage->setCloseConnect(httpCloseConnect);
        queue->enqueue(httpMessage.release());
    }
}

void WsmRequestDecoder::sendHttpError(
    Uint32 queueId,
    const String& status,
    const String& cimError,
    const String& pegasusError,
    Boolean httpCloseConnect)
{
    Buffer message;
    message = WsmWriter::formatHttpErrorRspMessage(
        status,
        cimError,
        pegasusError);

    sendResponse(queueId, message, httpCloseConnect);
}

void WsmRequestDecoder::handleEnqueue(Message* message)
{
    PEGASUS_ASSERT(message);
    PEGASUS_ASSERT(message->getType() == HTTP_MESSAGE);

    handleHTTPMessage((HTTPMessage*)message);

    delete message;
}

void WsmRequestDecoder::handleEnqueue()
{
    Message* message = dequeue();
    if (message)
        handleEnqueue(message);
}

//-----------------------------------------------------------------------------
//
// From the HTTP/1.1 Specification (RFC 2626):
//
// Both types of message consist of a start-line, zero or more header fields
// (also known as "headers"), an empty line (i.e., a line with nothing
// preceding the CRLF) indicating the end of the header fields, and possibly
// a message-body.
//
//-----------------------------------------------------------------------------
void WsmRequestDecoder::handleHTTPMessage(HTTPMessage* httpMessage)
{
    PEG_METHOD_ENTER(TRC_WSMSERVER, "WsmRequestDecoder::handleHTTPMessage()");

    // Set the Accept-Language into the thread for this service.
    // This will allow all code in this thread to get
    // the languages for the messages returned to the client.
    Thread::setLanguages(httpMessage->acceptLanguages);

    // Save queueId:
    Uint32 queueId = httpMessage->queueId;

    // Save userName and authType:
    String userName;
    String authType;
    Boolean httpCloseConnect = httpMessage->getCloseConnect();

    PEG_TRACE((TRC_WSMSERVER, Tracer::LEVEL4,
        "WsmRequestDecoder::handleHTTPMessage()- "
            "httpMessage->getCloseConnect() returned %d",
        httpCloseConnect));

    userName = httpMessage->authInfo->getAuthenticatedUser();
    authType = httpMessage->authInfo->getAuthType();

    // Parse the HTTP message:
    String startLine;
    Array<HTTPHeader> headers;
    char* content;
    Uint32 contentLength;

    httpMessage->parse(startLine, headers, contentLength);

    // Parse the request line:
    String methodName;
    String requestUri;
    String httpVersion;
    HttpMethod httpMethod = HTTP_METHOD__POST;

    HTTPMessage::parseRequestLine(
        startLine, methodName, requestUri, httpVersion);

    //  Set HTTP method for the request
    if (methodName == "M-POST")
    {
        httpMethod = HTTP_METHOD_M_POST;
    }

    // Unsupported methods are caught in the HTTPAuthenticatorDelegator
    PEGASUS_ASSERT(methodName == "M-POST" || methodName == "POST");

    //  Mismatch of method and version is caught in HTTPAuthenticatorDelegator
    PEGASUS_ASSERT(!((httpMethod == HTTP_METHOD_M_POST) &&
                     (httpVersion == "HTTP/1.0")));

    // Process M-POST and POST messages:
    if (httpVersion == "HTTP/1.1")
    {
        // Validate the presence of a "Host" header.  The HTTP/1.1
        // specification says this in section 14.23 regarding the Host
        // header field:
        //
        //     All Internet-based HTTP/1.1 servers MUST respond with a 400 (Bad
        //     Request) status code to any HTTP/1.1 request message which lacks
        //     a Host header field.
        //
        // Note:  The Host header value is not validated.

        const char* hostHeader;
        Boolean hostHeaderFound = HTTPMessage::lookupHeader(
            headers, "Host", hostHeader, false);

        if (!hostHeaderFound)
        {
            MessageLoaderParms parms(
                "Server.WsmRequestDecoder.MISSING_HOST_HEADER",
                "HTTP request message lacks a Host header field.");
            sendHttpError(
                queueId,
                HTTP_STATUS_BADREQUEST,
                "",
                MessageLoader::getMessage(parms),
                httpCloseConnect);
            PEG_METHOD_EXIT();
            return;
        }
    }

    // Calculate the beginning of the content from the message size and
    // the content length.
    content = (char*) httpMessage->message.getData() +
        httpMessage->message.size() - contentLength;

    // Lookup HTTP "User-Agent" header. For example:
    //
    //     User-Agent: Microsoft WinRM Client
    //
    // If it contains "WinRM", then omit the XML processing instruction from
    // the response (first line of XML response). A typical XML processing
    // instruction looks like this:
    //
    //     <?xml version="1.0" encoding="utf-8"?>
    //
    // The WinRM user agent should never receive this line.

    Boolean omitXMLProcessingInstruction;
    {
        String value;

        if (HTTPMessage::lookupHeader(headers, "User-Agent", value, true) &&
            value.find("WinRM") != Uint32(-1))
        {
            omitXMLProcessingInstruction = true;
        }
        else
        {
            omitXMLProcessingInstruction = false;
        }
    }

    // Validate the "Content-Type" header:
    const char* contentType;
    Boolean contentTypeHeaderFound = HTTPMessage::lookupHeader(
        headers, "Content-Type", contentType, true);
    String type;
    String charset;

    if (!contentTypeHeaderFound ||
        !HTTPMessage::parseContentTypeHeader(contentType, type, charset) ||
        (!String::equalNoCase(type, "application/soap+xml") &&
         !String::equalNoCase(type, "text/xml")))
    {
        MessageLoaderParms parms(
            "Server.WsmRequestDecoder.CONTENTTYPE_SYNTAX_ERROR",
            "HTTP Content-Type header error.");
        sendHttpError(
            queueId,
            HTTP_STATUS_BADREQUEST,
            "",
            MessageLoader::getMessage(parms),
            httpCloseConnect);
        PEG_METHOD_EXIT();
        return;
    }

    if (String::equalNoCase(charset, "utf-16"))
    {
        // Reject utf-16 requests.
        WsmFault fault(
            WsmFault::wsman_EncodingLimit,
            "UTF-16 is not supported; Please use UTF-8",
            ContentLanguageList(),
            WSMAN_FAULTDETAIL_CHARACTERSET);
         _wsmProcessor->sendResponse(new WsmFaultResponse(
             String::EMPTY, queueId, httpMethod, httpCloseConnect,
                omitXMLProcessingInstruction, fault));
         PEG_METHOD_EXIT();
         return;
    }

    if (!String::equalNoCase(charset, "utf-8"))
    {
        // DSP0226 R13.1-5:  A service shall emit Responses using the same
        // encoding as the original request. If the service does not support
        // the requested encoding or cannot determine the encoding, it should
        // use UTF-8 encoding to return a wsman:EncodingLimit fault with the
        // following detail code:
        // http://schemas.dmtf.org/wbem/wsman/1/wsman/faultDetail/CharacterSet

        WsmFault fault(
            WsmFault::wsman_EncodingLimit,
            String::EMPTY,
            ContentLanguageList(),
            WSMAN_FAULTDETAIL_CHARACTERSET);
         _wsmProcessor->sendResponse(new WsmFaultResponse(
              String::EMPTY, queueId, httpMethod, httpCloseConnect,
              omitXMLProcessingInstruction, fault));
         PEG_METHOD_EXIT();
         return;
    }

    // SoapAction header is optional, but if present, it must match
    // the content of <wsa:Action>
    String soapAction;
    HTTPMessage::lookupHeader(headers, "SOAPAction", soapAction, true);

    // Remove the quotes around the SOAPAction value
    if ((soapAction.size() > 1) &&
        (soapAction[0] == '\"') &&
        (soapAction[soapAction.size()-1] == '\"'))
    {
        soapAction = soapAction.subString(1, soapAction.size() - 2);
    }

    // Validating content falls within UTF8
    // (required to be compliant with section C12 of Unicode 4.0 spec,
    // chapter 3.)
    Uint32 count = 0;
    while (count < contentLength)
    {
        if (!(isUTF8((char*) &content[count])))
        {
            MessageLoaderParms parms(
                "Server.WsmRequestDecoder.INVALID_UTF8_CHARACTER",
                "Invalid UTF-8 character detected.");
            sendHttpError(
                queueId,
                HTTP_STATUS_BADREQUEST,
                "request-not-valid",
                MessageLoader::getMessage(parms),
                httpCloseConnect);

            PEG_METHOD_EXIT();
            return;
        }
        UTF8_NEXT(content, count);
    }

    handleWsmMessage(
        queueId,
        httpMethod,
        content,
        contentLength,
        soapAction,
        authType,
        userName,
        httpMessage->ipAddress,
        httpMessage->acceptLanguages,
        httpMessage->contentLanguages,
        httpCloseConnect,
        omitXMLProcessingInstruction);

    PEG_METHOD_EXIT();
}

void WsmRequestDecoder::handleWsmMessage(
    Uint32 queueId,
    HttpMethod httpMethod,
    char* content,
    Uint32 contentLength,
    String& soapAction,
    const String& authType,
    const String& userName,
    const String& ipAddress,
    const AcceptLanguageList& httpAcceptLanguages,
    const ContentLanguageList& httpContentLanguages,
    Boolean httpCloseConnect,
    Boolean omitXMLProcessingInstruction)
{
    PEG_METHOD_ENTER(TRC_WSMSERVER, "WsmRequestDecoder::handleWsmMessage()");


    // If CIMOM is shutting down, return "Service Unavailable" response
    if (_serverTerminating)
    {
        MessageLoaderParms parms(
            "Server.WsmRequestDecoder.CIMSERVER_SHUTTING_DOWN",
            "CIM Server is shutting down.");
        sendHttpError(
            queueId,
            HTTP_STATUS_SERVICEUNAVAILABLE,
            String::EMPTY,
            MessageLoader::getMessage(parms),
            httpCloseConnect);
        PEG_METHOD_EXIT();
        return;
    }

    WsmReader wsmReader(content);
    XmlEntry entry;
    AutoPtr<WsmRequest> request;
    String wsaMessageId;

    // Process <?xml ... >
    try
    {
        // These values are currently unused
        const char* xmlVersion = 0;
        const char* xmlEncoding = 0;

        // Note: WinRM does not send an XML declaration in its requests.
        // This return value is ignored.
        wsmReader.getXmlDeclaration(xmlVersion, xmlEncoding);

        // Decode the SOAP envelope

        wsmReader.expectStartTag(
            entry, WsmNamespaces::SOAP_ENVELOPE, "Envelope");

        String wsaAction;
        String wsaFrom;
        String wsaReplyTo;
        String wsaFaultTo;
        WsmEndpointReference epr;
        Uint32 wsmMaxEnvelopeSize = 0;
        AcceptLanguageList wsmLocale;
        Boolean wsmRequestEpr = false;
        Boolean wsmRequestItemCount = false;

        try
        {
            wsmReader.decodeRequestSoapHeaders(
                wsaMessageId,
                epr.address,
                wsaAction,
                wsaFrom,
                wsaReplyTo,
                wsaFaultTo,
                epr.resourceUri,
                *epr.selectorSet,
                wsmMaxEnvelopeSize,
                wsmLocale,
                wsmRequestEpr,
                wsmRequestItemCount);
        }
        catch (XmlException&)
        {
            // Do not treat this as an InvalidMessageInformationHeader fault.
            throw;
        }
        catch (Exception& e)
        {
            throw WsmFault(
                WsmFault::wsa_InvalidMessageInformationHeader,
                e.getMessage(),
                e.getContentLanguages());
        }

        // If no "Action" header was found, then this might still be a legal
        // identify request.

        if (wsaAction.size() == 0 && _isIdentifyRequest(wsmReader))
        {
            _sendIdentifyResponse(queueId);
            return;
        }

        // Set the Locale language into the thread for processing this request.
        Thread::setLanguages(wsmLocale);

        _checkRequiredHeader("wsa:To", epr.address.size());
        _checkRequiredHeader("wsa:MessageID", wsaMessageId.size());
        _checkRequiredHeader("wsa:Action", wsaAction.size());

        if (soapAction.size() && (soapAction != wsaAction))
        {
            throw WsmFault(
                WsmFault::wsa_MessageInformationHeaderRequired,
                MessageLoaderParms(
                    "WsmServer.WsmRequestDecoder.SOAPACTION_HEADER_MISMATCH",
                    "The HTTP SOAPAction header value \"$0\" does not match "
                        "the wsa:Action value \"$1\".",
                    soapAction,
                    wsaAction));
        }

        // Note: The wsa:To header is not validated.  DSP0226 section 5.3
        // indicates that this header is primarily useful for routing through
        // intermediaries.  The HTTPAuthenticatorDelegator examines the path
        // specified in the HTTP start line.

        // DSP0226 R5.3-1: The wsa:To header shall be present in all messages,
        // whether requests, responses, or events. In the absence of other
        // requirements, it is recommended that the network address for
        // resources that require authentication be suffixed by the token
        // sequence /wsman. If /wsman is used, unauthenticated access should
        // not be allowed.
        //     (1) <wsa:To> http://123.15.166.67/wsman </wsa:To>

        // DSP0226 R5.3-2: In the absence of other requirements, it is
        // recommended that the network address for resources that do not
        // require authentication be suffixed by the token sequence
        // /wsman-anon. If /wsman-anon is used, authenticated access shall
        // not be required.
        //     (1) <wsa:To> http://123.15.166.67/wsman-anon </wsa:To>

        if (wsaReplyTo != WSM_ADDRESS_ANONYMOUS)
        {
            // DSP0226 R5.4.2-2: A conformant service may require that all
            // responses be delivered over the same connection on which the
            // request arrives.
            throw WsmFault(
                WsmFault::wsman_UnsupportedFeature,
                MessageLoaderParms(
                    "WsmServer.WsmRequestDecoder.REPLYTO_ADDRESS_NOT_ANONYMOUS",
                    "Responses may only be delivered over the same connection "
                        "on which the request arrives."),
                WSMAN_FAULTDETAIL_ADDRESSINGMODE);
        }

        if (wsaFaultTo.size() && (wsaFaultTo != WSM_ADDRESS_ANONYMOUS))
        {
            // DSP0226 R5.4.3-3: A conformant service may require that all
            // faults be delivered to the client over the same transport or
            // connection on which the request arrives.
            throw WsmFault(
                WsmFault::wsman_UnsupportedFeature,
                MessageLoaderParms(
                    "WsmServer.WsmRequestDecoder.FAULTTO_ADDRESS_NOT_ANONYMOUS",
                    "Responses may only be delivered over the same connection "
                        "on which the request arrives."),
                WSMAN_FAULTDETAIL_ADDRESSINGMODE);
        }

        //
        // Parse the SOAP Body while decoding each action
        //

        String className;
        String methodName;

        if (wsaAction == WSM_ACTION_GET)
        {
            request.reset(_decodeWSTransferGet(
                wsmReader,
                wsaMessageId,
                epr));
        }
        else if (wsaAction == WSM_ACTION_PUT)
        {
            request.reset(_decodeWSTransferPut(
                wsmReader,
                wsaMessageId,
                epr));
        }
        else if (wsaAction == WSM_ACTION_CREATE)
        {
            request.reset(_decodeWSTransferCreate(
                wsmReader,
                wsaMessageId,
                epr));
        }
        else if (wsaAction == WSM_ACTION_DELETE)
        {
            request.reset(_decodeWSTransferDelete(
                wsmReader,
                wsaMessageId,
                epr));
        }
        else if (wsaAction == WSM_ACTION_ENUMERATE)
        {
            request.reset(_decodeWSEnumerationEnumerate(
                wsmReader,
                wsaMessageId,
                epr,
                wsmRequestItemCount));
        }
        else if (wsaAction == WSM_ACTION_PULL)
        {
            request.reset(_decodeWSEnumerationPull(
                wsmReader,
                wsaMessageId,
                epr,
                wsmRequestItemCount));
        }
        else if (wsaAction == WSM_ACTION_RELEASE)
        {
            request.reset(_decodeWSEnumerationRelease(
                wsmReader,
                wsaMessageId,
                epr));
        }
        else if (_parseInvokeAction(wsaAction, className, methodName))
        {
            request.reset(_decodeWSInvoke(
                wsmReader,
                wsaMessageId,
                epr,
                className,
                methodName));
        }
        else
        {
            throw WsmFault(
                WsmFault::wsa_ActionNotSupported,
                MessageLoaderParms(
                    "WsmServer.WsmRequestDecoder.ACTION_NOT_SUPPORTED",
                    "The wsa:Action value \"$0\" is not supported.",
                    wsaAction));
        }

        wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Envelope");

        request->authType = authType;
        request->userName = userName;
        request->ipAddress = ipAddress;
        request->httpMethod = httpMethod;
        // Note:  The HTTP Accept-Languages header is ignored
        request->acceptLanguages = wsmLocale;
        request->contentLanguages = httpContentLanguages;
        request->httpCloseConnect = httpCloseConnect;
        request->omitXMLProcessingInstruction = omitXMLProcessingInstruction;
        request->queueId = queueId;
        request->requestEpr = wsmRequestEpr;
        request->maxEnvelopeSize = wsmMaxEnvelopeSize;
    }
    catch (WsmFault& fault)
    {
        _wsmProcessor->sendResponse(new WsmFaultResponse(
            wsaMessageId, queueId, httpMethod, httpCloseConnect,
            omitXMLProcessingInstruction, fault));
        PEG_METHOD_EXIT();
        return;
    }
    catch (SoapNotUnderstoodFault& fault)
    {
        _wsmProcessor->sendResponse(new SoapFaultResponse(
            wsaMessageId, queueId, httpMethod, httpCloseConnect,
            omitXMLProcessingInstruction, fault));
        PEG_METHOD_EXIT();
        return;
    }
    catch (XmlException& e)
    {
        WsmFault fault(
            WsmFault::wsman_SchemaValidationError,
            e.getMessage(),
            e.getContentLanguages());
        _wsmProcessor->sendResponse(new WsmFaultResponse(
            wsaMessageId, queueId, httpMethod, httpCloseConnect,
            omitXMLProcessingInstruction, fault));
        PEG_METHOD_EXIT();
        return;
    }
    catch (Exception& e)
    {
        WsmFault fault(
            WsmFault::wsman_InternalError,
            e.getMessage(),
            e.getContentLanguages());
        _wsmProcessor->sendResponse(new WsmFaultResponse(
            wsaMessageId, queueId, httpMethod, httpCloseConnect,
            omitXMLProcessingInstruction, fault));
        PEG_METHOD_EXIT();
        return;
    }
    catch (const PEGASUS_STD(exception)& e)
    {
        WsmFault fault(WsmFault::wsman_InternalError, e.what());
        _wsmProcessor->sendResponse(new WsmFaultResponse(
            wsaMessageId, queueId, httpMethod, httpCloseConnect,
            omitXMLProcessingInstruction, fault));
        PEG_METHOD_EXIT();
        return;
    }
    catch (...)
    {
        WsmFault fault(WsmFault::wsman_InternalError);
        _wsmProcessor->sendResponse(new WsmFaultResponse(
            wsaMessageId, queueId, httpMethod, httpCloseConnect,
            omitXMLProcessingInstruction, fault));
        PEG_METHOD_EXIT();
        return;
    }

    _wsmProcessor->handleRequest(request.release());

    PEG_METHOD_EXIT();
}

void WsmRequestDecoder::_checkRequiredHeader(
    const char* headerName,
    Boolean headerSpecified)
{
    if (!headerSpecified)
    {
        throw WsmFault(
            WsmFault::wsa_MessageInformationHeaderRequired,
            MessageLoaderParms(
                "WsmServer.WsmRequestDecoder.MISSING_HEADER",
                "Required SOAP header \"$0\" was not specified.",
                headerName));
    }
}

void WsmRequestDecoder::_checkNoSelectorsEPR(const WsmEndpointReference& epr)
{
    // Make sure that at most __cimnamespace selector is present
    if (epr.selectorSet->selectors.size())
    {
        if (epr.selectorSet->selectors.size() > 1 ||
            epr.selectorSet->selectors[0].type != WsmSelector::VALUE ||
            epr.selectorSet->selectors[0].name != "__cimnamespace")
        {
            throw WsmFault(
                WsmFault::wsman_InvalidSelectors,
                MessageLoaderParms(
                    "WsmServer.WsmRequestDecoder.UNEXPECTED_SELECTORS",
                    "The operation allows only the __cimnamespace selector to "
                    "be present."),
                WSMAN_FAULTDETAIL_UNEXPECTEDSELECTORS);
        }
    }
}

WxfGetRequest* WsmRequestDecoder::_decodeWSTransferGet(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());

    XmlEntry entry;
    wsmReader.expectStartOrEmptyTag(
        entry, WsmNamespaces::SOAP_ENVELOPE, "Body");
    if (entry.type != XmlEntry::EMPTY_TAG)
    {
        wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");
    }

    return new WxfGetRequest(messageId, epr);
}

WxfPutRequest* WsmRequestDecoder::_decodeWSTransferPut(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());

    XmlEntry entry;
    wsmReader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Body");

    // The soap body must contain an XML representation of the updated instance
    WsmInstance instance;
    wsmReader.getInstanceElement(instance);

    wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");

    return new WxfPutRequest(messageId, epr, instance);
}

WxfCreateRequest* WsmRequestDecoder::_decodeWSTransferCreate(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());
    _checkNoSelectorsEPR(epr);

    XmlEntry entry;
    wsmReader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Body");

    // The soap body must contain an XML representation of the new instance
    WsmInstance instance;
    wsmReader.getInstanceElement(instance);

    wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");

    return new WxfCreateRequest(messageId, epr, instance);
}

WxfDeleteRequest* WsmRequestDecoder::_decodeWSTransferDelete(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());

    XmlEntry entry;
    wsmReader.expectStartOrEmptyTag(
        entry, WsmNamespaces::SOAP_ENVELOPE, "Body");
    if (entry.type != XmlEntry::EMPTY_TAG)
    {
        wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");
    }

    return new WxfDeleteRequest(messageId, epr);
}

WsenEnumerateRequest* WsmRequestDecoder::_decodeWSEnumerationEnumerate(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr,
    Boolean requestItemCount)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());
    _checkNoSelectorsEPR(epr);

    String expiration;
    WsmbPolymorphismMode polymorphismMode = WSMB_PM_UNKNOWN;
    WsenEnumerationMode enumerationMode = WSEN_EM_UNKNOWN;
    Boolean optimized = false;
    Uint32 maxElements = 0;
    String queryLanguage;
    String query;
    SharedPtr<WQLSelectStatement> selectStatement;

    XmlEntry entry;
    wsmReader.expectStartOrEmptyTag(
        entry, WsmNamespaces::SOAP_ENVELOPE, "Body");
    if (entry.type != XmlEntry::EMPTY_TAG)
    {
        wsmReader.decodeEnumerateBody(expiration, polymorphismMode,
            enumerationMode, optimized, maxElements, queryLanguage, query,
            selectStatement);
        wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");
    }

    // If PolymorphismMode header is not specified, set it to default
    if (polymorphismMode == WSMB_PM_UNKNOWN)
    {
        // DSP0227, R8.1-4: A service MAY optionally support the
        // wsmb:PolymorphismMode modifier element with a value of
        // IncludeSubClassProperties, which returns instances of the base
        // class and derived classes using the actual classs GED and XSD
        // type. This is the same as not specifying the polymorphism mode.
        polymorphismMode = WSMB_PM_INCLUDE_SUBCLASS_PROPERTIES;
    }
    else
    {
        // DSP0227, R8.1-6: The service SHOULD also return a
        // wsmb:PolymorphismModeNotSupported fault for requests using the
        // all classes ResourceURI if the PolymorphismMode is present and
        // does not equal IncludeSubClassProperties.

        CString tmp(epr.resourceUri.getCString());
        const char* suffix = WsmUtils::skipHostUri(tmp);

        if (strcmp(suffix, WSM_RESOURCEURI_ALLCLASSES_SUFFIX) == 0 &&
            polymorphismMode != WSMB_PM_INCLUDE_SUBCLASS_PROPERTIES)
        {
            throw WsmFault(
                WsmFault::wsmb_PolymorphismModeNotSupported,
                MessageLoaderParms(
                    "WsmServer.WsmReader.ENUMERATE_"
                        "POLYMORPHISM_INCLUDE_SUBCLASS",
                    "\"All classes\" resource URI requires "
                        "IncludeSubClassProperties polymorphism mode."));
        }
    }

    // If EnumerationMode header is not specified, set it to default
    if (enumerationMode == WSEN_EM_UNKNOWN)
    {
        enumerationMode = WSEN_EM_OBJECT;
    }

    // If optimized enumeration is requested but maxElements is not specified,
    // set it to default value of 1.
    if (optimized && maxElements == 0)
    {
        maxElements = 1;
    }

    return new WsenEnumerateRequest(
        messageId,
        epr,
        expiration,
        requestItemCount,
        optimized,
        maxElements,
        enumerationMode,
        polymorphismMode,
        queryLanguage,
        query,
        selectStatement);
}

WsenPullRequest* WsmRequestDecoder::_decodeWSEnumerationPull(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr,
    Boolean requestItemCount)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());
    _checkNoSelectorsEPR(epr);

    Uint64 enumerationContext = 0;
    String maxTime;
    Uint32 maxElements = 0;
    Uint32 maxCharacters = 0;

    XmlEntry entry;
    wsmReader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Body");
    wsmReader.decodePullBody(
        enumerationContext, maxTime, maxElements, maxCharacters);
    wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");

    // If maxElements is not specified, set it to default value of 1.
    if (maxElements == 0)
    {
        maxElements = 1;
    }

    return new WsenPullRequest(
        messageId,
        epr,
        enumerationContext,
        maxTime,
        requestItemCount,
        maxElements,
        maxCharacters);
}

WsenReleaseRequest* WsmRequestDecoder::_decodeWSEnumerationRelease(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr)
{
    _checkRequiredHeader("wsman:ResourceURI", epr.resourceUri.size());
    _checkNoSelectorsEPR(epr);

    Uint64 enumerationContext = 0;

    XmlEntry entry;
    wsmReader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Body");
    wsmReader.decodeReleaseBody(enumerationContext);
    wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");

    return new WsenReleaseRequest(
        messageId,
        epr,
        enumerationContext);
}

WsmRequest* WsmRequestDecoder::_decodeWSInvoke(
    WsmReader& wsmReader,
    const String& messageId,
    const WsmEndpointReference& epr,
    const String& className,
    const String& methodName)
{
    XmlEntry entry;

    //
    // Parse the <s:Body> element. Here is an example:
    //
    //   <s:Body>
    //     <p:Foo_INPUT xmlns:p=
    //       "http://schemas.dmtf.org/wbem/wscim/1/cim-schema/2/SomeClass">
    //       <p:Arg1>
    //         1234
    //       </p:Arg1>
    //       <p:Arg2>
    //         Hello!
    //       </p:Arg2>
    //     </p:Foo_INPUT>
    //   </s:Body>
    //

    WsmInstance instance;
    wsmReader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Body");
    wsmReader.decodeInvokeInputBody(className, methodName, instance);
    wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");

    return new WsInvokeRequest(messageId, epr, className, methodName, instance);
}

bool WsmRequestDecoder::_isIdentifyRequest(WsmReader& wsmReader)
{
    // Parse the <s:Body> element. Here is an example:
    //
    //   <s:Body>
    //     <wsmid:Identify>
    //   </s:Body>

    XmlEntry entry;
    wsmReader.setHideEmptyTags(true);
    wsmReader.expectStartTag(entry, WsmNamespaces::SOAP_ENVELOPE, "Body");

    try
    {
        // Expect an identify element. Ignore the namespace to be more
        // tolerant.
        int nsType = wsmReader.expectStartTag(entry, "Identify");
        wsmReader.expectEndTag(nsType, "Identify");
    }
    catch (...)
    {
        wsmReader.setHideEmptyTags(false);
        return false;
    }

    wsmReader.expectEndTag(WsmNamespaces::SOAP_ENVELOPE, "Body");
    wsmReader.setHideEmptyTags(false);

    return true;
}

void WsmRequestDecoder::_sendIdentifyResponse(Uint32 queueId)
{
    const char HTTP[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/soap+xml;charset=UTF-8\r\n"
        "Content-Length: ";

    const char XML[] =
        "<s:Envelope xmlns:s=\""
        "http://www.w3.org/2003/05/soap-envelope"
        "\" xmlns:wsmid=\""
        "http://schemas.dmtf.org/wbem/wsman/identify/1/wsmanidentity.xsd"
        "\">"
        "<s:Header>"
        "</s:Header>"
        "<s:Body>"
        "<wsmid:IdentifyResponse>"
        "<wsmid:ProtocolVersion>"
        WSMAN_PROTOCOL_VERSION
        "</wsmid:ProtocolVersion>"
        "<wsmid:ProductVendor>"
        WSMAN_PRODUCT_VENDOR
        "</wsmid:ProductVendor>"
        "<wsmid:ProductVersion>"
        WSMAN_PRODUCT_VERSION
        "</wsmid:ProductVersion>"
        "</wsmid:IdentifyResponse>"
        "</s:Body>"
        "</s:Envelope>";

    Buffer message;
    message.append(HTTP, sizeof(HTTP) - 1);

    char buf[32];
    int n = sprintf(buf, "%d\r\n\r\n", int(sizeof(XML) - 1));
    message.append(buf, n);

    message.append(XML, sizeof(XML) - 1);

    sendResponse(queueId, message, false);
}

PEGASUS_NAMESPACE_END
