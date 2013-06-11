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

/* This program tests the generation and resolution of instances.  It creates
   a set of qualifiers and a class and then creates instances of the class
   and confirms both the creation characteristics and the resolution
   characteristics.
*/

#include <Pegasus/Common/PegasusAssert.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/CIMClass.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/DeclContext.h>
#include <Pegasus/Common/Resolver.h>
#include <Pegasus/Common/XmlWriter.h>

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

static Boolean verbose;

void test01()
{
    const CIMNamespaceName NAMESPACE = CIMNamespaceName("/zzz");

    // Create and populate a declaration context:

    SimpleDeclContext* context = new SimpleDeclContext;

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("counter"), false,
        CIMScope::PROPERTY));

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("classcounter"), false,
        CIMScope::CLASS));

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("min"), String(),
        CIMScope::PROPERTY));

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("max"), String(),
        CIMScope::PROPERTY));

    context->addQualifierDecl(NAMESPACE,
        CIMQualifierDecl(CIMName("Description"), String(),
        CIMScope::PROPERTY));

    CIMClass class1(CIMName("MyClass"));

    class1
        .addProperty(CIMProperty(CIMName("count"), Uint32(55))
            .addQualifier(CIMQualifier(CIMName("counter"), true))
            .addQualifier(CIMQualifier(CIMName("min"), String("0")))
            .addQualifier(CIMQualifier(CIMName("max"), String("1"))))
        .addProperty(CIMProperty(CIMName("message"), String("Hello"))
            .addQualifier(CIMQualifier(CIMName("description"),
                String("My Message"))))
        .addProperty(CIMProperty(CIMName("ratio"), Real32(1.5)));

    // Test
    PEGASUS_TEST_ASSERT(
        class1.findProperty(CIMName("count")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        class1.findProperty(CIMName("message")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        class1.findProperty(CIMName("ratio")) != PEG_NOT_FOUND);

    Resolver::resolveClass(class1, context, NAMESPACE);
    context->addClass(NAMESPACE, class1);

    if (verbose)
    {
        XmlWriter::printClassElement(class1);
    }

    CIMInstance instance0(CIMName("MyClass"));
    PEGASUS_TEST_ASSERT(instance0.getClassName().equal(CIMName("MyClass")));
    instance0.setPath(CIMObjectPath("//localhost/root/cimv2:MyClass.Foo=1"));
    PEGASUS_TEST_ASSERT(instance0.getPath() ==
        CIMObjectPath("//localhost/root/cimv2:MyClass.Foo=1"));

    CIMInstance instance1(CIMName("MyClass"));
    instance1.addQualifier(CIMQualifier(CIMName("classcounter"), true));

    instance1.addProperty(CIMProperty(CIMName("message"), String("Goodbye")));

    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("message")) != PEG_NOT_FOUND);

    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("count")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("ratio")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("nuts")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(instance1.getPropertyCount() == 1);

    if (verbose)
    {
        XmlWriter::printInstanceElement(instance1);
    }

    Resolver::resolveInstance(instance1, context, NAMESPACE, true);

    if (verbose)
    {
        XmlWriter::printInstanceElement(instance1);
    }

    // Now test for properties after resolution.

    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("message")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("count")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("ratio")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("nuts")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(instance1.getPropertyCount() == 3);

    // Now remove a property

    Uint32 posProperty;
    posProperty = instance1.findProperty(CIMName("count"));
    instance1.removeProperty(posProperty);

    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("message")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("count")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("ratio")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance1.findProperty(CIMName("nuts")) == PEG_NOT_FOUND);

    PEGASUS_TEST_ASSERT(instance1.getPropertyCount() == 2);

    // Instance qualifier tests

    CIMQualifier cq=instance1.getQualifier(instance1.findQualifier(
        CIMName("classcounter")));

    const CIMInstance instance2 = instance1.clone();
    PEGASUS_TEST_ASSERT(instance2.identical(instance1));
    PEGASUS_TEST_ASSERT(
        instance1.findQualifier(CIMName("nuts")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        instance2.findQualifier(CIMName("nuts")) == PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(instance1.getQualifierCount() != 4);
    PEGASUS_TEST_ASSERT(instance1.getQualifierCount() == 1);
    PEGASUS_TEST_ASSERT(instance2.getQualifierCount() == 1);

    if (verbose)
    {
        XmlWriter::printInstanceElement(instance2);
    }

    // Tests for CIMConstInstance
    CIMConstInstance cinstance1(CIMName("MyClass")), cinstance3;
    CIMConstInstance ccopy(cinstance1);

    cinstance1 = instance1;
    PEGASUS_TEST_ASSERT(cinstance1.identical(instance1));

    PEGASUS_TEST_ASSERT(cinstance1.getQualifierCount() == 1);
    CIMConstQualifier ccq = cinstance1.getQualifier(cinstance1.findQualifier(
        CIMName("classcounter")));
    PEGASUS_TEST_ASSERT(
        cinstance1.findProperty(CIMName("message")) != PEG_NOT_FOUND);
    CIMConstProperty ccp =
        cinstance1.getProperty(cinstance1.findProperty(CIMName("message")));

    cinstance3 = cinstance1;
    PEGASUS_TEST_ASSERT(cinstance3.identical(cinstance1));

    PEGASUS_TEST_ASSERT(cinstance1.getClassName() == CIMName("MyClass"));
    PEGASUS_TEST_ASSERT(cinstance1.getClassName().equal(CIMName("MyClass")));
    PEGASUS_TEST_ASSERT(cinstance1.getClassName().equal(CIMName("MYCLASS")));
    PEGASUS_TEST_ASSERT(cinstance1.getClassName().equal(CIMName("myclass")));
    PEGASUS_TEST_ASSERT(!cinstance1.getClassName().equal(CIMName("blob")));

    PEGASUS_TEST_ASSERT(cinstance1.getQualifierCount() != 4);
    PEGASUS_TEST_ASSERT(cinstance1.getPropertyCount() == 2);

    CIMConstInstance cinstance2 = cinstance1.clone();
    PEGASUS_TEST_ASSERT(cinstance2.identical(cinstance1));

    if (verbose)
    {
        XmlWriter::printInstanceElement(cinstance1);
    }

    cinstance1.buildPath(class1);

    PEGASUS_TEST_ASSERT( !cinstance1.isUninitialized() );

    delete context;
}

void test02()
{
    const CIMNamespaceName NAMESPACE = CIMNamespaceName("/zzz");

    CIMClass cimClass(CIMName("MyClass"));

    cimClass
        .addProperty(CIMProperty(CIMName("Last"), String())
            .addQualifier(CIMQualifier(CIMName("key"), true)))
        .addProperty(CIMProperty(CIMName("First"), String())
            .addQualifier(CIMQualifier(CIMName("key"), true)))
        .addProperty(CIMProperty(CIMName("Age"), String())
            .addQualifier(CIMQualifier(CIMName("key"), true)));

    CIMInstance cimInstance(CIMName("MyClass"));
    cimInstance.addProperty(CIMProperty(CIMName("first"), String("John")));
    cimInstance.addProperty(CIMProperty(CIMName("last"), String("Smith")));
    cimInstance.addProperty(CIMProperty(CIMName("age"), Uint8(101)));

    PEGASUS_TEST_ASSERT(
        cimInstance.findProperty(CIMName("first")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        cimInstance.findProperty(CIMName("last")) != PEG_NOT_FOUND);
    PEGASUS_TEST_ASSERT(
        cimInstance.findProperty(CIMName("age")) != PEG_NOT_FOUND);

    PEGASUS_TEST_ASSERT(cimInstance.getPropertyCount() == 3);


    CIMObjectPath instanceName =
        cimInstance.buildPath(CIMConstClass(cimClass));

    CIMObjectPath tmp("myclass.age=101,first=\"John\",last=\"Smith\"");

    PEGASUS_TEST_ASSERT(tmp.makeHashCode() == instanceName.makeHashCode());

    // Test CIMInstance::buildPath with incomplete keys in the instance

    Boolean caughtNoSuchPropertyException = false;

    try
    {
        CIMInstance badInstance(CIMName("MyClass"));
        badInstance.addProperty(CIMProperty(CIMName("first"), String("John")));
        badInstance.addProperty(CIMProperty(CIMName("last"), String("Smith")));
        instanceName = badInstance.buildPath(CIMConstClass(cimClass));
    }
    catch (const NoSuchProperty&)
    {
        caughtNoSuchPropertyException = true;
    }

    PEGASUS_TEST_ASSERT(caughtNoSuchPropertyException);
}

// Build the instance of an association class and test the build path functions.
void test03()
{
    const CIMNamespaceName NAMESPACE = CIMNamespaceName("/zzz");

    CIMClass myPersonClass(CIMName("MY_PersonClass"));

    myPersonClass
        .addProperty(CIMProperty(CIMName("name"), String())
            .addQualifier(CIMQualifier(CIMName("Key"), true)));

    CIMClass myAssocClass(CIMName("MY_AssocClass"));

    myAssocClass
        .addQualifier(CIMQualifier(CIMName("association"), true))
        .addProperty(CIMProperty(CIMName("parent"), CIMObjectPath(), 0,
            CIMName("MY_PersonClass"))
            .addQualifier(CIMQualifier(CIMName("key"), true)))
        .addProperty(CIMProperty(CIMName("child"), CIMObjectPath(), 0,
            CIMName("MY_PersonClass"))
            .addQualifier(CIMQualifier(CIMName("key"), true)))
        .addProperty(CIMProperty(CIMName("Age"), String()));

    CIMInstance fatherInstance(CIMName("MY_PersonClass"));

    fatherInstance
        .addProperty(CIMProperty(CIMName("name"), String("father")));

    CIMInstance daughterInstance(CIMName("My_PersonClass"));
    daughterInstance
       .addProperty(CIMProperty(CIMName("name"), String("daughter")));

    CIMObjectPath fatherInstancePath =
        fatherInstance.buildPath(CIMConstClass(myPersonClass));

    CIMObjectPath daughterInstancePath =
        daughterInstance.buildPath(CIMConstClass(myPersonClass));

    CIMInstance assocInstance(CIMName("MY_AssocClass"));

    assocInstance.addProperty(CIMProperty(CIMName("parent"),
        CIMObjectPath(fatherInstancePath),0,CIMName("MY_PersonClass")));

    assocInstance.addProperty(CIMProperty(CIMName("child"),
        CIMObjectPath(daughterInstancePath),0, CIMName("MY_PersonClass")));

    CIMObjectPath assocInstancePath =
        assocInstance.buildPath(CIMConstClass(myAssocClass));

    // Now do the asserts, etc.
    // See if the pathing works on Associations and association instances

    if (verbose)
    {
        XmlWriter::printClassElement(myPersonClass);
        XmlWriter::printClassElement(myAssocClass);
        XmlWriter::printInstanceElement(fatherInstance);
        XmlWriter::printInstanceElement(daughterInstance);
        XmlWriter::printInstanceElement(assocInstance);
    }

    if (verbose)
    {
        cout << "Paths " << endl;
        cout << "FatherInstancePath = " << fatherInstancePath.toString() <<
            endl;
        cout << "DaughterInstancePath = " << daughterInstancePath.toString() <<
            endl;
        cout << "AssocInstancePath = " << assocInstancePath.toString() << endl;
    }

    String x ="MY_AssocClass.child=\"My_PersonClass.name=\\\"daughter\\\"\","
        "parent=\"MY_PersonClass.name=\\\"father\\\"\"";
    PEGASUS_TEST_ASSERT(x == assocInstancePath.toString());
    CIMObjectPath px = x;
    PEGASUS_TEST_ASSERT(px.identical(assocInstancePath));
}

Boolean _propertyIdentical(
    const char* propertyName,
    CIMInstance& instance1,
    CIMInstance& instance2)
{
    Uint32 pos = instance1.findProperty(propertyName);
    CIMConstProperty p1 = instance1.getProperty(pos);
    pos = instance2.findProperty(propertyName);
    CIMConstProperty p2 = instance2.getProperty(pos);
    return (p1.identical(p2));
}

//
// Test of the Instance creation and instance filtering
//
void test04()
{
    if (verbose)
    {
        cout << "Test04 - Create instance from Class. " << endl;
    }

    const CIMNamespaceName NAMESPACE = CIMNamespaceName("/zzz");

    // Create and populate a declaration context:

    SimpleDeclContext* context = new SimpleDeclContext;

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("counter"), false,
        CIMScope::PROPERTY));

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("classcounter"), false,
        CIMScope::CLASS));

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("min"), String(),
        CIMScope::PROPERTY));

    context->addQualifierDecl(
        NAMESPACE, CIMQualifierDecl(CIMName("max"), String(),
        CIMScope::PROPERTY));

    context->addQualifierDecl(NAMESPACE,
        CIMQualifierDecl(CIMName("Description"), String(),
        CIMScope::PROPERTY));

    CIMClass class1(CIMName("MyClass"));

    class1
        .addProperty(CIMProperty(CIMName("count"), Uint32(55))
            .addQualifier(CIMQualifier(CIMName("counter"), true))
            .addQualifier(CIMQualifier(CIMName("min"), String("0")))
            .addQualifier(CIMQualifier(CIMName("max"), String("1"))))
        .addProperty(CIMProperty(CIMName("message"), String("Hello"))
            .addQualifier(CIMQualifier(CIMName("description"),
                String("My Message"))))
        .addProperty(CIMProperty(CIMName("ratio"), Real32(1.5)));


    Resolver::resolveClass(class1, context, NAMESPACE);
    context->addClass(NAMESPACE, class1);


    if (verbose)
    {
        XmlWriter::printClassElement(class1);
    }

    //
    // Create instance with qualifiers, classorigin, and Null propertyList
    //
    {
        CIMInstance newInstance;

        newInstance = class1.buildInstance(true, true, CIMPropertyList());

        if (verbose)
        {
            XmlWriter::printInstanceElement(newInstance);
        }

        PEGASUS_TEST_ASSERT(
            newInstance.getPropertyCount() == class1.getPropertyCount());
        PEGASUS_TEST_ASSERT(
            newInstance.getQualifierCount() == class1.getQualifierCount());
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("message") != PEG_NOT_FOUND);

    }

    //
    // Test with include qualifiers false. Should be no qualifiers in result
    //
    {
        CIMInstance newInstance =
            class1.buildInstance(false, true, CIMPropertyList());

        PEGASUS_TEST_ASSERT(newInstance.getQualifierCount() == 0);
        PEGASUS_TEST_ASSERT(
            newInstance.getPropertyCount() == class1.getPropertyCount());
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("message") != PEG_NOT_FOUND);
    }

    //
    // Test with empty property list.  Should have no properties.
    //
    {
        Array<CIMName> pl1Array;
        CIMPropertyList pl1(pl1Array);

        CIMInstance newInstance = class1.buildInstance(false, true, pl1);

        PEGASUS_TEST_ASSERT(newInstance.getQualifierCount() == 0);
        PEGASUS_TEST_ASSERT(newInstance.getPropertyCount() == 0);
    }

    //
    // Test with a property that exists in property list.
    //
    {
        Array<CIMName> pl1Array;
        pl1Array.append("ratio");
        CIMPropertyList pl1(pl1Array);

        CIMInstance newInstance = class1.buildInstance(false, true, pl1);

        if (verbose)
        {
            cout << "Test with one property in new instance" << endl;
            XmlWriter::printInstanceElement(newInstance);
        }

        PEGASUS_TEST_ASSERT(newInstance.getPropertyCount() == 1);
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("message") == PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(newInstance.getQualifierCount() == 0);
    }

    //
    // Test with a property that does/does not exist in property list
    //
    {
        Array<CIMName> pl1Array;
        CIMPropertyList pl1(pl1Array);
        pl1.clear();
        pl1Array.append("blob");
        pl1Array.append("ratio");
        pl1.set(pl1Array);

        CIMInstance newInstance = class1.buildInstance(false, true, pl1);

        PEGASUS_TEST_ASSERT(newInstance.getPropertyCount() == 1);
        PEGASUS_TEST_ASSERT(newInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(newInstance.findProperty("blob") == PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            newInstance.findProperty("message") == PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(newInstance.getQualifierCount() == 0);
    }

    ///////////////////////////////////////////////////////////////////////
    //
    // Instance Filtering function tests
    //
    ///////////////////////////////////////////////////////////////////////

    // build instance as starting point for tests.
    CIMInstance tstInstance =
        class1.buildInstance(true, true, CIMPropertyList());

    //
    // Test complete copy, no change
    //
    {
        if (verbose)
        {
            cout << "Test1" << endl;
        }

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(true, true, CIMPropertyList());

        PEGASUS_TEST_ASSERT(tstInstance.identical(filterInstance));
        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 3);
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() ==
            tstInstance.getQualifierCount());
    }

    //
    // Filter to one property, ratio
    //
    {
        if (verbose)
        {
            cout << "Test2" << endl;
        }

        Array<CIMName> pl1Array;
        pl1Array.append("ratio");
        CIMPropertyList pl1(pl1Array);

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(true, true, pl1);

        if (verbose)
        {
            XmlWriter::printInstanceElement(filterInstance);
        }

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 1);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            _propertyIdentical("ratio", filterInstance, tstInstance));
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() ==
                tstInstance.getQualifierCount());
    }

    //
    // Filter to one property, message
    //
    {
        if (verbose)
        {
            cout << "Test3" << endl;
        }

        Array<CIMName> pl1Array;
        pl1Array.append("message");
        CIMPropertyList pl1(pl1Array);

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(true, true, pl1);

        if (verbose)
        {
            XmlWriter::printInstanceElement(filterInstance);
        }

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 1);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("message") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            _propertyIdentical("message", filterInstance, tstInstance));
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() ==
                tstInstance.getQualifierCount());
    }

    //
    // Filter to one property, count
    //
    {
        if (verbose)
        {
            cout << "Test4" << endl;
        }

        Array<CIMName> pl1Array;
        pl1Array.append("count");
        CIMPropertyList pl1(pl1Array);

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(true, true, pl1);

        if (verbose)
        {
            XmlWriter::printInstanceElement(filterInstance);
        }

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 1);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("count") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() ==
                tstInstance.getQualifierCount());
    }

    //
    // Filter to no properties
    //
    {
        if (verbose)
        {
            cout << "Test5a" << endl;
        }

        Array<CIMName> pl1Array;
        CIMPropertyList pl1(pl1Array);

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(true, true, pl1);

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 0);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("ratio") == PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() ==
            tstInstance.getQualifierCount());
    }

    //
    // Filter to two properties
    //
    {
        if (verbose)
        {
            cout << "Test5b" << endl;
        }

        Array<CIMName> pl1Array;
        pl1Array.append("count");
        pl1Array.append("message");
        CIMPropertyList pl1(pl1Array);

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(true, true, pl1);

        PEGASUS_TEST_ASSERT(
            filterInstance.getPropertyCount() == 2);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("ratio") == PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("message") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            _propertyIdentical("message", filterInstance, tstInstance));
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("count") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(
            _propertyIdentical("count", filterInstance, tstInstance));
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() ==
                tstInstance.getQualifierCount());
    }

    //
    // Filter to no qualifiers and all properties.
    //
    {
        if (verbose)
        {
            cout << "Test6" << endl;
        }

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(false, true, CIMPropertyList());

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 3);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() == 0);
        for (Uint32 i = 0; i < filterInstance.getPropertyCount() ; i++)
        {
            CIMConstProperty p = filterInstance.getProperty(i);
            PEGASUS_TEST_ASSERT(p.getQualifierCount() == 0);
        }
    }

    //
    // Filter to no qualifiers and no properties.
    //
    {
        if (verbose)
        {
            cout << "Test6a" << endl;
        }

        Array<CIMName> pl1Array;
        CIMPropertyList pl1(pl1Array);

        CIMInstance filterInstance = tstInstance.clone();
        filterInstance.filter(false, true, pl1);

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 0);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("ratio") == PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() == 0);
    }

    //
    // Test Class Origin Filter
    //
    {
        if (verbose)
        {
            cout << "Test7 Class Origin" << endl;
        }

        // Create a subclass to do classOrigin testing
        CIMClass mySubClass(CIMName("subclass"));
        mySubClass.setSuperClassName(CIMName("MyClass"));

        Resolver::resolveClass(mySubClass, context, NAMESPACE);
        context->addClass(NAMESPACE, mySubClass);

        // build instance
        CIMInstance filterInstance =
            mySubClass.buildInstance(true, true, CIMPropertyList());
        filterInstance.filter(false, true, CIMPropertyList());

        PEGASUS_TEST_ASSERT(filterInstance.getPropertyCount() == 3);
        PEGASUS_TEST_ASSERT(
            filterInstance.findProperty("ratio") != PEG_NOT_FOUND);
        PEGASUS_TEST_ASSERT(filterInstance.getQualifierCount() == 0);
        for (Uint32 i = 0 ; i < filterInstance.getPropertyCount() ; i++)
        {
            CIMProperty p = filterInstance.getProperty(i);
            PEGASUS_TEST_ASSERT(!(p.getClassOrigin() == CIMName()));

        }
        filterInstance.filter(false, false, CIMPropertyList());

        for (Uint32 i = 0 ; i < filterInstance.getPropertyCount() ; i++)
        {
            CIMProperty p = filterInstance.getProperty(i);
            PEGASUS_TEST_ASSERT(p.getClassOrigin() == CIMName());
        }

        CIMInstance filterInstance2 =
            mySubClass.buildInstance(true, false, CIMPropertyList());

        for (Uint32 i = 0 ; i < filterInstance2.getPropertyCount() ; i++)
        {
            CIMProperty p = filterInstance2.getProperty(i);
            PEGASUS_TEST_ASSERT(p.getClassOrigin() == CIMName());
        }
    }
    delete context;
}

//
// Test instance qualifier operations
//
void test05()
{
    CIMInstance instance1(CIMName("MyClass"));
    instance1.addQualifier(CIMQualifier(CIMName("Qualifier1"), true));
    instance1.addQualifier(CIMQualifier(CIMName("Qualifier2"), Uint32(1)));
    instance1.addQualifier(CIMQualifier(CIMName("Qualifier3"), String("a")));

    Boolean caughtException = false;

    try
    {
        instance1.addQualifier(CIMQualifier(CIMName("Qualifier1"), true));
    }
    catch (const AlreadyExistsException&)
    {
        caughtException = true;
    }

    PEGASUS_TEST_ASSERT(caughtException);
    PEGASUS_TEST_ASSERT(instance1.getQualifierCount() == 3);

    // Test that the correct qualifiers exist in the instance

    Uint32 qualifierIndex;

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier1"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier2"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier3"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier4"));
    PEGASUS_TEST_ASSERT(qualifierIndex == PEG_NOT_FOUND);

    // Clone the instance and test for the correct qualifiers

    CIMConstInstance instance2 = instance1.clone();
    PEGASUS_TEST_ASSERT(instance2.getQualifierCount() == 3);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier1"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier2"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier3"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier4"));
    PEGASUS_TEST_ASSERT(qualifierIndex == PEG_NOT_FOUND);

    // Remove Qualifier2 from instance1

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier2"));
    instance1.removeQualifier(qualifierIndex);
    PEGASUS_TEST_ASSERT(instance1.getQualifierCount() == 2);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier2"));
    PEGASUS_TEST_ASSERT(qualifierIndex == PEG_NOT_FOUND);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier1"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    // Confirm that the qualifiers are still in instance2

    PEGASUS_TEST_ASSERT(instance2.getQualifierCount() == 3);
    qualifierIndex = instance2.findQualifier(CIMName("Qualifier1"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier2"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    // Remove a non-existant qualifier

    caughtException = false;

    try
    {
        instance1.removeQualifier(10);
    }
    catch (const IndexOutOfBoundsException&)
    {
        caughtException = true;
    }

    PEGASUS_TEST_ASSERT(caughtException);

    // Remove Qualifier1 and Qualifier3 from instance1

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier1"));
    instance1.removeQualifier(qualifierIndex);
    PEGASUS_TEST_ASSERT(instance1.getQualifierCount() == 1);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier3"));
    instance1.removeQualifier(qualifierIndex);
    PEGASUS_TEST_ASSERT(instance1.getQualifierCount() == 0);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier1"));
    PEGASUS_TEST_ASSERT(qualifierIndex == PEG_NOT_FOUND);

    qualifierIndex = instance1.findQualifier(CIMName("Qualifier3"));
    PEGASUS_TEST_ASSERT(qualifierIndex == PEG_NOT_FOUND);

    // Confirm that the qualifiers are still in instance2

    PEGASUS_TEST_ASSERT(instance2.getQualifierCount() == 3);
    qualifierIndex = instance2.findQualifier(CIMName("Qualifier1"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier2"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);

    qualifierIndex = instance2.findQualifier(CIMName("Qualifier3"));
    PEGASUS_TEST_ASSERT(qualifierIndex != PEG_NOT_FOUND);
}

int main(int argc, char** argv)
{
    verbose = getenv("PEGASUS_TEST_VERBOSE") ? true : false;
    try
    {
        test01();
        test02();
        test03();
        test04();
        test05();
    }
    catch (Exception& e)
    {
        cout << "Exception: " << e.getMessage() << endl;
        exit(1);
    }

    cout << argv[0] << " +++++ passed all tests" << endl;

    return 0;
}
