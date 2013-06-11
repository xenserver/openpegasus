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

#ifndef Pegasus_Client_h
#define Pegasus_Client_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/SSLContext.h>
#include <Pegasus/Common/CIMObject.h>
#include <Pegasus/Common/CIMClass.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/CIMObjectPath.h>
#include <Pegasus/Common/CIMValue.h>
#include <Pegasus/Common/CIMParamValue.h>
#include <Pegasus/Common/CIMPropertyList.h>
#include <Pegasus/Common/CIMQualifierDecl.h>
#include <Pegasus/Common/Exception.h>
#include <Pegasus/Client/CIMClientException.h>
#include <Pegasus/Client/Linkage.h>
#include <Pegasus/Common/AcceptLanguageList.h>
#include <Pegasus/Common/ContentLanguageList.h>
#include <Pegasus/Client/ClientOpPerformanceDataHandler.h>


PEGASUS_NAMESPACE_BEGIN

class CIMClientInterface;

/**
    The CIMClient class provides an interface for a client application
    to communicate with a CIM Server.  The client application specifies
    the target CIM Server by providing connection parameters and
    authentication credentials (as necessary).

    The CIMClient class models the functionality defined in the DMTF's
    Specification for CIM Operations over HTTP (DSP0200) version 1.2, using
    similar operations and parameters.
*/
class PEGASUS_CLIENT_LINKAGE CIMClient
{
public:

    /**
        Constructs a CIMClient object.
    */
    CIMClient();

    /**
        Destructs a CIMClient object.
    */
    ~CIMClient();

    /**
        Gets the currently configured timeout value for the CIMClient object.
        @return An integer indicating the currently configured timeout
        value (in milliseconds).
    */
    Uint32 getTimeout() const;

    /**
        Sets the timeout value for CIMClient operations.  If an operation
        response is not received within this timeout value, a
        ConnectionTimeoutException is thrown and the connection is reset.
        The default timeout value is 20 seconds (20000 milliseconds).
        @param timeoutMilliseconds An integer specifying the timeout value
        (in milliseconds).
    */
    void setTimeout(Uint32 timeoutMilliseconds);

    /** Connects to the CIM Server at the specified host name and port number.
        Example usage:
        <PRE>
            CIMClient client;
            String host("localhost");
            try
            {
                client.connect(host, 5988, "guest", "guest");
            }
            catch (Exception& e)
            {
                cerr << "Pegasus Exception: " << e.getMessage() <<
                    ". Trying to connect to " << host << endl;
            }
        </PRE>
        @param host A String host name to connect to.  If host is an empty
        string and portNumber is 0, the client will attempt to connect
        to the server's unix domain socket (on supporting platforms).
        @param portNumber A Uint32 port number to connect to.
        @param userName A String specifying the user name for the connection.
        @param password A String containing the password of the specified user.
        @exception AlreadyConnectedException
            If the CIMClient is already connected to a CIM Server.
        @exception InvalidLocatorException
            If the specified address is improperly formed.
        @exception CannotCreateSocketException
            If a socket cannot be created.
        @exception CannotConnectException
            If the socket connection fails.
        @exception CIMClientConnectionException
            If any other failure occurs.
    */
    void connect(
        const String& host,
        const Uint32 portNumber,
        const String& userName,
        const String& password);

    /** Connects to the CIM Server at the specified host name, port number
        and SSL context.
        @param host A String host name to connect to.
        @param portNumber A Uint32 port number to connect to.
        @param sslContext The SSL context to use for this connection.
        @param userName A String specifying the user name for the connection.
        @param password A String containing the password of the specified user.
        @exception AlreadyConnectedException
            If the CIMClient is already connected to a CIM Server.
        @exception InvalidLocatorException
            If the specified address is improperly formed.
        @exception CannotCreateSocketException
            If a socket cannot be created.
        @exception CannotConnectException
            If the socket connection fails.
        @exception CIMClientConnectionException
            If any other failure occurs.
    */
    void connect(
        const String& host,
        const Uint32 portNumber,
        const SSLContext& sslContext,
        const String& userName,
        const String& password);

    /** Connects to the local CIM Server as the current user.  Authentication
        is performed automatically, so no credentials are required for this
        connection.
        @exception AlreadyConnectedException
            If the CIMClient is already connected to a CIM Server.
        @exception CannotCreateSocketException
            If a socket cannot be created.
        @exception CannotConnectException
            If the socket connection fails.
        @exception CIMClientConnectionException
            If any other failure occurs.
    */
    void connectLocal();

    /**
        Disconnects from the CIM Server.  If no connection is established,
        this method has no effect.  A CIMClient with an existing connection
        must be disconnected before establishing a new connection.
    */
    void disconnect();

    /**
        Configures the accept languages to be specified on subsequent
        requests from this client.  Accept languages are the preferred
        languages for response data.
        @param langs An AcceptLanguageList object specifying the languages
        preferred by this client.
    */
    void setRequestAcceptLanguages(const AcceptLanguageList& langs);

    /**
        Gets the accept languages currently configured for this client.
        Accept languages are the preferred languages for response data.
        @return An AcceptLanguageList object specifying the preferred languages
        configured for this client.
    */
    AcceptLanguageList getRequestAcceptLanguages() const;

    /**
        Configures the content languages to be specified on subsequent
        requests from this client.  The content languages indicate the
        languages associated with request data sent from this client.
        @param langs A ContentLanguageList object specifying the languages
        associated with this client's request data.
    */
    void setRequestContentLanguages(const ContentLanguageList& langs);

    /**
        Gets the content languages currently configured for this client.
        The content languages indicate the languages associated with request
        data sent from this client.
        @return A ContentLanguageList object specifying the languages used in
        request data from this client.
    */
    ContentLanguageList getRequestContentLanguages() const;

    /**
        Gets the content languages of the last response received by this
        client.  The content languages indicate the languages associated
        with the data included in the response.
        @return A ContentLanguageList object specifying the languages used in
        the last response received by this client.
    */
    ContentLanguageList getResponseContentLanguages() const;

#ifdef PEGASUS_USE_EXPERIMENTAL_INTERFACES
    /** <I><B>Experimental Interface</B></I><BR>
    */
    void setRequestDefaultLanguages();
#endif // PEGASUS_USE_EXPERIMENTAL_INTERFACES

    /**
        Gets a specified CIM Class from a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param className A CIMName that specifies the CIM Class to be retrieved.
        @param localOnly A Boolean indicating whether only the elements
            (properties, methods, and qualifiers) defined or overridden within
            the definition of the Class are to be returned.  If false, all
            elements are requested.  If not specified, this parameter defaults
            to true.
        @param includeQualifiers A Boolean indicating whether the Qualifiers
            for the Class (including Qualifiers on the Class and on any
            returned Properties, Methods, or Method Parameters) are to be
            included in the response.  If false, no Qualifiers are requested.
            If not specified, this parameter defaults to true.
        @param includeClassOrigin A Boolean indicating whether the Class Origin
            attribute is to be included in elements of the returned Class.
            If false, no Class Origin attributes are requested.
            If not specified, this parameter defaults to false.
        @param propertyList A CIMPropertyList which optionally limits the
            Property elements included in the returned Class.
            If not NULL, the returned Class is not to include Properties that
            are omitted from the list.  If NULL, no specific filtering of
            Properties is requested.  (An empty list, distinct from a NULL
            list, indicates that no Properties are to be included in the
            returned Class.)  Note that this parameter acts as an additional
            filter in conjunction with the localOnly parameter.
            If not specified, this parameter defaults to NULL.

        @return A CIMClass containing the requested Class definition.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    CIMClass getClass(
        const CIMNamespaceName& nameSpace,
        const CIMName& className,
        Boolean localOnly = true,
        Boolean includeQualifiers = true,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList());

    /**
        Gets a specified CIM Instance from a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param instanceName A CIMObjectPath that identifies the CIM Instance to
            be retrieved.
        @param localOnly (DEPRECATED) A Boolean indicating whether only the
            properties and qualifiers defined or overridden within the Instance
            are to be returned.  If false, all elements are requested.
            If not specified, this parameter defaults to true.
        @param includeQualifiers (DEPRECATED) A Boolean indicating whether the
            Qualifiers for the Instance (including Qualifiers on the Instance
            and on any returned Properties) are to be included in the response.
            If false, no Qualifiers are requested.
            If not specified, this parameter defaults to false.
        @param includeClassOrigin A Boolean indicating whether the Class Origin
            attribute is to be included in elements of the returned Instance.
            If false, no Class Origin attributes are requested.
            If not specified, this parameter defaults to false.
        @param propertyList A CIMPropertyList which optionally limits the
            Property elements included in the returned Instance.  If not NULL,
            the returned Instance is not to include Properties that are omitted
            from the list.  If NULL, no specific filtering of Properties is
            requested.  (An empty list, distinct from a NULL list, indicates
            that no Properties are to be included in the returned Instance.)
            Note that this parameter acts as an additional filter in
            conjunction with the localOnly parameter.
            If not specified, this parameter defaults to NULL.

        @return A CIMInstance containing the requested Instance.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    CIMInstance getInstance(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& instanceName,
        Boolean localOnly = true,
        Boolean includeQualifiers = false,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList());

    /**
        Deletes a specified CIM Class from a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param className A CIMName that specifies the CIM Class to be deleted.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void deleteClass(
        const CIMNamespaceName& nameSpace,
        const CIMName& className);

    /**
        Deletes a specified CIM Instance from a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param instanceName A CIMObjectPath that identifies the CIM Instance to
            be deleted.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void deleteInstance(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& instanceName);

    /**
        Creates a specified CIM Class in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param newClass A CIMClass containing the new Class definition to be
            created.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void createClass(
        const CIMNamespaceName& nameSpace,
        const CIMClass& newClass);

    /**
        Creates a specified CIM Instance in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param newInstance A CIMInstance containing the new Instance to be
            created.

        @return A CIMObjectPath containing the name of the newly created
            Instance.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    CIMObjectPath createInstance(
        const CIMNamespaceName& nameSpace,
        const CIMInstance& newInstance);

    /**
        Modifies a specified CIM Class in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param modifiedClass A CIMClass containing the updates to be made to
            the Class definition.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void modifyClass(
        const CIMNamespaceName& nameSpace,
        const CIMClass& modifiedClass);

    /**
        Modifies a specified CIM Instance in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param modifiedInstance A CIMInstance containing the name of the
            Instance to be modified (specified by the Path attribute) and the
            updates to be made to the Instance.
        @param includeQualifiers (DEPRECATED) A Boolean indicating whether the
            Qualifiers for the Instance (including Qualifiers on the Instance
            and its Properties) are to be updated.
            If not specified, this parameter defaults to true.
        @param propertyList A CIMPropertyList which optionally limits the
            Property elements that are updated.  If not NULL, Properties that
            are omitted from the list are not updated.  If NULL, all Properties
            are updated.  (An empty list, distinct from a NULL list, indicates
            that no Properties are to be updated.)
            If not specified, this parameter defaults to NULL.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void modifyInstance(
        const CIMNamespaceName& nameSpace,
        const CIMInstance& modifiedInstance,
        Boolean includeQualifiers = true,
        const CIMPropertyList& propertyList = CIMPropertyList());

    /**
        Enumerates CIM Classes derived from a specified Class in a target
        namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param className A CIMName that specifies the CIM Class for which to
            enumerate derived Classes.  If NULL, the Classes which have no
            superclass are enumerated.
        @param deepInheritance A Boolean indicating whether the enumeration
            should include all levels of derivation.  If false, only the
            immediate subclasses are enumerated.  If not specified, this
            parameter defaults to false.
        @param localOnly A Boolean indicating whether only the elements
            (properties, methods, and qualifiers) defined or overridden within
            the definition of each Class are to be returned.  If false, all
            elements are requested.  If not specified, this parameter defaults
            to true.
        @param includeQualifiers A Boolean indicating whether the Qualifiers
            for each Class (including Qualifiers on the Class and on any
            returned Properties, Methods, or Method Parameters) are to be
            included in the response.  If false, no Qualifiers are requested.
            If not specified, this parameter defaults to true.
        @param includeClassOrigin A Boolean indicating whether the Class Origin
            attribute is to be included in elements of each returned Class.
            If false, no Class Origin attributes are requested.
            If not specified, this parameter defaults to false.

        @return An Array of zero or more CIMClass objects containing the
            requested Class definitions.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMClass> enumerateClasses(
        const CIMNamespaceName& nameSpace,
        const CIMName& className = CIMName(),
        Boolean deepInheritance = false,
        Boolean localOnly = true,
        Boolean includeQualifiers = true,
        Boolean includeClassOrigin = false);

    /**
        Enumerates the names of CIM Classes derived from a specified Class in a
        target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param className A CIMName that specifies the CIM Class for which to
            enumerate the names of derived Classes.  If NULL, the names of
            Classes which have no superclass are enumerated.
        @param deepInheritance A Boolean indicating whether the enumeration
            should include all levels of derivation.  If false, only the names
            of immediate subclasses are enumerated.  If not specified, this
            parameter defaults to false.

        @return An Array of zero or more CIMName objects containing the
            requested Class names.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMName> enumerateClassNames(
        const CIMNamespaceName& nameSpace,
        const CIMName& className = CIMName(),
        Boolean deepInheritance = false);

    /**
        Enumerates CIM Instances of a specified Class and its subclasses in a
        target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param className A CIMName that specifies the CIM Class for which to
            enumerate Instances.
        @param localOnly (DEPRECATED) A Boolean indicating whether only the
            properties and qualifiers defined or overridden within each
            Instance are to be returned.  If false, all elements are requested.
            If not specified, this parameter defaults to true.
        @param deepInheritance A Boolean indicating whether the returned
            Instances should include properties defined in subclasses of the
            specified class.  If false, only properties defined in the
            specified class are requested.  If not specified, this parameter
            defaults to true.
        @param includeQualifiers (DEPRECATED) A Boolean indicating whether the
            Qualifiers for each Instance (including Qualifiers on the Instance
            and on any returned Properties) are to be included in the response.
            If false, no Qualifiers are requested.
            If not specified, this parameter defaults to false.
        @param includeClassOrigin A Boolean indicating whether the Class Origin
            attribute is to be included in elements of each returned Instance.
            If false, no Class Origin attributes are requested.
            If not specified, this parameter defaults to false.
        @param propertyList A CIMPropertyList which optionally limits the
            Property elements included in the returned Instances.  If not NULL,
            the returned Instances are not to include Properties that are
            omitted from the list.  If NULL, no specific filtering of
            Properties is requested.  (An empty list, distinct from a NULL
            list, indicates that no Properties are to be included in the
            returned Instances.) Note that this parameter acts as an additional
            filter in conjunction with the deepInheritance and localOnly
            parameters.  If not specified, this parameter defaults to NULL.

        @return An Array of zero or more CIMInstance objects containing the
            requested Instances.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMInstance> enumerateInstances(
        const CIMNamespaceName& nameSpace,
        const CIMName& className,
        Boolean deepInheritance = true,
        Boolean localOnly = true,
        Boolean includeQualifiers = false,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList());

    /**
        Enumerates the names of CIM Instances of a specified Class and its
        subclasses in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param className A CIMName that specifies the CIM Class for which to
            enumerate Instance names.

        @return An Array of zero or more CIMObjectPaths containing the
            requested Instance names.  Host and namespace attributes are not
            included in these CIMObjectPath values, per the WBEM protocol.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMObjectPath> enumerateInstanceNames(
        const CIMNamespaceName& nameSpace,
        const CIMName& className);

    /**
        Executes a query against a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param queryLanguage Defines the query language in which the query
            parameter is expressed.
        @param query Specifies the query to be executed.

        @return An Array of zero or more CIM Classes or Instances that comprise
            the query result.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMObject> execQuery(
        const CIMNamespaceName& nameSpace,
        const String& queryLanguage,
        const String& query);

    /**
        Enumerates CIM Objects (Classes or Instances) which are associated with
        a specified Object in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param objectName A CIMObjectPath that specifies the CIM Object for
            which to enumerate associated Objects.  It may contain either a
            Class name or Instance name (model path).
        @param assocClass A CIMName specifying an association class constraint.
            If not NULL, the returned Objects are to be limited to those
            associated with the specified Object via an Instance of this Class
            or one of its subclasses.
        @param resultClass A CIMName specifying a result filter.  If not NULL,
            each returned Object is to be either an Instance of this Class (or
            one of its subclasses) or be this Class (or one of its subclasses).
        @param role A String specifying a role filter.  If not the empty
            String, each returned Object is to be associated to the specified
            Object via an Association in which the specified Object plays the
            specified role (I.e., the role value matches the name of the
            Property in the Association Class that refers to the specified
            object).
        @param resultRole A String specifying a result role filter.  If not the
            empty String, each returned Object is to be associated to the
            specified Object via an Association in which the returned Object
            plays the specified role (I.e., the role value matches the name of
            the Property in the Association Class that refers to the returned
            Object).
        @param includeQualifiers (DEPRECATED) A Boolean indicating whether the
            Qualifiers for each Object are to be included in the response.
            If false, no Qualifiers are requested.
            If not specified, this parameter defaults to false.
        @param includeClassOrigin A Boolean indicating whether the Class Origin
            attribute is to be included in elements of each returned Object.
            If false, no Class Origin attributes are requested.
            If not specified, this parameter defaults to false.
        @param propertyList A CIMPropertyList which optionally limits the
            Property elements included in the returned Objects.  If not NULL,
            the returned Objects are not to include Properties that are
            omitted from the list.  If NULL, no specific filtering of
            Properties is requested.  (An empty list, distinct from a NULL
            list, indicates that no Properties are to be included in the
            returned Objects.)  If not specified, this parameter defaults to
            NULL.

        @return An Array of zero or more CIMObjects containing the associated
            Classes or Instances.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMObject> associators(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& assocClass = CIMName(),
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY,
        const String& resultRole = String::EMPTY,
        Boolean includeQualifiers = false,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList());

    /**
        Enumerates the names of CIM Objects (Classes or Instances) which are
        associated with a specified Object in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param objectName A CIMObjectPath that specifies the CIM Object for
            which to enumerate the names of associated Objects.  It may contain
            either a Class name or Instance name (model path).
        @param assocClass A CIMName specifying an association class constraint.
            If not NULL, the returned Object names are to be limited to those
            associated with the specified Object via an Instance of this Class
            or one of its subclasses.
        @param resultClass A CIMName specifying a result filter.  If not NULL,
            each returned name is to identify an Object which either is an
            Instance of this Class (or one of its subclasses) or is this Class
            (or one of its subclasses).
        @param role A String specifying a role filter.  If not the empty
            String, each returned Object name is to be associated to the
            specified Object via an Association in which the specified Object
            plays the specified role (I.e., the role value matches the name of
            the Property in the Association Class that refers to the specified
            object).
        @param resultRole A String specifying a result role filter.  If not the
            empty String, each returned Object name is to be associated to the
            specified Object via an Association in which the returned Object
            name plays the specified role (I.e., the role value matches the
            name of the Property in the Association Class that refers to the
            returned Object name).

        @return An Array of zero or more CIMObjectPaths containing the names of
            associated Classes or Instances.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMObjectPath> associatorNames(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& assocClass = CIMName(),
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY,
        const String& resultRole = String::EMPTY);

    /**
        Enumerates CIM Association Objects (Classes or Instances) which refer
        to a specified Object in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param objectName A CIMObjectPath that specifies the CIM Object for
            which to enumerate referring Objects.  It may contain either a
            Class name or Instance name (model path).
        @param resultClass A CIMName specifying a result filter.  If not NULL,
            each returned Object is to be either an Instance of this Class (or
            one of its subclasses) or be this Class (or one of its subclasses).
        @param role A String specifying a result role filter.  If not the
            empty String, each returned Object is to refer to the specified
            Object via a CIMProperty whose name matches the value of this
            parameter.
        @param includeQualifiers (DEPRECATED) A Boolean indicating whether the
            Qualifiers for each Object are to be included in the response.
            If false, no Qualifiers are requested.
            If not specified, this parameter defaults to false.
        @param includeClassOrigin A Boolean indicating whether the Class Origin
            attribute is to be included in elements of each returned Object.
            If false, no Class Origin attributes are requested.
            If not specified, this parameter defaults to false.
        @param propertyList A CIMPropertyList which optionally limits the
            Property elements included in the returned Objects.  If not NULL,
            the returned Objects are not to include Properties that are
            omitted from the list.  If NULL, no specific filtering of
            Properties is requested.  (An empty list, distinct from a NULL
            list, indicates that no Properties are to be included in the
            returned Objects.)  If not specified, this parameter defaults to
            NULL.

        @return An Array of zero or more CIMObjects containing the Classes or
            Instances that refer to the specified Object.  Each returned Object
            contains a full Path attribute.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMObject> references(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY,
        Boolean includeQualifiers = false,
        Boolean includeClassOrigin = false,
        const CIMPropertyList& propertyList = CIMPropertyList());

    /**
        Enumerates the names of CIM Objects (Classes or Instances) which refer
        to a specified Object in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param objectName A CIMObjectPath that specifies the CIM Object for
            which to enumerate the names of referring Objects.  It may contain
            either a Class name or Instance name (model path).
        @param resultClass A CIMName specifying a result filter.  If not NULL,
            each returned name is to identify an Object which either is an
            Instance of this Class (or one of its subclasses) or is this Class
            (or one of its subclasses).
        @param role A String specifying a result role filter.  If not the
            empty String, each Object for which the name is returned is to
            refer to the specified Object via a CIMProperty whose name matches
            the value of this parameter.

        @return An Array of zero or more CIMObjectPaths containing the names of
            referring Classes or Instances.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMObjectPath> referenceNames(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& objectName,
        const CIMName& resultClass = CIMName(),
        const String& role = String::EMPTY);

    /**
        Gets a single Property value from a CIM Instance in a target Namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param instanceName A CIMObjectPath that identifies the CIM Instance
            from which to retrieve the Property value.
        @param propertyName A String containing the name of the Property for
            which to retrieve the value.

        @return A CIMValue containing the requested Property value.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    CIMValue getProperty(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& instanceName,
        const CIMName& propertyName);

    /**
        Sets a single Property value in a CIM Instance in a target Namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param instanceName A CIMObjectPath that identifies the CIM Instance
            in which to set the Property value.
        @param propertyName A String containing the name of the Property for
            which to set the value.
        @param newValue A CIMValue containing the new Property value.  If not
            specified, the Property is set to NULL.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void setProperty(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& instanceName,
        const CIMName& propertyName,
        const CIMValue& newValue = CIMValue());

    /**
        Gets a Qualifier declaration from a target Namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param qualifierName A CIMName that identifies the Qualifier for which
            to retrieve the declaration.

        @return A CIMQualifierDecl containing the requested Qualifier
            declaration.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    CIMQualifierDecl getQualifier(
        const CIMNamespaceName& nameSpace,
        const CIMName& qualifierName);

    /**
        Adds or updates a Qualifier declaration in a target Namespace.  If the
        Qualifier declaration already exists, it is overwritten.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param qualifierDeclaration A CIMQualifierDecl object containing the
            Qualifier declaration to be added or updated in the namespace.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void setQualifier(
        const CIMNamespaceName& nameSpace,
        const CIMQualifierDecl& qualifierDeclaration);

    /**
        Deletes a Qualifier declaration in a target Namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param qualifierName A CIMName containing the name of the Qualifier
            to be deleted.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    void deleteQualifier(
        const CIMNamespaceName& nameSpace,
        const CIMName& qualifierName);

    /**
        Enumerates Qualifier declarations in a target Namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.

        @return An Array of zero or more CIMQualifierDecl objects.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    Array<CIMQualifierDecl> enumerateQualifiers(
        const CIMNamespaceName& nameSpace);

    /**
        Executes an extrinsic CIM method in a target namespace.

        @param nameSpace A CIMNamespaceName that specifies the target namespace.
        @param instanceName A CIMObjectPath that defines the CIM Class or
            Instance on which to execute the method.
        @param methodName A CIMName the specifies the name of the method to
            execute.
        @param inParameters An Array of CIMParamValue objects specifying the
            input parameters for the method.
        @param outParameters An output Array of CIMParamValue objects containing
            the method output parameters.

        @return A CIMValue containing the method return value.

        @exception CIMException If the CIM Server fails to perform the
            requested operation.  See DSP0200 for specific CIM Status Codes
            that may be expected.
        @exception Exception If an error occurs while sending the request or
            receiving the response.
    */
    CIMValue invokeMethod(
        const CIMNamespaceName& nameSpace,
        const CIMObjectPath& instanceName,
        const CIMName& methodName,
        const Array<CIMParamValue>& inParameters,
        Array<CIMParamValue>& outParameters);

    /**
        Registers a ClientOpPerformanceDataHandler object.  The specified
        object is called with performance data relative to each operation
        performed by this CIMClient object.  Only one
        ClientOpPerformanceDataHandler can be registered at a time.
        A subsequent registration replaces the previous one.
        @param handler The ClientOpPerformanceDataHandler object to register.
    */
    void registerClientOpPerformanceDataHandler(
        ClientOpPerformanceDataHandler & handler);

    /**
        Unregisters the current ClientOpPerformanceDataHandler, if applicable.
    */
    void deregisterClientOpPerformanceDataHandler();

private:

    /**
        The copy constructor is not available for the CIMClient class.
    */
    CIMClient(const CIMClient&);

    /**
        The assignment operator is not available for the CIMClient class.
    */
    CIMClient& operator=(const CIMClient&);

    CIMClientInterface* _rep;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_Client_h */
