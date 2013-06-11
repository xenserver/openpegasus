#//%LICENSE////////////////////////////////////////////////////////////////
#//
#// Licensed to The Open Group (TOG) under one or more contributor license
#// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
#// this work for additional information regarding copyright ownership.
#// Each contributor licenses this file to you under the OpenPegasus Open
#// Source License; you may not use this file except in compliance with the
#// License.
#//
#// Permission is hereby granted, free of charge, to any person obtaining a
#// copy of this software and associated documentation files (the "Software"),
#// to deal in the Software without restriction, including without limitation
#// the rights to use, copy, modify, merge, publish, distribute, sublicense,
#// and/or sell copies of the Software, and to permit persons to whom the
#// Software is furnished to do so, subject to the following conditions:
#//
#// The above copyright notice and this permission notice shall be included
#// in all copies or substantial portions of the Software.
#//
#// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#//
#//////////////////////////////////////////////////////////////////////////

# Test cimcli using the cimcli test Provider and other information in the
# server.

ROOT = ../../../../..
DIR = Providers/TestProviders/CLITestProvider/tests

SRC_DIR = $(ROOT)/src/$(DIR)

ifdef PEGASUS_TMP
    TMP_DIR = $(subst \,/,$(PEGASUS_TMP))
else
    TMP_DIR = $(SRC_DIR)
endif


QUOTE=\"

ifeq ($(OS_TYPE),vms)
QUOTE=""
endif

include $(ROOT)/mak/config.mak
include $(ROOT)/mak/configschema.mak

RESULTFILE = $(TMP_DIR)/result
MASTERRESULTFILE = $(SRC_DIR)/result.master

TABLEFORMATFILE = $(TMP_DIR)/tableformat
MASTERTABLEFORMATFILE = $(SRC_DIR)/tableformat.master

HELPRESULTFILE = $(TMP_DIR)/helpresult
ifdef PEGASUS_HAS_SSL
MASTERHELPRESULTFILE = $(SRC_DIR)/helpresult.master.ssl
else
MASTERHELPRESULTFILE = $(SRC_DIR)/helpresult.master
endif

NSRESULTFILE = $(TMP_DIR)/namespacetestresult
MASTERNSRESULTFILE = $(SRC_DIR)/namespacetestresult.master

# sampleprovider\Load puts the sample MOF into a separate namespace.
# The following variable defines this namespace.

PROVIDERNS=test/TestProvider
STATICNS=test/static

## Define this namespace because it is based on CIM 2.5 which is
## completely static and which we can use to test some of the
## class and qualifier operations and feel assured that the
## returned information is constant. The static namespace is defined to
## be the current default and so changes with each new pegasus
## default namespace change.
TESTCIMV2=test/cimv2

depend:

sub:

misc:

tests:

messages:

general:


### Test the getclass operation.  This serves both as a test of getclass
### and a confirmation that we have not modified the class since the output
### diff will fail if the class is modified
getclass:
	@$(ECHO) 0. +++++ Test For getClass
	@$(ECHO) 0. +++++ Test For getClass  >> $(RESULTFILE)

	@cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderClass \
	    >> $(RESULTFILE)
	@cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass \
	    >> $(RESULTFILE)
	@cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass -niq \
	    >> $(RESULTFILE)
##      output as xml and repeat with the -ic option set
	@cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass -o xml \
	    >> $(RESULTFILE)
	@cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass -o xml \
	    -ic >> $(RESULTFILE)
	@$(ECHO) 0. +++++ Test getClass

### Test reference parameters
testRefs:
	@$(ECHO) 1. +++++ Test For Reference params
	@$(ECHO) 1. +++++ Test For Reference params  >> $(RESULTFILE)
##      Test removed.


### test get and set property operations
testPropertyOperations:
	@$(ECHO) 2. +++++ Test Getproperty and setProperty
	@$(ECHO) 2. +++++ Test Getproperty and setProperty >> $(RESULTFILE)

##@cimcli sp  -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" Name John
	@cimcli gp  -n "$(PROVIDERNS)"  \
	    Test_CLITestProviderClass.Id=\"Mike\" Name >> $(RESULTFILE)

##      Test property values for the predefined instance
	@cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass >> $(RESULTFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass >> $(RESULTFILE)
	@$(ECHO) 2. +++++ Test_CLITestProviderClass Instances

### test create instance operations
testCreateInstanceOperation:
	@$(ECHO) 3. +++++ Test Instances create Instance
	@$(ECHO) 3. +++++ Tests Instances create Instance >> $(RESULTFILE)

## Test Instance Creation with scalars
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="ScalarTest" \
	    scalBool=true scalUint8=12 scalSint8=12 scalUint16=500 \
	    scalSint16=500 scalUint32=9999 scalSint32=9999 \
	    scalUint64=99999 scalUint64=99999 scalString="testString" \
	    >> $(RESULTFILE)

##      Test the creation. Will do error exit if this fails
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="ScalarTest" \
	    scalBool=true scalUint8=12 scalSint8=12 scalUint16=500 \
	    scalSint16=500 scalUint32=9999 scalSint32=9999 \
	    scalUint64=99999 scalUint64=99999 scalString="testString" \
	    >> $(RESULTFILE)

##      Test Instance Creation with arrays
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass Id="arrayTest" \
	    arrayBool=true,true,true,false,true \
	    arrayUint8=12  arraySint8=-127,0,12,127 \
	    arrayUint32=99,0,31000,63000,4294967295 \
	    arraySint32=-32000,0,32000,-2147483648,2147483647 \
	    arrayUint64=99,0,31000,63000,18446744073709551615 \
	    arraySint64=-32000,0,32000,-9223372036854775808 \
	    scalString="testString" \
	    scalDateTime=19991224120000.000000+360 >> $(RESULTFILE)

##      run testintance against the instance just created
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id="arrayTest" \
	    arrayBool=true,true,true,false,true \
	    arrayUint8=12  arraySint8=-127,0,12,127 \
	    arrayUint32=99,0,31000,63000,4294967295 \
	    arraySint32=-32000,0,32000,-2147483648,2147483647 \
	    arrayUint64=99,0,31000,63000,18446744073709551615 \
	    arraySint64=-32000,0,32000,-9223372036854775808 \
	    scalString="testString" \
	    scalDateTime=19991224120000.000000+360 >> $(RESULTFILE)

##      Test enumerate of All the instances we created
	@cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass >> $(RESULTFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass >> $(RESULTFILE)

### Test multiple forms of modifyInstance. Requires instance created in
### test 3
testModifyInstanceOperation:
	@$(ECHO) 4. +++++ Test Instances modify Instance
	@$(ECHO) 4. +++++ Test Instances modify Instance >> $(RESULTFILE)

	@cimcli mi -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="ScalarTest" \
	    scalBool=false scalUint8=14 scalSint8=-12 scalUint16=501 \
	    scalSint16=501 scalUint32=9998 scalSint32=9999 \
	    scalUint64=888 scalUint64=1 >> $(RESULTFILE)   
	     
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="ScalarTest" \
	    scalBool=false scalUint8=14 scalSint8=-12 scalUint16=501 \
	    scalSint16=501 scalUint32=9998 scalSint32=9999 \
	    scalUint64=888 scalUint64=1 scalString=testString >> $(RESULTFILE)

#####   NOTE: This test reflects a possible error in Pegasus today.  The
#####       key property should not be required in the input.
	@cimcli mi -n "$(PROVIDERNS)" \
	    Test_CLITestProviderClass.Id=\"ScalarTest\" \
	    Id=ScalarTest \
	    scalBool=true scalUint8=15 scalSint8=-13 scalUint16=502 \
	    scalSint16=502 scalUint32=9997 scalSint32=9998 \
	    scalUint64=888 scalUint64=1 >> $(RESULTFILE)

	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=ScalarTest \
	    scalBool=true scalUint8=15 scalSint8=-13 scalUint16=502 \
	    scalSint16=502 scalUint32=9997 scalSint32=9998 \
	    scalUint64=888 scalUint64=1 >> $(RESULTFILE)

	@$(ECHO) +++++ Test modify instance Passed.


### Test Delete instance. Assumes instances created with testCreateInstance
testDeleteInstanceOperation:	
	@$(ECHO) 5. +++++ Test delete instance
	@$(ECHO) 5. +++++ Test delete instance >> $(RESULTFILE)

##      test with CIMObjectPath input parameter
	@cimcli di -n "$(PROVIDERNS)" \
	    Test_CLITestProviderClass.Id=\"ScalarTest\" >> $(RESULTFILE)

##      test delete with class name=value input parameter
	@cimcli di -n "$(PROVIDERNS)" \
	    Test_CLITestProviderClass Id=arrayTest >> $(RESULTFILE)

	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="ScalarTest" \
	    scalBool=true  \
	    scalUint8=12 scalSint8=12 \
	    scalUint16=500 scalSint16=500 \
	    scalUint32=9999 scalSint32=9999 \
	    scalUint64=99999 scalUint64=99999 \
	    scalString="testString" \
	    >> $(RESULTFILE)

##      Test the creation. Will do error exit if this fails
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="ScalarTest" \
	    scalBool=true \
	    scalUint8=12 scalSint8=12 \
	    scalUint16=500 scalSint16=500 \
	    scalUint32=9999 scalSint32=9999 \
	    scalUint64=99999 scalUint64=99999 \
	    scalString="testString" \
	    >> $(RESULTFILE)

	@cimcli di -n "$(PROVIDERNS)" \
	    Test_CLITestProviderClass Id=ScalarTest >> $(RESULTFILE)

	@$(ECHO) +++++ Test delete instance Passed.

### Test invoke method with a variety of parameter data types
testInvokeMethod:
	@$(ECHO) 6. +++++ Test invokeMethod
	@$(ECHO) 6. +++++ Test invokeMethod >> $(RESULTFILE)

## 	Test using inOutParameter method that returns all
##      input parameters as corresponding output parameters
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    InOutParamTest \
	    scalBool=true \
	    arrayBool=true,false,true \
	    scalUint32=99 \
	    arrayUint32=99,0,31000,63000 \
	    scalSint32=-99 \
	    arraySint32=-32000,0,32000 \
	    scalString=ThisIsMyTest \
	    arrayString="one",two,"three","multiword string" \
	    scalDateTime=19991224120000.000000+360 \
	    arrayDateTime=19991224120000.000000+360,19991224120000.000000+360 \
	    >>$(RESULTFILE)
	@$(ECHO) +++++ Test invoke method Passed.

### Test cimcli output with the table output format
testTableOutputFormat:
	@$(ECHO) 7. +++++ Test table output format
	@$(ECHO) 7. +++++ Test table output format >> $(TABLEFORMATFILE)

	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Norman Name=Susan
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Thomas Name=Peter
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    -o table \
	    >> $(TABLEFORMATFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    -o table \
	    -pl Id,Name,scaleBool,scalUint8,arrayBool \
	    >> $(TABLEFORMATFILE)

	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Norman\"
	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Thomas\"
	@$(ECHO) +++++ Test table output format Passed.

### Test qualifer declaration get and enumerate.
testQualifierDecls:
	@$(ECHO) 8. +++++ Test QualifierDecl Operations
	@$(ECHO) 8. +++++ Test QualifierDecl Operations >> $(RESULTFILE)

	@cimcli gq -n "$(TESTCIMV2)" Association >>$(RESULTFILE)
##      Use sort to assure ordering of response objects
	@cimcli eq -n "$(TESTCIMV2)" --sort >>$(RESULTFILE)
	@$(ECHO) +++++ Test QualifierDecls Passed.

### Test the class operations.  Note that we may have issues with the
### enumerates and ordering. Also we use the static namespace to 
### assure that the classes do not change
testClassOperations:
	@$(ECHO) 9. +++++ getClass Operation
	@$(ECHO) 9. +++++ getClass Operation >> $(RESULTFILE)

	@cimcli nc -n $(STATICNS) --sum -count 67 >>$(RESULTFILE)
	@cimcli nc -n $(STATICNS) --sort >>$(RESULTFILE)
	@cimcli gc CIM_ManagedElement -n $(STATICNS) >>$(RESULTFILE)
	@cimcli gc CIM_ManagedElement -n $(STATICNS) -o xml >>$(RESULTFILE)
	@$(ECHO) +++++ Test ClassOperations Passed.

### Test different options on propertylist parameter
testPropertyListOption:
	@$(ECHO) 10. +++++ testPropertyListOptions
	@$(ECHO) 10. +++++ testPropertyListOptions >> $(RESULTFILE)

	@cimcli gc CIM_ManagedElement -n $(STATICNS) -pl Caption \
	    >>$(RESULTFILE)
	@cimcli gc CIM_ManagedElement -n $(STATICNS) \
	    -pl Caption,Description >>$(RESULTFILE)
	@cimcli gc CIM_ManagedElement -n $(STATICNS) \
	 -pl Caption,Description,ElementName >>$(RESULTFILE)
	@cimcli gc CIM_ManagedElement -n $(STATICNS) -pl "" >>$(RESULTFILE)

	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    -pl scalBool,arrayUint64 >>$(RESULTFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    -pl "" >>$(RESULTFILE)

	@cimcli gi -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    -pl scalBool,arrayUint64,scalString  >>$(RESULTFILE)
	@$(ECHO) +++++ Test testPropertyListOption Passed.

### Test the different formats for input of getinstance operation
testGetInstanceOperation:
	@$(ECHO) 12. +++++ test getInstance Operation
	@$(ECHO) 12. +++++ test getInstance Operation >> $(RESULTFILE)

	@cimcli gi Test_CLITestProviderClass -n "$(PROVIDERNS)" Id=Mike \
	    >>$(RESULTFILE)

	@cimcli gi Test_CLITestProviderClass -n "$(PROVIDERNS)" Id=Mike \
	    -pl Name,scalBool >>$(RESULTFILE)

	@cimcli gi Test_CLITestProviderClass.Id=\"Mike\" -n "$(PROVIDERNS)" \
	    -pl Name,scalBool >>$(RESULTFILE)
	   
	@$(ECHO) +++++ Test getInstance Passed.

### Test the testInstance Operation
### Tests for positive (No error responses).
testTestInstanceOperation:
	@$(ECHO) 13. +++++ test TestInstance Operations
	@$(ECHO) 13. +++++ test TestInstance Operations >> $(RESULTFILE)

	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=Mike \
	    Name=Bob  >>$(RESULTFILE)
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    Name=Bob  >>$(RESULTFILE)
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=Mike \
	    scalSint32=100  >>$(RESULTFILE)
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=Mike \
	    scalBool=true \
	    scalSint32=100  >>$(RESULTFILE)
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=Mike \
	    arrayUint32=4,128,240  >>$(RESULTFILE)
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=Mike \
	    arrayString="First","Second","Third"  >>$(RESULTFILE)
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=Mike \
	    scalBool=true \
	    scalSint32=100 \
	    arrayUint32=4,128,240 \
	    arrayString="First","Second","Third"  >>$(RESULTFILE)

	@$(ECHO) +++++ Test testInstance Passed.

### Test integer input and test with decimal, hex, octal and binary integers
testIntegerVariations:
##      Test variations on integer values
	@$(ECHO) 14. +++++ test Integer Variations
	@$(ECHO) 14. +++++ test Integer Variations >> $(RESULTFILE)

	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest \
	    scalBool=true \
	    scalUint8=12 \
	    scalUint16=100 \
	    scalUint32=9999 \
	    scalUint64=99999 \
	    arrayUint32=99,0,31000,63000,4294967295 >> $(RESULTFILE)

##      Display the created instance
	@cimcli gi -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	   Id=integerVariationTest >> $(RESULTFILE)

##      test values input
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest \
	    scalUint8=12 \
	    scalUint16=100 \
	    scalUint32=9999 \
	    scalUint64=99999 \
	    arrayUint32=99,0,31000,63000,4294967295 >> $(RESULTFILE)

##      test Hex
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest \
	    scalBool=true \
	    scalUint8=0xC \
	    scalUint16=0X64 \
	    scalUint32=0X270F \
	    scalUint64=0X1869F \
	    arrayUint32=0X63,0,0X7918,0XF618,0XFFFFFFFF >> $(RESULTFILE)

##      test Octal
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest \
	    scalBool=true \
	    scalUint8=014 \
	    scalUint16=0144 \
	    scalUint32=023417 \
	    scalUint64=0303237 \
	    arrayUint32=0143,0,074430,0173030,037777777777 >> $(RESULTFILE)

##      test Binary
	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest \
	    scalBool=true \
	    scalUint8=1100b \
	    scalUint16=1100100b \
	    scalUint32=10011100001111b >> $(RESULTFILE)

	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest >> $(RESULTFILE)

	@$(ECHO) +++++ Test integerVariations Passed.

### Test reference and reference name operations
referencetests:
	@$(ECHO) 15. +++++ referencetests
	@$(ECHO) 15. +++++ referencetests >> $(RESULTFILE)

	@cimcli r Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    -count 1 \
	    >> $(RESULTFILE)
	@cimcli rn Test_CLITestProviderClass -n "$(PROVIDERNS)" -count 1 \
	    --setRtnHostNames ChangedHostName \
	    >> $(RESULTFILE)

##      set the flag to use a fixed hostname
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    setProviderParameters \
	    substituteHostName=localhost >> $(RESULTFILE)

	@cimcli r -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    >> $(RESULTFILE)
	@cimcli r -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    -r parent  >> $(RESULTFILE)
	@cimcli rn -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    >> $(RESULTFILE)
###     This test confirms that specific input parameters are correctly passed
###     to the provider since the test provider returns this information
###     in a specific property.
	@cimcli r -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    -r child -rc Test_CLITestProviderLinkClass \
	    -pl requestInputParameters >> $(RESULTFILE)

##      Reset the flag to return a real hostname
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    resetProviderParameters >> $(RESULTFILE)
	@$(ECHO) +++++ Test referencetests Passed.

### Test operation of the association and association name operations
associationtests:
	@$(ECHO) 16. +++++ associationtests
	@$(ECHO) 16. +++++ associationtests >> $(RESULTFILE)

	@cimcli a Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    -count 1 \
	    >> $(RESULTFILE)
	@cimcli an Test_CLITestProviderClass -n "$(PROVIDERNS)" -count 1 \
	    --setRtnHostNames ChangedHostName \
	    >> $(RESULTFILE)

##      Set the flag to use a fixed hostname
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    setProviderParameters \
	    substituteHostName=localhost >> $(RESULTFILE)
	@cimcli a -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    -count 1 >> $(RESULTFILE)
	@cimcli an -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    >> $(RESULTFILE)
	@cimcli a -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    -pl "" >> $(RESULTFILE) 
###     This test returns property with the input parameters
	@cimcli a -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    -ac Test_CLITestProviderLinkClass -rc Test_CLITestProviderClass \
	    -r child -rr parent -pl requestInputParameters -count 1 \
	    >> $(RESULTFILE)
##      Reset the flag to use the real Host Name
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
		resetProviderParameters >> $(RESULTFILE)

	@$(ECHO) +++++ Test associationtests Passed.

### Test cimcli response to help option requests
helpoutputtests:
	@$(ECHO) 17. +++++ test Help Output
	@$(ECHO) 17. +++++ test Help Output >> $(HELPRESULTFILE)

	@cimcli --help >> $(HELPRESULTFILE)
	@cimcli -hc  >> $(HELPRESULTFILE)
	@cimcli -ho >> $(HELPRESULTFILE)
	@cimcli -h mi  >> $(HELPRESULTFILE)

	@$(ECHO) +++++ Test Help Output Passed.

### Test the parameter options for property definitions that
### produce null or default property values
nullvaluetests:
	@$(ECHO) 18. +++++ nullvaluetests
	@$(ECHO) 18. +++++ nullvaluetests >> $(RESULTFILE)

	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="NullValueTest" \
	    scalBool= defaultBool= \
	    scalUint32= defaultUint32= \
	    scalString= defaultString=  >> $(RESULTFILE)

	@cimcli gi -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="NullValueTest"  >> $(RESULTFILE)

	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="NullValueTest" \
	    scalBool= defaultBool= \
	    scalUint32= defaultUint32= \
	    scalString= defaultString=  >> $(RESULTFILE)

	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="NullValueTest"  >> $(RESULTFILE)

	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=EmptyStringTest \
	    scalUint32= \
	    scalUint64=99999 \
	    arrayUint32=99,0,31000,63000,4294967295 \
	    scalString= \
	    defaultString!  >> $(RESULTFILE)

	@cimcli gi -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="EmptyStringTest"  >> $(RESULTFILE)

	@cimcli ti -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=EmptyStringTest \
	    scalUint32= \
	    scalUint64=99999 \
	    arrayUint32=99,0,31000,63000,4294967295 \
	    scalString= \
	    defaultString!  >> $(RESULTFILE)

	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="EmptyStringTest"  >> $(RESULTFILE)

	@$(ECHO) +++++ Test nullvaluetests Passed.

### Test the ns operation.  Note that this test is subject to knowledge
### of the number of namespaces and their names in the target system
### We assume that this is generally a constant for the pegasus nightly
### test environment.
namespacetests:
	@$(ECHO) 19. +++++ namespacetests
	@$(ECHO) 19. +++++ namespacetests >> $(NSRESULTFILE)

	@cimcli ns >> $(NSRESULTFILE)
	@cimcli ns --sum >> $(NSRESULTFILE)
##      Output results but do not honor error return code.
##      Should always return an error.
	-@cimcli ns --sum -count 25 >> $(NSRESULTFILE) 2>&1
	-@cimcli ns --sum -count 15 >> $(NSRESULTFILE) 2>&1

	@$(ECHO) +++++ Test namespacetests Passed.

### Test the use of the -count option which compares number of objects 
### returned against command line parameter. 
objectcountertests:
	@$(ECHO) 20. +++++ objectCounterTests
	@$(ECHO) 20. +++++ objectCounterTests >> $(RESULTFILE)

##      Set the flag to use a fixed hostname
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
	    setProviderParameters \
	    substituteHostName=localhost >> $(RESULTFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    --sum -count 1 >>$(RESULTFILE)
	@cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    --sum -count 1 >>$(RESULTFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    -count 1 >>$(RESULTFILE)
	@cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    -count 1 >>$(RESULTFILE)

##      The following tests all return errors (count wrong) and the 
##      Failed message. Ignore the errors and only confirm the message in the
##      result file.
	-@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    --sum -count 10 >>$(RESULTFILE) 2>&1
	-@cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    --sum -count 10 >>$(RESULTFILE) 2>&1
	-@cimcli a -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    --sum -count 10 >>$(RESULTFILE) 2>&1
	-@cimcli r -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    --sum -count 10 >>$(RESULTFILE) 2>&1
	-@cimcli an -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    --sum -count 10 >>$(RESULTFILE) 2>&1
	-@cimcli rn -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Mike\" \
	    --sum -count 10 >>$(RESULTFILE) 2>&1
	@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
		resetProviderParameters >> $(RESULTFILE)

	@$(ECHO) +++++ Test objectcountertests Passed.

##
##	Confirm the sort of instances and instance Paths responses
##
SortInstancesTest:
	@$(ECHO) 21. +++++ SortInstancesTest
	@$(ECHO) 21. +++++ SortInstancesTest >> $(RESULTFILE)
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Zed Name=Futz
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Norman Name=Susan
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Thomas Name=Peter
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Fred Name=Jones
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Judy Name=blah
	@cimcli ci -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=Andy Name=Blue
	@cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass --sort \
		>>$(RESULTFILE)
	@cimcli ei -n "$(PROVIDERNS)" Test_CLITestProviderClass --sort \
	        -pl Id,Name \
		>>$(RESULTFILE)

	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Zed\"
	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Norman\"
	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Thomas\"
	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Fred\"
	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Judy\"
	@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Andy\"

	@$(ECHO) +++++ Test SortInstancesTest Passed.

## Test Enumerate Classes and Enumerate ClassNames
enumerateClasses:
	@$(ECHO) 22. +++++ Test For enumerateClasses
	@$(ECHO) 22. +++++ Test For enumerateClasses  >> $(RESULTFILE)

##      test enumerate classes with our known class to assure
##      defined output. These calls all use --sort because client cannot depend
##      on ordering of enuemerate instance returns from server
	@cimcli ec -n "$(PROVIDERNS)"  Test_CLITestProviderClass --sort\
	    >> $(RESULTFILE)
	@cimcli ec -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass  --sort\
	    >> $(RESULTFILE)
	@cimcli ec -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass -niq \
	    --sort >> $(RESULTFILE)
##      output as xml and repeat with the -ic option set
	@cimcli ec -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass -o xml \
	    --sort >> $(RESULTFILE)
	@cimcli ec -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass -o xml \
	    --sort -ic >> $(RESULTFILE)

	@cimcli nc -n "$(STATICNS)"  --sort >> $(RESULTFILE)

	@$(ECHO) 22. +++++ Test enumerateClasses

## Test executeQuery Operation. This test should 
## return a single instance.  We just test the response count to avoid
## the issue of variable information in the result file
testExecQuery:
	@$(ECHO) 23. +++++ Test For execQuery
	@$(ECHO) 23. +++++ Test For execQuery  >> $(RESULTFILE)
ifdef PEGASUS_ENABLE_EXECQUERY
	@cimcli xq "Select scaleBool from Test_CLITestProviderClass" \
	  -n "$(PROVIDERNS)" -count 1
endif

	@$(ECHO) 23. +++++ Test enumerateClasses

### Execute the set of test targets defined below
poststarttests:
##      Remove existing master files
	@$(RM) $(RESULTFILE)
	@$(RM) $(TABLEFORMATFILE)
	@$(RM) $(HELPRESULTFILE)
	@$(RM) $(NSRESULTFILE)

##      Execute the list of tests
	$(MAKE) getclass
	$(MAKE) testQualifierDecls
	$(MAKE) testClassOperations
	$(MAKE) testRefs
	$(MAKE) testPropertyOperations
	$(MAKE) testCreateInstanceOperation
	$(MAKE) testModifyInstanceOperation
	$(MAKE) testDeleteInstanceOperation
	$(MAKE) testInvokeMethod
	$(MAKE) testTableOutputFormat
	$(MAKE) testPropertyListOption
	$(MAKE) testGetInstanceOperation
	$(MAKE) testTestInstanceOperation
	$(MAKE) testIntegerVariations
	$(MAKE) referencetests
	$(MAKE) associationtests
	$(MAKE) helpoutputtests
	$(MAKE) nullvaluetests
####    See Bug 8690
####    $(MAKE) namespacetests
	$(MAKE) objectcountertests
	$(MAKE) SortInstancesTest
	$(MAKE) enumerateClasses
	$(MAKE) testExecQuery

	@$(ECHO) +++++ Compare Result files
##      diffsort used because there is at least one instance where
##      the redirection of std and err outputs changes order.
####    @$(DIFF) $(MASTERRESULTFILE) $(RESULTFILE)
	@ $(call DIFFSORT,$(RESULTFILE),$(MASTERRESULTFILE))
	@$(DIFF) $(MASTERTABLEFORMATFILE) $(TABLEFORMATFILE)
	@$(DIFF) $(MASTERHELPRESULTFILE) $(HELPRESULTFILE)

##      Following comment out because we cannot depend on the
##      total number of namespaces. See bug 8690
####    -@$(DIFF) $(MASTERNSRESULTFILE) $(NSRESULTFILE)

	@$(ECHO) +++++ Remove Result files since test successful
	-@$(RM) $(RESULTFILE)
	-@$(RM) $(TABLEFORMATFILE)
	-@$(RM) $(HELPRESULTFILE)
	-@$(RM) $(NSRESULTFILE)

	@$(ECHO) +++++ All cimcli tests Passed.

### Clean up an created/modified/or deleted instances that may exist
### in the server (i.e. the provider) and remove the result files.
### Note that the normal path also removes result files so that the
### only reason for this is to redo a failed test before the provider is
### reinitialized (server restarted or provider reloaded)
clean:

##      Remove any existing instances
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Zed\"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Norman\"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Thomas\"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Fred\"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Judy\"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass.Id=\"Andy\"

	-@cimcli di -n "$(PROVIDERNS)" \
		Test_CLITestProviderClass.Id=\"ScalarTest\"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=arrayTest
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass Id=stringTest
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id=integerVariationTest
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="NullValueTest"
	-@cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass \
	    Id="EmptyStringTest"
	-@cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
		resetProviderParameters

##      Remove the result files
	-@$(RM) $(RESULTFILE)
	-@$(RM) $(TABLEFORMATFILE)
	-@$(RM) $(HELPRESULTFILE)
	-@$(RM) $(NSRESULTFILE)

##################### Local targets useful for manual testing ############

## enumerate instances of the test class
ni:
	cimcli ni -n "$(PROVIDERNS)" Test_CLITestProviderClass

## Interactive get of instances of the test class
giall:
	cimcli gi -n "$(PROVIDERNS)" Test_CLITestProviderClass

## Interactive delete of the test class
di:
	cimcli di -n "$(PROVIDERNS)" Test_CLITestProviderClass

gi:
	cimcli gi -n "$(PROVIDERNS)" Test_CLITestProviderClass -v

## Displays classes used in this test
gc:
	cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderClass
	cimcli gc -n "$(PROVIDERNS)"  Test_CLITestProviderLinkClass

im:
	cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
		setProviderParameters \
		substituteHostName=tempHostName

rm:
	cimcli im Test_CLITestProviderClass -n "$(PROVIDERNS)" \
		resetProviderParameters

a:
	cimcli r Test_CLITestProviderClass.Id=\"Mike\" -n "$(PROVIDERNS)" \
	 --setRtnHostNames "blah" -v

# END_OF_FILE

