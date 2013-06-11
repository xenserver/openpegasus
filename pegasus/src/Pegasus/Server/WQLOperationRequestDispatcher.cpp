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

#include "WQLOperationRequestDispatcher.h"
#include <Pegasus/Common/AutoPtr.h>
#include <Pegasus/Common/StatisticalData.h>

PEGASUS_NAMESPACE_BEGIN

PEGASUS_USING_STD;

void WQLOperationRequestDispatcher::applyQueryToEnumeration(
    CIMResponseMessage* msg,
    QueryExpressionRep* query)
{
    CIMEnumerateInstancesResponseMessage* enr =
        (CIMEnumerateInstancesResponseMessage*) msg;
    WQLSelectStatement* qs = ((WQLQueryExpressionRep*)query)->_stmt;

    Array<CIMInstance>& a = enr->getResponseData().getInstances();

    for (int i = a.size() - 1; i >= 0; i--)
    {
        WQLInstancePropertySource ips(a[i]);
        try
        {
            if (qs->evaluateWhereClause(&ips))
            {
                //
                // Specify that missing requested project properties are
                // allowed to be consistent with clarification from DMTF
                //
                qs->applyProjection(a[i], true);
            }
            else a.remove(i);
        }
        catch (...)
        {
            a.remove(i);
        }
    }
}

void WQLOperationRequestDispatcher::handleQueryResponseAggregation(
    OperationAggregate* poA)
{
    PEG_METHOD_ENTER(TRC_DISPATCHER,
        "WQLOperationRequestDispatcher::handleQueryResponseAggregation");

    Uint32 numberResponses = poA->numberResponses();

    PEG_TRACE((
        TRC_DISPATCHER,
        Tracer::LEVEL4,
        "WQLOperationRequestDispatcher::ExecQuery Response - "
            "Name Space: %s  Class name: %s Response Count: %u",
        (const char*) poA->_nameSpace.getString().getCString(),
        (const char*) poA->_className.getString().getCString(),
        numberResponses));

    if (numberResponses == 0)
        return;

    CIMResponseMessage* response = poA->getResponse(0);
    CIMExecQueryResponseMessage* toResponse = 0;
    Uint32 startIndex = 0;
    Uint32 endIndex = numberResponses - 1;
    Boolean manyResponses = true;
    if (response->getType() == CIM_ENUMERATE_INSTANCES_RESPONSE_MESSAGE)
    {
        // Create an ExecQuery response from an EnumerateInstances request
        CIMRequestMessage* request = poA->getRequest();
        AutoPtr<CIMExecQueryResponseMessage> query(
            new CIMExecQueryResponseMessage(
                request->messageId,
                CIMException(),
                request->queueIds.copyAndPop()));
        query->syncAttributes(request);
        toResponse = query.release();
    }
    else
    {
        toResponse = (CIMExecQueryResponseMessage*) response;
        manyResponses = false;
    }

    // Work backward and delete each response off the end of the array
    for (Uint32 i = endIndex; i >= startIndex; i--)
    {
        if (manyResponses)
            response = poA->getResponse(i);

        if (response->getType() == CIM_ENUMERATE_INSTANCES_RESPONSE_MESSAGE)
        {
            // convert enumerate instances responses to exec query responses
            applyQueryToEnumeration(response, poA->_query);
            CIMEnumerateInstancesResponseMessage* fromResponse =
                (CIMEnumerateInstancesResponseMessage*) response;
            CIMClass cimClass;

            Boolean clsRead=false;
            Array<CIMInstance>& a =
                fromResponse->getResponseData().getInstances();
            for (Uint32 j = 0, m = a.size();
                 j < m; j++)
            {
                CIMObject co=CIMObject(a[j]);
                CIMObjectPath op=co.getPath();
                const Array<CIMKeyBinding>& kbs=op.getKeyBindings();
                if (kbs.size() == 0)
                {     // no path set why ?
                    if (clsRead == false)
                    {
                        cimClass = _repository->getClass(
                            poA->_nameSpace, op.getClassName(),
                            false,true,false, CIMPropertyList());
                        clsRead=true;
                    }
                    op = a[j].buildPath(cimClass);
                }
                op.setNameSpace(poA->_nameSpace);
                op.setHost(System::getHostName());
                co.setPath(op);
                if (manyResponses)
                    toResponse->getResponseData().appendObject(co);
            }
        }
        else
        {
            CIMExecQueryResponseMessage* fromResponse =
                (CIMExecQueryResponseMessage*) response;

            CIMResponseData & from = fromResponse->getResponseData();
            from.completeHostNameAndNamespace(
                System::getHostName(),
                poA->_nameSpace);

            if (manyResponses)
            {
                toResponse->getResponseData().appendResponseData(from);
            }            
        }
        if (manyResponses)
        {
            poA->deleteResponse(i);
        }

        if (i == 0)
            break;
    } // for all responses in response list

    // if we started with an enumerateInstances repsonse, then add it to overall
    if ((startIndex == 0) && manyResponses)
        poA->appendResponse(toResponse);

    PEG_METHOD_EXIT();
}

void WQLOperationRequestDispatcher::handleQueryRequest(
    CIMExecQueryRequestMessage* request)
{
    PEG_METHOD_ENTER(TRC_DISPATCHER,
        "WQLOperationRequestDispatcher::handleQueryRequest");

    Boolean exception=false;
    AutoPtr<WQLSelectStatement> selectStatement(new WQLSelectStatement());
    AutoPtr<WQLQueryExpressionRep> qx;
    CIMException cimException;
    CIMName className;

    if (request->queryLanguage!="WQL")
    {
        cimException = PEGASUS_CIM_EXCEPTION(
            CIM_ERR_QUERY_LANGUAGE_NOT_SUPPORTED, request->queryLanguage);
        exception=true;
    }
    else
    {
        try
        {
            WQLParser::parse(request->query, *selectStatement.get());
            className = selectStatement->getClassName();
            qx.reset(new WQLQueryExpressionRep("WQL", selectStatement.get()));
            selectStatement.release();
        }
        catch (ParseError&)
        {
            cimException =
                PEGASUS_CIM_EXCEPTION(CIM_ERR_INVALID_QUERY, request->query);
            exception=true;
        }
        catch (MissingNullTerminator&)
        {
            cimException =
                PEGASUS_CIM_EXCEPTION(CIM_ERR_INVALID_QUERY, request->query);
            exception = true;
        }

        if (exception == false)
        {
            if (!_checkExistenceOfClass(request->nameSpace, className))
            {
                cimException = PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_INVALID_CLASS, className.getString());
                exception = true;
            }
        }
    }

    if (exception)
    {
        CIMResponseMessage* response = request->buildResponse();
        response->cimException = cimException;

        _enqueueResponse(request, response);
        PEG_METHOD_EXIT();
        return;
    }

    //
    // Get names of descendent classes:
    //
    Array<ProviderInfo> providerInfos;

    // This gets set by _lookupAllInstanceProviders()
    Uint32 providerCount;

    try
    {
        providerInfos =
            _lookupAllInstanceProviders(
                request->nameSpace,
                className,
                providerCount);
    }
    catch (CIMException& e)
    {
        // Return exception response if exception from getSubClasses
        CIMResponseMessage* response = request->buildResponse();
        response->cimException = e;

        _enqueueResponse(request, response);
        PEG_METHOD_EXIT();
        return;
    }

    // Test for "enumerate too Broad" and if so, execute exception.
    // This limits the number of provider invocations, not the number
    // of instances returned.
    if (providerCount > _maximumEnumerateBreadth)
    {
        PEG_TRACE((TRC_DISPATCHER, Tracer::LEVEL1,
            "ERROR: Enumerate operation too broad for class %s.  "
                "Limit = %u, providerCount = %u",
            (const char*)request->className.getString().getCString(),
            _maximumEnumerateBreadth,
            providerCount));

        CIMResponseMessage* response = request->buildResponse();
        response->cimException =
            PEGASUS_CIM_EXCEPTION_L(CIM_ERR_NOT_SUPPORTED, MessageLoaderParms(
                "Server.WQLOperationRequestDispatcher.QUERY_REQ_TOO_BROAD",
                "The query request is too broad."));

        _enqueueResponse(request, response);
        PEG_METHOD_EXIT();
        return;
    }

    // If no provider is registered and the repository isn't the default,
    // return CIM_ERR_NOT_SUPPORTED
    if ((providerCount == 0) && !(_repository->isDefaultInstanceProvider()))
    {
        PEG_TRACE((TRC_DISPATCHER, Tracer::LEVEL2,
            "CIM_ERROR_NOT_SUPPORTED for %s",
            (const char*)request->className.getString().getCString()));

        CIMResponseMessage* response = request->buildResponse();
        response->cimException =
            PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);

        _enqueueResponse(request, response);
        PEG_METHOD_EXIT();
        return;
    }

    // We have instances for Providers and possibly repository.
    // Set up an aggregate object and save a copy of the original request.

    OperationAggregate* poA= new OperationAggregate(
        new CIMExecQueryRequestMessage(*request),
            request->getType(),
            request->messageId,
            request->queueIds.top(),
            className, CIMNamespaceName(),
            qx.release(),
            "WQL");

    // Set the number of expected responses in the OperationAggregate
    Uint32 numClasses = providerInfos.size();
    poA->_aggregationSN = cimOperationAggregationSN++;
    poA->_nameSpace=request->nameSpace;

    if (_repository->isDefaultInstanceProvider())
    {
        // Loop through providerInfos, forwarding requests to repository
        for (Uint32 i = 0; i < numClasses; i++)
        {
            ProviderInfo& providerInfo = providerInfos[i];

            // this class is registered to a provider - skip
            if (providerInfo.hasProvider)
                continue;

            // If this class does not have a provider

            PEG_TRACE((TRC_DISPATCHER, Tracer::LEVEL4,
                "Routing ExecQuery request for class %s to the "
                    "repository.  Class # %u of %u, aggregation SN %u.",
                (const char*)providerInfo.className.getString().getCString(),
                (unsigned int)(i + 1),
                (unsigned int)(numClasses),
                (unsigned int)(poA->_aggregationSN)));

            // Create an EnumerateInstances response from an ExecQuery request
            AutoPtr<CIMEnumerateInstancesResponseMessage> response(
                new CIMEnumerateInstancesResponseMessage(
                    request->messageId,
                    CIMException(),
                    request->queueIds.copyAndPop()));
            response->syncAttributes(request);

            try
            {
                // Enumerate instances only for this class
                response->getResponseData().setInstances(
                    _repository->enumerateInstancesForClass(
                        request->nameSpace,
                        providerInfo.className));
            }
            catch (CIMException& e)
            {
                response->cimException = e;
            }
            catch (Exception& e)
            {
                response->cimException = PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_FAILED, e.getMessage());
            }
            catch (...)
            {
                response->cimException = PEGASUS_CIM_EXCEPTION(
                    CIM_ERR_FAILED, String::EMPTY);
            }

            poA->appendResponse(response.release());
        } // for all classes and derived classes

        Uint32 numberResponses = poA->numberResponses();
        Uint32 totalIssued = providerCount + (numberResponses > 0 ? 1 : 0);
        poA->setTotalIssued(totalIssued);

        if (numberResponses > 0)
        {
            handleEnumerateInstancesResponseAggregation(poA);
            CIMResponseMessage* response = poA->removeResponse(0);
            _forwardRequestForAggregation(
                getQueueId(),
                String(),
                new CIMExecQueryRequestMessage(*request),
                poA, response);
        }
    } // if isDefaultInstanceProvider
    else
    {
        // Set the number of expected responses in the OperationAggregate
        poA->setTotalIssued(providerCount);
    }

    // Loop through providerInfos, forwarding requests to providers
    for (Uint32 i = 0; i < numClasses; i++)
    {
        // If this class has a provider
        ProviderInfo& providerInfo = providerInfos[i];

        // this class is NOT registered to a provider - skip
        if (!providerInfo.hasProvider)
            continue;

        PEG_TRACE((TRC_DISPATCHER, Tracer::LEVEL4,
            "Routing ExecQuery request for class %s to "
                "service \"%s\" for control provider \"%s\".  "
                "Class # %u of %u, aggregation SN %u.",
            (const char*)providerInfo.className.getString().getCString(),
            lookup(providerInfo.serviceId)->getQueueName(),
            (const char*)providerInfo.controlProviderName.getCString(),
            (unsigned int)(i + 1),
            (unsigned int)numClasses,
            (unsigned int)(poA->_aggregationSN)));

        ProviderIdContainer* providerIdContainer =
            providerInfo.providerIdContainer.get();

        if (providerInfo.hasNoQuery)
        {
            OperationContext* context = &request->operationContext;
            const OperationContext::Container* container = 0;
            container = &context->get(IdentityContainer::NAME);
            const IdentityContainer& identityContainer =
                dynamic_cast<const IdentityContainer&>(*container);

            AutoPtr<CIMEnumerateInstancesRequestMessage> enumReq(
                new CIMEnumerateInstancesRequestMessage(
                    request->messageId,
                    request->nameSpace,
                    providerInfo.className,
                    false,false,false,
                    CIMPropertyList(),
                    request->queueIds,
                    request->authType,
                    identityContainer.getUserName()));

            context = &enumReq->operationContext;
            if (providerIdContainer)
                context->insert(*providerIdContainer);
            context->insert(identityContainer);
            _forwardRequestForAggregation(
                 providerInfo.serviceId,
                 providerInfo.controlProviderName,
                 enumReq.release(), poA);
        }
        else
        {
            AutoPtr<CIMExecQueryRequestMessage> requestCopy(
                new CIMExecQueryRequestMessage(*request));

            OperationContext* context = &request->operationContext;
            if (providerIdContainer)
                context->insert(*providerIdContainer);

            requestCopy->operationContext = *context;
            requestCopy->className = providerInfo.className;

            _forwardRequestForAggregation(
                providerInfo.serviceId,
                providerInfo.controlProviderName,
                requestCopy.release(), poA);
        }
    } // for all classes and derived classes

    PEG_METHOD_EXIT();
}

PEGASUS_NAMESPACE_END
