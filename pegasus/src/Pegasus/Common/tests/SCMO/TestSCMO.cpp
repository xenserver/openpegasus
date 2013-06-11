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

#include "TestSCMO.h"
#include <Pegasus/Common/XmlReader.h>
#include <Pegasus/Common/XmlParser.h>
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/SCMOClassCache.h>


PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

#define VCOUT if (verbose) cout

static Boolean verbose;
static Boolean loadClassOnce;

const String MASTERQUALIFIER ("/src/Pegasus/Common/tests/SCMO/masterQualifier");
const String MASTERCLASS ("/src/Pegasus/Common/tests/SCMO/masterClass");
const String TESTSCMOXML("/src/Pegasus/Common/tests/SCMO/TestSCMO.xml");
const String TESTSCMO2XML("/src/Pegasus/Common/tests/SCMO/TestSCMO2.xml");
const String
   TESTCSCLASSXML("/src/Pegasus/Common/tests/SCMO/CIMComputerSystemClass.xml");
const String
   TESTCSINSTXML("/src/Pegasus/Common/tests/SCMO/CIMComputerSystemInst.xml");

SCMOClass _scmoClassCache_GetClass(
    const CIMNamespaceName& nameSpace,
    const CIMName& className)
{
    CIMClass CIM_TESTClass2;
    Buffer text;

    VCOUT << endl << "Loading CIM SCMO_TESTClass2" << endl;

    // check if the class was alrady loaded.
    // If this fails, the class was requested to be loaded more then once !
    PEGASUS_TEST_ASSERT(!loadClassOnce);

    String TestSCMO2XML (getenv("PEGASUS_ROOT"));
    TestSCMO2XML.append(TESTSCMO2XML);

    FileSystem::loadFileToMemory(text,(const char*)TestSCMO2XML.getCString());

    XmlParser theParser((char*)text.getData());
    XmlReader::getObject(theParser,CIM_TESTClass2);

    // The class was loaded.
    loadClassOnce = true;

    VCOUT << endl << "Done." << endl;

    return SCMOClass(
        CIM_TESTClass2,
        (const char*)nameSpace.getString().getCString());
}

void structureSizes()
{
    cout << endl << "Sizes of structures:" << endl;
    cout << "=====================" << endl << endl;
    cout << "SCMBUnion           : " << sizeof(SCMBUnion) << endl;
    cout << "SCMBDataPtr         : " << sizeof(SCMBDataPtr) << endl;
    cout << "SCMBValue           : " << sizeof(SCMBValue) << endl;
    cout << "SCMBKeyBindingValue : " << sizeof(SCMBKeyBindingValue) << endl;
    cout << "SCMBUserKeyBindingElement : "
         << sizeof(SCMBUserKeyBindingElement) << endl;
    cout << "SCMBQualifier       : " << sizeof(SCMBQualifier) << endl;
    cout << "SCMBMgmt_Header     : " << sizeof(SCMBMgmt_Header) << endl;
    cout << "SCMBClassProperty   : " << sizeof(SCMBClassProperty) << endl;
    cout << "SCMBClassPropertySet_Header : "
         << sizeof(SCMBClassPropertySet_Header) << endl;
    cout << "SCMBClassPropertyNode : " << sizeof(SCMBClassPropertyNode) << endl;
    cout << "SCMBKeyBindingNode  : " << sizeof(SCMBKeyBindingNode) << endl;
    cout << "SCMBKeyBindingSet_Header : "
         << sizeof(SCMBKeyBindingSet_Header) << endl;
    cout << "SCMBClass_Main      : " << sizeof(SCMBClass_Main) << endl;
    cout << "SCMBInstance_Main   : " << sizeof(SCMBInstance_Main) << endl;
}

void CIMClassToSCMOClass()
{
    CIMClass theCIMClass;
    Buffer text;
    VCOUT << endl << "CIMClass to SCMOClass..." << endl;


    VCOUT << "Loading CIM SCMO_TESTClass" << endl;

    String TestSCMOXML (getenv("PEGASUS_ROOT"));
    TestSCMOXML.append(TESTSCMOXML);

    FileSystem::loadFileToMemory(text,(const char*)TestSCMOXML.getCString());

    XmlParser theParser((char*)text.getData());
    XmlReader::getObject(theParser,theCIMClass);


    SCMOClass theSCMOClass(theCIMClass);

    const char TestCSMOClassLog[]="TestSCMOClass.log";
    SCMODump dump(&(TestCSMOClassLog[0]));

    // Do not dump the volatile data.
    dump.dumpSCMOClass(theSCMOClass,false);

    String masterFile (getenv("PEGASUS_ROOT"));
    masterFile.append(MASTERCLASS);

    PEGASUS_TEST_ASSERT(dump.compareFile(masterFile));

    dump.closeFile();
    dump.deleteFile();

    VCOUT << "Creating CIMClass out of SCMOClass." << endl;
    CIMClass newCimClass;

    theSCMOClass.getCIMClass(newCimClass);

    // Before the newCimClass can be compared with the orginal class,
    // the methods of the original class has to be removed,
    // because the SCMOClass currently does not support methods.
    while (0 != theCIMClass.getMethodCount())
    {
        theCIMClass.removeMethod(0);
    }

    PEGASUS_TEST_ASSERT(newCimClass.identical(theCIMClass));

    VCOUT << "Creaing SCMO instance out of SCMOClass." << endl;

    SCMOInstance theSCMOInstance(theSCMOClass);

    char TestSCMOClass[]= "TestSCMO Class";
    SCMBUnion tmp;
    tmp.extString.pchar = &(TestSCMOClass[0]);
    tmp.extString.length = strlen(TestSCMOClass);

    theSCMOInstance.setPropertyWithOrigin(
        "CreationClassName",
        CIMTYPE_STRING,
        &tmp);

    VCOUT << "Test of building key bindings from properties." << endl;

    char ThisIsTheName[] = "This is the Name";
    SCMBUnion tmp2;
    tmp2.extString.pchar  = &(ThisIsTheName[0]);
    tmp2.extString.length = strlen(ThisIsTheName);

    const SCMBUnion * tmp3;
    CIMType keyType;
    CIMType cimType;
    Boolean isArray;
    Uint32 number;
    SCMO_RC rc;

    theSCMOInstance.buildKeyBindingsFromProperties();

    // Only one of two key properties are set in the instance.
    // After creating key bindings out of key properies,
    // the key binding 'Name' must not be set.

    rc = theSCMOInstance.getKeyBinding("Name",keyType,&tmp3);
    PEGASUS_TEST_ASSERT(rc == SCMO_NULL_VALUE);
    PEGASUS_TEST_ASSERT(keyType == CIMTYPE_STRING);

    theSCMOInstance.setPropertyWithOrigin(
        "Name",
        CIMTYPE_STRING,
        &tmp2);

    theSCMOInstance.buildKeyBindingsFromProperties();

    // The key binding should be generated out of the property.

    theSCMOInstance.getKeyBinding("Name",keyType,&tmp3);

    PEGASUS_TEST_ASSERT(keyType == CIMTYPE_STRING);
    PEGASUS_TEST_ASSERT(tmp3->extString.length==tmp2.extString.length);
    PEGASUS_TEST_ASSERT(strcmp(tmp3->extString.pchar,tmp2.extString.pchar)==0);

    // do not forget to clean up tmp3, because it is a string
    free((void*)tmp3);

    VCOUT << "Test of cloning SCMOInstances." << endl;

    SCMOInstance cloneInstance = theSCMOInstance.clone();

    SCMOInstance asObjectPath = theSCMOInstance.clone(true);

    // The key binding of the orignal instance has to be set on the only
    // objectpath cloning.
    asObjectPath.getKeyBinding("Name",keyType,&tmp3);

    PEGASUS_TEST_ASSERT(keyType == CIMTYPE_STRING);
    PEGASUS_TEST_ASSERT(tmp3->extString.length==tmp2.extString.length);
    PEGASUS_TEST_ASSERT(strcmp(tmp3->extString.pchar,tmp2.extString.pchar)==0);

    // do not forget to clean up tmp3, because it is a string
    free((void*)tmp3);

    // But the associated key property has to be empty.
    rc = asObjectPath.getProperty("Name",cimType,&tmp3,isArray,number);

    PEGASUS_TEST_ASSERT(tmp3 == NULL);
    PEGASUS_TEST_ASSERT(rc == SCMO_NULL_VALUE);

    // But the in the full clone the property has to be set...
    rc = cloneInstance.getProperty("Name",cimType,&tmp3,isArray,number);

    PEGASUS_TEST_ASSERT(keyType == CIMTYPE_STRING);
    PEGASUS_TEST_ASSERT(tmp3->extString.length==tmp2.extString.length);
    PEGASUS_TEST_ASSERT(strcmp(tmp3->extString.pchar,tmp2.extString.pchar)==0);

    // do not forget to clean up tmp3, because it is a string
    free((void*)tmp3);

    VCOUT << endl << "Test 1: Done." << endl;
}

void SCMOClassQualifierTest()
{
    VCOUT << endl << "Getting SCMOClass from cache ..." << endl;

    SCMOClassCache* _theCache = SCMOClassCache::getInstance();

    SCMOClass SCMO_TESTClass2 = _theCache->getSCMOClass(
            "cimv2",
            strlen("cimv2"),
            "SCMO_TESTClass2",
            strlen("SCMO_TESTClass2"));

    VCOUT << endl << "SCMOClass qualifer test ..." << endl;

    String masterFile (getenv("PEGASUS_ROOT"));
    masterFile.append(MASTERQUALIFIER);

    SCMODump dump("TestSCMOClassQualifier.log");

    dump.dumpSCMOClassQualifiers(SCMO_TESTClass2);

    PEGASUS_TEST_ASSERT(dump.compareFile(masterFile));

    dump.deleteFile();

    VCOUT << "Done." << endl;
}

void SCMOInstancePropertyTest()
{
    SCMO_RC rc;

    // definition of return values.
    const SCMBUnion* unionReturn;
    CIMType typeReturn;
    Boolean isArrayReturn;
    Uint32 sizeReturn;


    SCMBUnion boolValue;
    boolValue.simple.val.bin=true;
    boolValue.simple.hasValue=true;

    SCMOClassCache* _theCache = SCMOClassCache::getInstance();

    SCMOClass SCMO_TESTClass2 = _theCache->getSCMOClass(
            "cimv2",
            strlen("cimv2"),
            "SCMO_TESTClass2",
            strlen("SCMO_TESTClass2"));

    SCMOInstance SCMO_TESTClass2_Inst(SCMO_TESTClass2);

    /**
     * Negative test cases for setting a propertty
     */

    VCOUT << endl <<
        "SCMOInstance Negative test cases for setting a property ..."
        << endl << endl;

    VCOUT << "Invalid property name." << endl;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "NotAProperty",
        CIMTYPE_BOOLEAN,
        &boolValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_NOT_FOUND);

    VCOUT << "Property type is differente." << endl;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "DateTimeProperty",
        CIMTYPE_BOOLEAN,
        &boolValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_WRONG_TYPE);

    VCOUT << "Property is not an array." << endl;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "BooleanProperty",
        CIMTYPE_BOOLEAN,
        &boolValue,
        true,10);

    PEGASUS_TEST_ASSERT(rc==SCMO_NOT_AN_ARRAY);

    VCOUT << "Value is not an array." << endl;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "BooleanPropertyArray",
        CIMTYPE_BOOLEAN,
        &boolValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_IS_AN_ARRAY);

    VCOUT << "Empty Array." << endl;

    // this just to get an valid pointer but I put no elements in it
    SCMBUnion *uint32ArrayValue = (SCMBUnion*)malloc(1*sizeof(Uint32));

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint32PropertyArray",
        CIMTYPE_UINT32,
        uint32ArrayValue,
        true,0);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint32PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(unionReturn==NULL);

    free(uint32ArrayValue);

    VCOUT << "Get default value of the class." << endl;
    rc = SCMO_TESTClass2_Inst.getProperty(
        "BooleanProperty",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    // The default value of this instance is TRUE
    PEGASUS_TEST_ASSERT(unionReturn->simple.val.bin);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);


    VCOUT << endl << "Done." << endl << endl;

    VCOUT << "SCMOInstance setting and reading properties ..." << endl;

    /**
     * Test Char16
     */

    VCOUT << endl << "Test Char16" << endl;

    SCMBUnion char16value;
    char16value.simple.val.c16=0x3F4A;
    char16value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Char16Property",
        CIMTYPE_CHAR16,
        &char16value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Char16Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);
    PEGASUS_TEST_ASSERT(
        char16value.simple.val.c16 == unionReturn->simple.val.c16);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion char16ArrayValue[3];
    char16ArrayValue[0].simple.val.c16 = 1024;
    char16ArrayValue[0].simple.hasValue=true;
    char16ArrayValue[1].simple.val.c16 = 2048;
    char16ArrayValue[1].simple.hasValue=true;
    char16ArrayValue[2].simple.val.c16 = 4096;
    char16ArrayValue[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Char16PropertyArray",
        CIMTYPE_CHAR16,
        char16ArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Char16PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        char16ArrayValue[0].simple.val.c16 == unionReturn[0].simple.val.c16);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        char16ArrayValue[1].simple.val.c16 == unionReturn[1].simple.val.c16);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        char16ArrayValue[2].simple.val.c16 == unionReturn[2].simple.val.c16);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test Uint8
     */
    VCOUT << "Test Uint8" << endl;

    SCMBUnion uint8value;
    uint8value.simple.val.u8=0x77;
    uint8value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint8Property",
        CIMTYPE_UINT8,
        &uint8value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint8Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        uint8value.simple.val.u8 == unionReturn->simple.val.u8);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion uint8ArrayValue[3];
    uint8ArrayValue[0].simple.val.u8 = 42;
    uint8ArrayValue[0].simple.hasValue=true;
    uint8ArrayValue[1].simple.val.u8 = 155;
    uint8ArrayValue[1].simple.hasValue=true;
    uint8ArrayValue[2].simple.val.u8 = 192;
    uint8ArrayValue[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint8PropertyArray",
        CIMTYPE_UINT8,
        uint8ArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint8PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        uint8ArrayValue[0].simple.val.u8 == unionReturn[0].simple.val.u8);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint8ArrayValue[1].simple.val.u8 == unionReturn[1].simple.val.u8);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint8ArrayValue[2].simple.val.u8 == unionReturn[2].simple.val.u8);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test Uint16
     */

    VCOUT << "Test Uint16" << endl;

    SCMBUnion uint16value;
    uint16value.simple.val.u16=0xF77F;
    uint16value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint16Property",
        CIMTYPE_UINT16,
        &uint16value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint16Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        uint16value.simple.val.u16 == unionReturn->simple.val.u16);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion uint16ArrayValue[3];
    uint16ArrayValue[0].simple.val.u16 = 218;
    uint16ArrayValue[0].simple.hasValue=true;
    uint16ArrayValue[1].simple.val.u16 = 2673;
    uint16ArrayValue[1].simple.hasValue=true;
    uint16ArrayValue[2].simple.val.u16 = 172;
    uint16ArrayValue[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint16PropertyArray",
        CIMTYPE_UINT16,
        uint16ArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint16PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);

    PEGASUS_TEST_ASSERT(
        uint16ArrayValue[0].simple.val.u16 == unionReturn[0].simple.val.u16);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint16ArrayValue[1].simple.val.u16 == unionReturn[1].simple.val.u16);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint16ArrayValue[2].simple.val.u16 == unionReturn[2].simple.val.u16);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);



    /**
     * Test Uint32
     */

    VCOUT << "Test Uint32" << endl;

    SCMBUnion uint32value;
    uint32value.simple.val.u32=0xF7F7F7F7;
    uint32value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint32Property",
        CIMTYPE_UINT32,
        &uint32value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint32Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        uint32value.simple.val.u32 == unionReturn->simple.val.u32);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion uint32ArrayValue2[3];
    uint32ArrayValue2[0].simple.val.u32 = 42;
    uint32ArrayValue2[0].simple.hasValue=true;
    uint32ArrayValue2[1].simple.val.u32 = 289;
    uint32ArrayValue2[1].simple.hasValue=true;
    uint32ArrayValue2[2].simple.val.u32 = 192;
    uint32ArrayValue2[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint32PropertyArray",
        CIMTYPE_UINT32,
        uint32ArrayValue2,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint32PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        uint32ArrayValue2[0].simple.val.u32 == unionReturn[0].simple.val.u32);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint32ArrayValue2[1].simple.val.u32 == unionReturn[1].simple.val.u32);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint32ArrayValue2[2].simple.val.u32 == unionReturn[2].simple.val.u32);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);


    /**
     * Test Uint64
     */

    VCOUT << "Test Uint64" << endl;

    SCMBUnion uint64value;
    uint64value.simple.val.u64=PEGASUS_UINT64_LITERAL(0xA0A0B0B0C0C0D0D0);
    uint64value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint64Property",
        CIMTYPE_UINT64,
        &uint64value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint64Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        uint64value.simple.val.u64 == unionReturn->simple.val.u64);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion uint64ArrayValue[4];
    uint64ArrayValue[0].simple.val.u64 = 394;
    uint64ArrayValue[0].simple.hasValue=true;
    uint64ArrayValue[1].simple.val.u64 = 483734;
    uint64ArrayValue[1].simple.hasValue=true;
    uint64ArrayValue[2].simple.val.u64 =
        PEGASUS_UINT64_LITERAL(0x1234567890ABCDEF);
    uint64ArrayValue[2].simple.hasValue=true;
    uint64ArrayValue[3].simple.val.u64 = 23903483;
    uint64ArrayValue[3].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint64PropertyArray",
        CIMTYPE_UINT64,
        uint64ArrayValue,
        true,4);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint64PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==4);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        uint64ArrayValue[0].simple.val.u64 == unionReturn[0].simple.val.u64);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint64ArrayValue[1].simple.val.u64 == unionReturn[1].simple.val.u64);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint64ArrayValue[2].simple.val.u64 == unionReturn[2].simple.val.u64);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        uint64ArrayValue[3].simple.val.u64 == unionReturn[3].simple.val.u64);
    PEGASUS_TEST_ASSERT(unionReturn[3].simple.hasValue);

    /**
     * Test Sint8
     */
    VCOUT << "Test Sint8" << endl;

    SCMBUnion sint8value;
    sint8value.simple.val.s8=Sint8(0xF3);
    sint8value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint8Property",
        CIMTYPE_SINT8,
        &sint8value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint8Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        sint8value.simple.val.s8 == unionReturn->simple.val.s8);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion sint8ArrayValue[3];
    sint8ArrayValue[0].simple.val.s8 = -2;
    sint8ArrayValue[0].simple.hasValue=true;
    sint8ArrayValue[1].simple.val.s8 = 94;
    sint8ArrayValue[1].simple.hasValue=true;
    sint8ArrayValue[2].simple.val.s8 = -123;
    sint8ArrayValue[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint8PropertyArray",
        CIMTYPE_SINT8,
        sint8ArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint8PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        sint8ArrayValue[0].simple.val.s8 == unionReturn[0].simple.val.s8);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint8ArrayValue[1].simple.val.s8 == unionReturn[1].simple.val.s8);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint8ArrayValue[2].simple.val.s8 == unionReturn[2].simple.val.s8);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test Sint16
     */

    VCOUT << "Test Sint16" << endl;

    SCMBUnion sint16value;
    sint16value.simple.val.s16=Sint16(0xF24B);
    sint16value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint16Property",
        CIMTYPE_SINT16,
        &sint16value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint16Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        sint16value.simple.val.s16 == unionReturn->simple.val.s16);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion sint16ArrayValue[4];
    sint16ArrayValue[0].simple.val.s16 = Sint16(-8);
    sint16ArrayValue[0].simple.hasValue=true;
    sint16ArrayValue[1].simple.val.s16 = Sint16(23872);
    sint16ArrayValue[1].simple.hasValue=true;
    sint16ArrayValue[2].simple.val.s16 = Sint16(334);
    sint16ArrayValue[2].simple.hasValue=true;
    sint16ArrayValue[3].simple.val.s16 = Sint16(0xF00F);
    sint16ArrayValue[3].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint16PropertyArray",
        CIMTYPE_SINT16,
        sint16ArrayValue,
        true,4);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint16PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==4);
    PEGASUS_TEST_ASSERT(isArrayReturn);

    PEGASUS_TEST_ASSERT(
        sint16ArrayValue[0].simple.val.s16 == unionReturn[0].simple.val.s16);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint16ArrayValue[1].simple.val.s16 == unionReturn[1].simple.val.s16);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint16ArrayValue[2].simple.val.s16 == unionReturn[2].simple.val.s16);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint16ArrayValue[3].simple.val.s16 == unionReturn[3].simple.val.s16);
    PEGASUS_TEST_ASSERT(unionReturn[3].simple.hasValue);

    /**
     * Test Sint32
     */

    VCOUT << "Test Sint32" << endl;

    SCMBUnion sint32value;
    sint32value.simple.val.s32=0xF0783C;
    sint32value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint32Property",
        CIMTYPE_SINT32,
        &sint32value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint32Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        sint32value.simple.val.s32 == unionReturn->simple.val.s32);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion sint32ArrayValue2[3];
    sint32ArrayValue2[0].simple.val.s32 = 42;
    sint32ArrayValue2[0].simple.hasValue=true;
    sint32ArrayValue2[1].simple.val.s32 = -28937332;
    sint32ArrayValue2[1].simple.hasValue=true;
    sint32ArrayValue2[2].simple.val.s32 = 19248372;
    sint32ArrayValue2[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint32PropertyArray",
        CIMTYPE_SINT32,
        sint32ArrayValue2,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint32PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        sint32ArrayValue2[0].simple.val.s32 == unionReturn[0].simple.val.s32);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint32ArrayValue2[1].simple.val.s32 == unionReturn[1].simple.val.s32);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint32ArrayValue2[2].simple.val.s32 == unionReturn[2].simple.val.s32);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test Uint64
     */

    VCOUT << "Test Uint64" << endl;

    SCMBUnion sint64value;
    sint64value.simple.val.s64=(Sint64)-1;
    sint64value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint64Property",
        CIMTYPE_SINT64,
        &sint64value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint64Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        sint64value.simple.val.s64 == unionReturn->simple.val.s64);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion sint64ArrayValue[4];
    sint64ArrayValue[0].simple.val.s64 = 394;
    sint64ArrayValue[0].simple.hasValue=true;
    sint64ArrayValue[1].simple.val.s64 = -483734324;
    sint64ArrayValue[1].simple.hasValue=true;
    sint64ArrayValue[2].simple.val.s64 = 232349034;
    sint64ArrayValue[2].simple.hasValue=true;
    sint64ArrayValue[3].simple.val.s64 = 0;
    sint64ArrayValue[3].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Sint64PropertyArray",
        CIMTYPE_SINT64,
        sint64ArrayValue,
        true,4);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Sint64PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(sizeReturn==4);
    PEGASUS_TEST_ASSERT(isArrayReturn);
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    PEGASUS_TEST_ASSERT(
        sint64ArrayValue[0].simple.val.s64 == unionReturn[0].simple.val.s64);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint64ArrayValue[1].simple.val.s64 == unionReturn[1].simple.val.s64);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint64ArrayValue[2].simple.val.s64 == unionReturn[2].simple.val.s64);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        sint64ArrayValue[3].simple.val.s64 == unionReturn[3].simple.val.s64);
    PEGASUS_TEST_ASSERT(unionReturn[3].simple.hasValue);

    /**
     * Test Real32
     */

    VCOUT << "Test Real32" << endl;

    SCMBUnion real32value;
    real32value.simple.val.r32=Real32(2.4271e-4);
    real32value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Real32Property",
        CIMTYPE_REAL32,
        &real32value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Real32Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        real32value.simple.val.r32 == unionReturn->simple.val.r32);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion real32ArrayValue[3];
    real32ArrayValue[0].simple.val.r32 = Real32(3.94e30);
    real32ArrayValue[0].simple.hasValue=true;
    real32ArrayValue[1].simple.val.r32 = Real32(-4.83734324e-35);
    real32ArrayValue[1].simple.hasValue=true;
    real32ArrayValue[2].simple.val.r32 = Real32(2.323490e34);
    real32ArrayValue[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Real32PropertyArray",
        CIMTYPE_REAL32,
        real32ArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Real32PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);

    PEGASUS_TEST_ASSERT(
        real32ArrayValue[0].simple.val.r32 == unionReturn[0].simple.val.r32);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        real32ArrayValue[1].simple.val.r32 == unionReturn[1].simple.val.r32);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        real32ArrayValue[2].simple.val.r32 == unionReturn[2].simple.val.r32);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test Real64
     */

    VCOUT << "Test Real64" << endl;

    SCMBUnion real64value;
    real64value.simple.val.r64=Real64(2.4271e-40);
    real64value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Real64Property",
        CIMTYPE_REAL64,
        &real64value);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Real64Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        real64value.simple.val.r64 == unionReturn->simple.val.r64);
    PEGASUS_TEST_ASSERT(unionReturn->simple.hasValue);

    SCMBUnion real64ArrayValue[3];
    real64ArrayValue[0].simple.val.r64 = Real64(3.94e38);
    real64ArrayValue[0].simple.hasValue=true;
    real64ArrayValue[1].simple.val.r64 = Real64(-4.83734644e-35);
    real64ArrayValue[1].simple.hasValue=true;
    real64ArrayValue[2].simple.val.r64 = Real64(2.643490e34);
    real64ArrayValue[2].simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Real64PropertyArray",
        CIMTYPE_REAL64,
        real64ArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Real64PropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);

    PEGASUS_TEST_ASSERT(
        real64ArrayValue[0].simple.val.r64 == unionReturn[0].simple.val.r64);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        real64ArrayValue[1].simple.val.r64 == unionReturn[1].simple.val.r64);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(
        real64ArrayValue[2].simple.val.r64 == unionReturn[2].simple.val.r64);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test Boolean
     */

    VCOUT << "Test Boolean" << endl;

    boolValue.simple.val.bin=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "BooleanProperty",
        CIMTYPE_BOOLEAN,
        &boolValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "BooleanProperty",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(unionReturn->simple.val.bin);

    SCMBUnion boolArrayValue[3];
    boolArrayValue[0].simple.val.bin = true;
    boolArrayValue[0].simple.hasValue= true;
    boolArrayValue[1].simple.val.bin = false;
    boolArrayValue[1].simple.hasValue= true;
    boolArrayValue[2].simple.val.bin = true;
    boolArrayValue[2].simple.hasValue= true;


    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "BooleanPropertyArray",
        CIMTYPE_BOOLEAN,
        boolArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "BooleanPropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);

    PEGASUS_TEST_ASSERT(unionReturn[0].simple.val.bin);
    PEGASUS_TEST_ASSERT(unionReturn[0].simple.hasValue);
    PEGASUS_TEST_ASSERT(!unionReturn[1].simple.val.bin);
    PEGASUS_TEST_ASSERT(unionReturn[1].simple.hasValue);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.val.bin);
    PEGASUS_TEST_ASSERT(unionReturn[2].simple.hasValue);

    /**
     * Test DateTime
     */

    VCOUT << "Test DateTime" << endl;

    SCMBUnion myDateTimeValue;
    myDateTimeValue.dateTimeValue.usec = PEGASUS_UINT64_LITERAL(17236362);
    myDateTimeValue.dateTimeValue.utcOffset = 0;
    myDateTimeValue.dateTimeValue.sign = ':';
    myDateTimeValue.dateTimeValue.numWildcards = 0;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "DateTimeProperty",
        CIMTYPE_DATETIME,
        &myDateTimeValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "DateTimeProperty",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        memcmp(
            &(myDateTimeValue.dateTimeValue),
            &(unionReturn->dateTimeValue),
            sizeof(CIMDateTimeRep))== 0);

    SCMBUnion dateTimeArrayValue[3];

    dateTimeArrayValue[0].dateTimeValue.usec = Uint64(988243387);
    dateTimeArrayValue[0].dateTimeValue.utcOffset = 0;
    dateTimeArrayValue[0].dateTimeValue.sign = ':';
    dateTimeArrayValue[0].dateTimeValue.numWildcards = 0;

    dateTimeArrayValue[1].dateTimeValue.usec = Uint64(827383727);
    dateTimeArrayValue[1].dateTimeValue.utcOffset = 0;
    dateTimeArrayValue[1].dateTimeValue.sign = ':';
    dateTimeArrayValue[1].dateTimeValue.numWildcards = 0;

    dateTimeArrayValue[2].dateTimeValue.usec = Uint64(932933892);
    dateTimeArrayValue[2].dateTimeValue.utcOffset = 0;
    dateTimeArrayValue[2].dateTimeValue.sign = ':';
    dateTimeArrayValue[2].dateTimeValue.numWildcards = 0;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "DateTimePropertyArray",
        CIMTYPE_DATETIME,
        dateTimeArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "DateTimePropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);


    PEGASUS_TEST_ASSERT(
        memcmp(&(dateTimeArrayValue[0].dateTimeValue),
               &(unionReturn[0].dateTimeValue),
               sizeof(CIMDateTimeRep)
               )== 0);
    PEGASUS_TEST_ASSERT(
        memcmp(
            &(dateTimeArrayValue[1].dateTimeValue),
            &(unionReturn[1].dateTimeValue),
            sizeof(CIMDateTimeRep))== 0);
    PEGASUS_TEST_ASSERT(
        memcmp(
            &(dateTimeArrayValue[2].dateTimeValue),
            &(unionReturn[2].dateTimeValue),
            sizeof(CIMDateTimeRep))== 0);

    /**
     * Test string
     */

    VCOUT << "Test String" << endl;

    char ThisIsASingleString[] = "This is a single String!";
    SCMBUnion stringValue;
    stringValue.extString.pchar = &(ThisIsASingleString[0]);
    stringValue.extString.length = strlen(ThisIsASingleString);

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "StringProperty",
        CIMTYPE_STRING,
        &stringValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "StringProperty",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==0);
    PEGASUS_TEST_ASSERT(!isArrayReturn);

    PEGASUS_TEST_ASSERT(
        stringValue.extString.length==unionReturn->extString.length);
    PEGASUS_TEST_ASSERT(
        strcmp(
            stringValue.extString.pchar,
            unionReturn->extString.pchar) == 0);

    free((void*)unionReturn);

    char arraySting0[] = "The Array String Number one.";
    char arraySting1[] = "The Array String Number two.";
    char arraySting2[] = "The Array String Number three.";
    SCMBUnion stringArrayValue[3];
    stringArrayValue[0].extString.pchar= &(arraySting0[0]);
    stringArrayValue[0].extString.length= strlen(arraySting0);

    stringArrayValue[1].extString.pchar=&(arraySting1[0]);
    stringArrayValue[1].extString.length=strlen(arraySting1);

    stringArrayValue[2].extString.pchar=&(arraySting2[0]);
    stringArrayValue[2].extString.length=strlen(arraySting2);

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "StringPropertyArray",
        CIMTYPE_STRING,
        stringArrayValue,
        true,3);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "StringPropertyArray",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(sizeReturn==3);
    PEGASUS_TEST_ASSERT(isArrayReturn);

    PEGASUS_TEST_ASSERT(
        stringArrayValue[0].extString.length==unionReturn[0].extString.length);
    PEGASUS_TEST_ASSERT(
        strcmp(
            stringArrayValue[0].extString.pchar,
            unionReturn[0].extString.pchar) == 0);
    PEGASUS_TEST_ASSERT(
        stringArrayValue[1].extString.length==unionReturn[1].extString.length);
    PEGASUS_TEST_ASSERT(
        strcmp(
            stringArrayValue[1].extString.pchar,
            unionReturn[1].extString.pchar) == 0);
    PEGASUS_TEST_ASSERT(
        stringArrayValue[2].extString.length==unionReturn[2].extString.length);
    PEGASUS_TEST_ASSERT(
        strcmp(
            stringArrayValue[2].extString.pchar,
            unionReturn[2].extString.pchar) == 0);

    // do not forget !!!
    free((void*)unionReturn);

    VCOUT << endl << "Done." << endl << endl;

}

void SCMOInstanceKeyBindingsTest()
{
    SCMO_RC rc;

    CIMType returnKeyBindType;
    const SCMBUnion * returnKeyBindValue;
    Uint32 noKeyBind;
    const char * returnName;

    SCMOClassCache* _theCache = SCMOClassCache::getInstance();

    SCMOClass SCMO_TESTClass2 = _theCache->getSCMOClass(
            "cimv2",
            strlen("cimv2"),
            "SCMO_TESTClass2",
            strlen("SCMO_TESTClass2"));

    SCMOInstance SCMO_TESTClass2_Inst(SCMO_TESTClass2);

    SCMBUnion uint64KeyVal;
    uint64KeyVal.simple.val.u64 = PEGASUS_UINT64_LITERAL(4834987289728);
    uint64KeyVal.simple.hasValue= true;

    SCMBUnion boolKeyVal;
    boolKeyVal.simple.val.bin  = true;
    boolKeyVal.simple.hasValue = true;

    SCMBUnion real32KeyVal;
    real32KeyVal.simple.val.r32 = 3.9399998628365712e+30;
    real32KeyVal.simple.hasValue = true;

    char stringKeyBinding[] = "This is the String key binding.";
    SCMBUnion stringKeyVal;
    stringKeyVal.extString.pchar = &(stringKeyBinding[0]);
    stringKeyVal.extString.length = strlen(stringKeyBinding);

    /**
     * Test Key bindings
     */

    VCOUT << "Key Bindings Tests." << endl << endl;

    VCOUT << "Wrong key binding type." << endl;
    // Real32Property is a key property
    rc = SCMO_TESTClass2_Inst.setKeyBinding(
        "Real32Property",
        CIMTYPE_BOOLEAN,
        &real32KeyVal);

    PEGASUS_TEST_ASSERT(rc==SCMO_TYPE_MISSMATCH);

    VCOUT << "Key binding not set." << endl;

    rc = SCMO_TESTClass2_Inst.getKeyBinding(
        "Real32Property",
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_NULL_VALUE);
    PEGASUS_TEST_ASSERT(returnKeyBindType==CIMTYPE_REAL32);
    PEGASUS_TEST_ASSERT(returnKeyBindValue==NULL);

    // set key bindings

    rc = SCMO_TESTClass2_Inst.setKeyBinding(
        "StringProperty",
        CIMTYPE_STRING,
        &stringKeyVal);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.setKeyBinding(
        "Real32Property",
        CIMTYPE_REAL32,
        &real32KeyVal);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.setKeyBinding(
        "Uint64Property",
        CIMTYPE_UINT64,
        &uint64KeyVal);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getKeyBinding(
        "Real32Property",
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(returnKeyBindType==CIMTYPE_REAL32);
    PEGASUS_TEST_ASSERT(returnKeyBindValue->simple.val.r32 ==
                        real32KeyVal.simple.val.r32);

    rc = SCMO_TESTClass2_Inst.getKeyBinding(
        "Uint64Property",
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(returnKeyBindType==CIMTYPE_UINT64);
    PEGASUS_TEST_ASSERT(returnKeyBindValue->simple.val.u64 ==
                        uint64KeyVal.simple.val.u64);

    rc = SCMO_TESTClass2_Inst.getKeyBinding(
        "StringProperty",
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(returnKeyBindType==CIMTYPE_STRING);

    PEGASUS_TEST_ASSERT(
        stringKeyVal.extString.length==returnKeyBindValue->extString.length);
    PEGASUS_TEST_ASSERT(
        strcmp(
            stringKeyVal.extString.pchar,
            returnKeyBindValue->extString.pchar) == 0);

    // do not forget !
    free((void*)returnKeyBindValue);

    VCOUT << "Get Key binding by index." << endl;

    noKeyBind = SCMO_TESTClass2_Inst.getKeyBindingCount();

    PEGASUS_TEST_ASSERT(noKeyBind==3);

    VCOUT << "Test index boundaries." << endl;

    rc = SCMO_TESTClass2_Inst.getKeyBindingAt(
        noKeyBind,
        &returnName,
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_INDEX_OUT_OF_BOUND);
    PEGASUS_TEST_ASSERT(returnKeyBindValue==NULL);

    VCOUT << "Iterate for index 0 to " << noKeyBind-1 << "." << endl;
    for (Uint32 i = 0; i < noKeyBind; i++)
    {
        rc = SCMO_TESTClass2_Inst.getKeyBindingAt(
            i,
            &returnName,
            returnKeyBindType,
            &returnKeyBindValue);

        if (returnKeyBindType == CIMTYPE_STRING)
        {
            free((void*)returnKeyBindValue);
        }
        PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    }

    VCOUT << "Test User defined Key Bindings." << endl;

    char stringKeyBinding2[]="This is the as User defined String key binding.";
    SCMBUnion stringUserKeyVal;
    stringUserKeyVal.extString.pchar = &(stringKeyBinding2[0]);
    stringUserKeyVal.extString.length = strlen(stringKeyBinding2);

    rc = SCMO_TESTClass2_Inst.setKeyBinding(
        "UserStringProperty",
        CIMTYPE_STRING,
        &stringUserKeyVal);


    rc = SCMO_TESTClass2_Inst.getKeyBinding(
        "UserStringProperty",
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(returnKeyBindType==CIMTYPE_STRING);

    PEGASUS_TEST_ASSERT(
        stringUserKeyVal.extString.length==
        returnKeyBindValue->extString.length);
    PEGASUS_TEST_ASSERT(
        strcmp(
            stringUserKeyVal.extString.pchar,
            returnKeyBindValue->extString.pchar) == 0);

    // do not forget !
    free((void*)returnKeyBindValue);

    VCOUT << "Test User defined Key Bindings with index." << endl;

    noKeyBind = SCMO_TESTClass2_Inst.getKeyBindingCount();
    PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    PEGASUS_TEST_ASSERT(noKeyBind == 4);

    rc = SCMO_TESTClass2_Inst.getKeyBindingAt(
        noKeyBind,
        &returnName,
        returnKeyBindType,
        &returnKeyBindValue);

    PEGASUS_TEST_ASSERT(rc==SCMO_INDEX_OUT_OF_BOUND);
    PEGASUS_TEST_ASSERT(returnKeyBindValue==NULL);

    VCOUT << "Iterate for index 0 to " << noKeyBind-1 << "." << endl;
    for (Uint32 i = 0; i < noKeyBind; i++)
    {
        rc = SCMO_TESTClass2_Inst.getKeyBindingAt(
            i,
            &returnName,
            returnKeyBindType,
            &returnKeyBindValue);

        if (returnKeyBindType == CIMTYPE_STRING)
        {
            free((void*)returnKeyBindValue);
        }
        PEGASUS_TEST_ASSERT(rc==SCMO_OK);
    }

    VCOUT << "Test CIMObjectpath." << endl;

    CIMObjectPath theCIMPath;

    SCMO_TESTClass2_Inst.getCIMObjectPath(theCIMPath);

     Array<CIMKeyBinding> theCIMKeyBindings =
         theCIMPath.getKeyBindings();

    PEGASUS_TEST_ASSERT(theCIMKeyBindings.size() == 4);

    SCMOClass theDummySCMOInstance(SCMO_TESTClass2_Inst.getClassName(),
                  SCMO_TESTClass2_Inst.getNameSpace());

    SCMOInstance theDummyInstance(theDummySCMOInstance,theCIMPath);

    theDummyInstance.markAsCompromised();

    for (Uint32 i = 0 ; i < theDummyInstance.getKeyBindingCount();i++)
    {
        rc = theDummyInstance.getKeyBindingAt(
            i,
            &returnName,
            returnKeyBindType,
            &returnKeyBindValue);

        PEGASUS_ASSERT(rc==SCMO_OK);

        rc = SCMO_TESTClass2_Inst.setKeyBinding(
            returnName,
            returnKeyBindType,
            returnKeyBindValue);

        PEGASUS_ASSERT(rc==SCMO_OK);

        if (returnKeyBindType == CIMTYPE_STRING)
        {
            free((void*)returnKeyBindValue);
        }

    }


    VCOUT << endl << "Done." << endl;
}

void SCMOInstancePropertyFilterTest()
{
    SCMO_RC rc;

    // definition of return values.
    const SCMBUnion* unionReturn;
    CIMType typeReturn;
    Boolean isArrayReturn;
    Uint32 sizeReturn;
    const char * nameReturn;

    SCMOClassCache* _theCache = SCMOClassCache::getInstance();

    SCMOClass SCMO_TESTClass2 = _theCache->getSCMOClass(
            "cimv2",
            strlen("cimv2"),
            "SCMO_TESTClass2",
            strlen("SCMO_TESTClass2"));

    SCMOInstance SCMO_TESTClass2_Inst(SCMO_TESTClass2);

    SCMBUnion uint32value;
    uint32value.simple.val.u32 = 0x654321;
    uint32value.simple.hasValue=true;

    rc = SCMO_TESTClass2_Inst.setPropertyWithOrigin(
        "Uint32Property",
        CIMTYPE_UINT32,
        &uint32value);

    /**
     * Test property filter
     */

    VCOUT << endl << "SCMOInstance property filter tests..." << endl;
    VCOUT << endl << "Set regular property filter." << endl;
    const char* propertyFilter[] =
    {
        "Uint16Property",
        "StringPropertyArray",
        "Sint16PropertyArray",
        "StringProperty",   // it's a key property, already part of the filter.
        0
    };

    SCMO_TESTClass2_Inst.setPropertyFilter(propertyFilter);

    // 6 properties = 4 of filter + 3 key properies - 1 key property,
    // per default part of the filter.

    PEGASUS_TEST_ASSERT(SCMO_TESTClass2_Inst.getPropertyCount()==6);

    for (Uint32 i = 0; i < SCMO_TESTClass2_Inst.getPropertyCount();i++)
    {
        rc = SCMO_TESTClass2_Inst.getPropertyAt(
            i,
            &nameReturn,
            typeReturn,
            &unionReturn,
            isArrayReturn,
            sizeReturn);

        if (typeReturn == CIMTYPE_STRING &&
            rc == SCMO_OK)
        {
                // do not forget !!!
                free((void*)unionReturn);
        }

        PEGASUS_TEST_ASSERT(rc!=SCMO_INDEX_OUT_OF_BOUND);
    }

    VCOUT << "Check indexing." << endl;
    // the indey is just from 0 to 5. 6 is invalid !
    rc = SCMO_TESTClass2_Inst.getPropertyAt(
        SCMO_TESTClass2_Inst.getPropertyCount(),
        &nameReturn,
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);


    PEGASUS_TEST_ASSERT(rc==SCMO_INDEX_OUT_OF_BOUND);

    Uint32 nodeIndex;
    // The property Uint32Property is not part of the filter
    // and not a key property!
    rc = SCMO_TESTClass2_Inst.getPropertyNodeIndex("Uint32Property",nodeIndex);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    uint32value.simple.val.u32 = 0x123456;

    rc = SCMO_TESTClass2_Inst.setPropertyWithNodeIndex(
        nodeIndex,
        CIMTYPE_UINT32,
        &uint32value,
        false,0);

    PEGASUS_TEST_ASSERT(rc==SCMO_OK);

    rc = SCMO_TESTClass2_Inst.getPropertyNodeIndex(NULL,nodeIndex);

    PEGASUS_TEST_ASSERT(rc==SCMO_INVALID_PARAMETER);

    VCOUT << "Reset filter." << endl;
    // reset filter
    SCMO_TESTClass2_Inst.setPropertyFilter(NULL);

    PEGASUS_TEST_ASSERT(SCMO_TESTClass2_Inst.getPropertyCount()==28);

    rc = SCMO_TESTClass2_Inst.getProperty(
        "Uint32Property",
        typeReturn,
        &unionReturn,
        isArrayReturn,
        sizeReturn);

    // The property should not have changed! The setPropertyWithNodeIndex()
    // must not have changed the value.
    PEGASUS_TEST_ASSERT(
        uint32value.simple.val.u32 != unionReturn->simple.val.u32);

    const char* noPropertyFiler[] = { 0 };

    VCOUT << "Empty filter." << endl;
    // no properties in the filter
    SCMO_TESTClass2_Inst.setPropertyFilter(noPropertyFiler);

    // you can not filter out key properties !
    PEGASUS_TEST_ASSERT(SCMO_TESTClass2_Inst.getPropertyCount()==3);

    VCOUT << endl << "Done." << endl;

}

void SCMOInstanceConverterTest()
{

    CIMClass CIM_CSClass;
    CIMInstance CIM_CSInstance;
    Buffer text;

    VCOUT << endl << "Conversion Test CIM<->SCMO.." << endl;
    VCOUT << endl << "Loading CIMComputerSystemClass.xml" << endl;

    String TestCSClassXML (getenv("PEGASUS_ROOT"));
    TestCSClassXML.append(TESTCSCLASSXML);

    FileSystem::loadFileToMemory(text,(const char*)TestCSClassXML.getCString());

    XmlParser theParser((char*)text.getData());
    XmlReader::getObject(theParser,CIM_CSClass);

    text.clear();

    VCOUT << "Loading CIMComputerSystemInstance.xml" << endl;

    String TestCSInstXML (getenv("PEGASUS_ROOT"));
    TestCSInstXML.append(TESTCSINSTXML);

    FileSystem::loadFileToMemory(text,(const char*)TestCSInstXML.getCString());

    XmlParser theParser2((char*)text.getData());
    XmlReader::getObject(theParser2,CIM_CSInstance);

    VCOUT << "Creating SCMOClass from CIMClass" << endl;

    SCMOClass SCMO_CSClass(CIM_CSClass);

    VCOUT << "Creating SCMOInstance from CIMInstance" << endl;
    SCMOInstance SCMO_CSInstance(SCMO_CSClass,CIM_CSInstance);

    CIMInstance newInstance;

    VCOUT << "Converting CIMInstance from SCMOInstance" << endl;
    SCMO_CSInstance.getCIMInstance(newInstance);

    PEGASUS_TEST_ASSERT(newInstance.identical(CIM_CSInstance));

    VCOUT << endl << "Done." << endl << endl;
}


int main (int argc, char *argv[])
{

    CIMClass CIM_TESTClass2;

    verbose = getenv("PEGASUS_TEST_VERBOSE") ? true : false;

    // check if cache is loading the class only once
    loadClassOnce = false;

    try
    {

        if (verbose)
        {
            structureSizes();
        }

        CIMClassToSCMOClass();

        // init the cache.
        SCMOClassCache* _thecache = SCMOClassCache::getInstance();
        _thecache->setCallBack(_scmoClassCache_GetClass);

        SCMOClassQualifierTest();

        SCMOInstancePropertyTest();

        SCMOInstanceKeyBindingsTest();

        SCMOInstancePropertyFilterTest();

        SCMOInstanceConverterTest();

        // destroy the cache.
        _thecache->destroy();
    }
    catch (CIMException& e)
    {
        cout << endl << "CIMException: " ;
        cout << e.getMessage() << endl << endl ;
        exit(-1);
    }

    catch (Exception& e)
    {
        cout << endl << "Exception: " ;
        cout << e.getMessage() << endl << endl ;
        exit(-1);
    }
    catch (...)
    {
        cout << endl << "Unkown excetption!" << endl << endl;
        exit(-1);
    }

    cout << argv[0] << " +++++ passed all tests" << endl;
    return 0;

}
