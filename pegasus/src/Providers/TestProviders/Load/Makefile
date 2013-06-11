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
# Makefile builds a TestProvider Namespace
# Note: For best results, ensure that the test/TestProvider namespace does not
#       exist before calling this Makefile.

ROOT = ../../../..
include $(ROOT)/mak/config.mak
include $(ROOT)/mak/configschema.mak

# Loads the test MOF into a separate namespace.
# The following variable determines the Namespace into which the
# TestProvider classes are to be loaded.

TESTPROVIDERNS=test/TestProvider

ifdef PEGASUS_ENABLE_REMOTE_CMPI
CREATE_REMOTE_NAMESPACE = CreateExtNS2
REMOTE_TESTPROVIDERNS = test/Remote/Localhost
endif

# Create a single variable to control schema compile
ifeq ($(PEGASUS_ENABLE_SLP),true)
	PEGASUS_BUILD_INTEROP=TRUE
endif
ifndef PEGASUS_DISABLE_PERFINST
	PEGASUS_BUILD_INTEROP=TRUE
endif

# The following variable determines the version of the Schema to
# be loaded into the TestProvider namespace for EmbeddedInstance tests.
EI_SCHEMA_DIR = $(ROOT)/Schemas/CIM2131
EI_DYNAMIC_TEST_NAMESPACE = test/EmbeddedInstance/Dynamic
EI_STATIC_TEST_NAMESPACE = test/EmbeddedInstance/Static

##
## Load the test MOF with cimmofl.
##
repository:
	@ $(ECHO) +++++ Loading Core$(CIM_SCHEMA_VER)_Qualifiers.mof into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" -n$(TESTPROVIDERNS) $(CIM_SCHEMA_DIR)/Core$(CIM_SCHEMA_VER)_Qualifiers.mof
	@ $(ECHO) +++++ Loading CIM_Core$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Core$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_Event$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Event$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_Physical$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Physical$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_System$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_System$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_Interop$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Interop$(CIM_SCHEMA_VER).mof

	@ $(ECHO) +++++ Loading PG_Events$(PG_INTEROP_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(PG_INTEROP_MOF_PATH)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(PG_INTEROP_MOF_PATH)/PG_Events$(PG_INTEROP_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading TestInstanceProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TestInstanceProviderSchema.mof
	@ $(ECHO) +++++ Loading TestChunkingStressProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TestChunkingStressProviderSchema.mof
	@ $(ECHO) +++++ Loading IndicationStressTest class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" IndicationStressTestProviderSchema.mof

	@ $(ECHO) +++++ Registering TestInstanceProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestInstanceProviderRegistration.mof
	@ $(ECHO) +++++ Registering TestChunkingStressProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestChunkingStressProviderRegistration.mof
	@ $(ECHO) +++++ Registering IndicationStressTest Provider  ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" IndicationStressTestProviderR.mof
	@ $(ECHO) +++++ Registering IndicationStressTestConsumer Provider  ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" IndicationStressTestConsumerR.mof

	@ $(ECHO) +++++ Loading IndicationTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" IndicationTestProvider.mof
	@ $(ECHO) +++++ Registering IndicationTestProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" IndicationTestProviderR.mof

	@ $(ECHO) +++++ Loading MethodTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" MethodTestProvider.mof
	@ $(ECHO) +++++ Registering MethodTestProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" MethodTestProviderR.mof

	@ $(ECHO) +++++ Loading CLITestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" CLITestProvider.mof
	@ $(ECHO) +++++ Registering CLITestProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" CLITestProviderR.mof
	@ $(ECHO) +++++ Installing the Family MOF file for the association provider
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TST_Family.mof
	@ $(ECHO) +++++ Registering FamilyProvider schema Providers  ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" FamilyProviderSchemaR.mof

	@ $(ECHO) +++++ Loading CIMOMHandleTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" CIMOMHandleTestProviderSchema.mof
	@ $(ECHO) +++++ Registering CIMOMHandleTestProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" CIMOMHandleTestProviderRegistration.mof

	@ $(ECHO) +++++ Loading TestAggregationOutputProvider.mof into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TestAggregationOutputProvider.mof
	@ $(ECHO) +++++ Registering TestAggregationOutputProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestAggregationOutputProviderR.mof

	@ $(ECHO) +++++ Loading LocalizedProvider.mof into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" LocalizedProvider.mof
	@ $(ECHO) +++++ Registering LocalizedProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" LocalizedProviderR.mof

ifeq ($(PEGASUS_ENABLE_CMPI_PROVIDER_MANAGER), true)
	@ $(ECHO) +++++ Loading TestCMPI_ProviderSchema class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TestCMPI_ProviderSchema.mof

	@ $(ECHO) +++++ Registering TestCMPI_ProviderRegistration ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestCMPI_ProviderRegistration.mof

	@ $(ECHO) +++++ Loading TestCMPI_Association class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TestCMPI_AssociationProvider.mof
	@ $(ECHO) +++++ Registering TestCMPI_AssociationProviderRegistration ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestCMPI_AssociationProviderR.mof

	@ $(ECHO) +++++ Loading CMPIPerf_TestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" CMPIPerf_TestProviderSchema.mof
	@ $(ECHO) +++++ Registering CMPIPerf_TestProviderRegistration ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" CMPIPerf_TestProviderRegistration.mof
	
ifdef PEGASUS_ENABLE_EXECQUERY
	@ $(ECHO) +++++ Registering TestCMPI_ProviderExecQueryRegistration ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestCMPI_ProviderExecQueryRegistration.mof
endif

endif

ifndef PEGASUS_DISABLE_PROV_USERCTXT
	@ $(ECHO) +++++ Loading UserContextTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" UserContextTestProvider.mof

	@ $(ECHO) +++++ Registering UserContextTestProvider ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" UserContextRequestorR.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" UserContextPrivilegedR.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" UserContextDesignatedR.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" UserContextCIMServerR.mof
endif
	@ $(ECHO) +++++ Loading OOPModuleFailureTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" FailureTestIndication.mof

ifdef PEGASUS_BUILD_INTEROP 
	@ $(ECHO) +++++ Loading TestServerProfile class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(TESTPROVIDERNS)" TestServerProfileSchema.mof
	@ $(ECHO) +++++ Registering TestServerProfile Providers ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" TestServerProfileR.mof
endif

	@ $(ECHO) +++++ EmbeddedInstanceTest provider class definitions into $(EI_DYNAMIC_TEST_NAMESPACE) and $(EI_STATIC_TEST_NAMESPACE) namespace ...
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" -n$(EI_DYNAMIC_TEST_NAMESPACE) $(EI_SCHEMA_DIR)/Core_Qualifiers.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" CIMError.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Core.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Event.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Interop.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" EmbeddedInstanceProviderSchema.mof

	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" -n$(EI_STATIC_TEST_NAMESPACE) $(EI_SCHEMA_DIR)/Core_Qualifiers.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" CIMError.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Core.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Event.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Interop.mof
	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" EmbeddedInstanceProviderSchema.mof

	@ cimmofl "-R$(REPOSITORY_DIR)" "-N$(REPOSITORY_NAME)" "-M$(REPOSITORY_MODE)" "-n$(INTEROPNS)" EmbeddedInstanceProviderR.mof
# create remote namespace for Remote CMPI test providers
ifdef PEGASUS_ENABLE_REMOTE_CMPI
	@ echo +++++ Creating remote namespace ...
	@ $(CREATE_REMOTE_NAMESPACE) -r $(REPOSITORY_DIR)/$(REPOSITORY_NAME) -n $(REMOTE_TESTPROVIDERNS) -p $(TESTPROVIDERNS) -l localhost
endif

##
## Load the test MOF with cimmof.
##
repositoryServer:
	@ $(ECHO) +++++ Loading Core$(CIM_SCHEMA_VER)_Qualifiers.mof into $(TESTPROVIDERNS) namespace ...
	@ cimmof -n$(TESTPROVIDERNS) $(CIM_SCHEMA_DIR)/Core$(CIM_SCHEMA_VER)_Qualifiers.mof
	@ $(ECHO) +++++ Loading CIM_Core$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Core$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_Event$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Event$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_Physical$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Physical$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_System$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_System$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading CIM_Interop$(CIM_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-I$(CIM_SCHEMA_DIR)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(CIM_SCHEMA_DIR)/CIM_Interop$(CIM_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading PG_Events$(PG_INTEROP_SCHEMA_VER) into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-I$(PG_INTEROP_MOF_PATH)" "-n$(TESTPROVIDERNS)" $(ALLOW_EXPERIMENTAL) $(PG_INTEROP_MOF_PATH)/PG_Events$(PG_INTEROP_SCHEMA_VER).mof
	@ $(ECHO) +++++ Loading IndicationStressTest class definition into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" IndicationStressTestProviderSchema.mof
	@ $(ECHO) +++++ Registering IndicationStressTest Provider  ...
	@ cimmof "-n$(INTEROPNS)" IndicationStressTestProviderR.mof
	@ $(ECHO) +++++ Registering IndicationStressTestConsumer Provider  ...
	@ cimmof "-n$(INTEROPNS)" IndicationStressTestConsumerR.mof

	@ $(ECHO) +++++ Loading IndicationTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" IndicationTestProvider.mof
	@ $(ECHO) +++++ Registering IndicationTestProvider ...
	@ cimmof "-n$(INTEROPNS)" IndicationTestProviderR.mof

	@ $(ECHO) +++++ Loading MethodTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" MethodTestProvider.mof
	@ $(ECHO) +++++ Registering MethodTestProvider ...
	@ cimmof "-n$(INTEROPNS)" MethodTestProviderR.mof

	@ $(ECHO) +++++ Loading CLITestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" CLITestProvider.mof
	@ $(ECHO) +++++ Registering CLITestProvider ...
	@ cimmof "-n$(INTEROPNS)" CLITestProviderR.mof
	@ $(ECHO) +++++ Installing the Family MOF file for the association provider
	@ cimmof "-n$(TESTPROVIDERNS)" TST_Family.mof
	@ $(ECHO) +++++ Registering FamilyProvider schema Providers  ...
	@ cimmof "-n$(INTEROPNS)" FamilyProviderSchemaR.mof

	@ $(ECHO) +++++ Loading TestAggregationOutputProvider.mof into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" TestAggregationOutputProvider.mof
	@ $(ECHO) +++++ Registering TestAggregationOutputProvider ...
	@ cimmof "-n$(INTEROPNS)" TestAggregationOutputProviderR.mof

	@ $(ECHO) +++++ Loading LocalizedProvider.mof into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" LocalizedProvider.mof
	@ $(ECHO) +++++ Registering LocalizedProvider ...
	@ cimmof "-n$(INTEROPNS)" LocalizedProviderR.mof

ifeq ($(PEGASUS_ENABLE_CMPI_PROVIDER_MANAGER), true)
	@ $(ECHO) +++++ Loading TestCMPI_ProviderSchema class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof   "-n$(TESTPROVIDERNS)" TestCMPI_ProviderSchema.mof
	@ $(ECHO) +++++ Registering TestCMPI_ProviderRegistration ...
	@ cimmof   "-n$(INTEROPNS)" TestCMPI_ProviderRegistration.mof
	@ $(ECHO) +++++ Loading CMPIPerf_TestProviderSchema class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof   "-n$(TESTPROVIDERNS)" CMPIPerf_TestProviderSchema.mof
	@ $(ECHO) +++++ Registering CMPIPerf_TestProviderRegistration ...
	@ cimmof   "-n$(INTEROPNS)" CMPIPerf_TestProviderRegistration.mof	
ifdef PEGASUS_ENABLE_EXECQUERY
	@ $(ECHO) +++++ Registering TestCMPI_ProviderExecQueryRegistration ...
	@ cimmof   "-n$(INTEROPNS)" TestCMPI_ProviderExecQueryRegistration.mof
endif

endif

ifndef PEGASUS_DISABLE_PROV_USERCTXT
	@ $(ECHO) +++++ Loading UserContextTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" UserContextTestProvider.mof

	@ $(ECHO) +++++ Registering UserContextTestProvider ...
	@ cimmof "-n$(INTEROPNS)" UserContextRequestorR.mof
	@ cimmof "-n$(INTEROPNS)" UserContextPrivilegedR.mof
	@ cimmof "-n$(INTEROPNS)" UserContextDesignatedR.mof
	@ cimmof "-n$(INTEROPNS)" UserContextCIMServerR.mof
endif
	@ $(ECHO) +++++ Loading OOPModuleFailureTestProvider class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" FailureTestIndication.mof

ifdef PEGASUS_BUILD_INTEROP 
	@ $(ECHO) +++++ Loading TestServerProfile class definitions into $(TESTPROVIDERNS) namespace ...
	@ cimmof "-n$(TESTPROVIDERNS)" TestServerProfileSchema.mof
	@ $(ECHO) +++++ Registering TestServerProfile Providers ...
	@ cimmof "-n$(INTEROPNS)" TestServerProfileR.mof
endif

	@ $(ECHO) +++++ EmbeddedInstanceTest provider class definitions into $(EI_DYNAMIC_TEST_NAMESPACE) and $(EI_STATIC_TEST_NAMESPACE) namespace ...
	@ cimmof -n$(EI_DYNAMIC_TEST_NAMESPACE) $(EI_SCHEMA_DIR)/Core_Qualifiers.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" CIMError.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Core.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Event.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Interop.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_DYNAMIC_TEST_NAMESPACE)" "-aE" EmbeddedInstanceProviderSchema.mof

	@ cimmof "-n$(EI_STATIC_TEST_NAMESPACE)" $(EI_SCHEMA_DIR)/Core_Qualifiers.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" CIMError.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Core.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Event.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" $(EI_SCHEMA_DIR)/CIM_Interop.mof
	@ cimmof "-I$(EI_SCHEMA_DIR)" "-n$(EI_STATIC_TEST_NAMESPACE)" "-aE" EmbeddedInstanceProviderSchema.mof

	@ cimmof "-n$(INTEROPNS)" EmbeddedInstanceProviderR.mof
# create remote namespace for Remote CMPI test providers
ifdef PEGASUS_ENABLE_REMOTE_CMPI
	@ echo +++++ Creating remote namespace ...
	@ $(CREATE_REMOTE_NAMESPACE) -r $(REPOSITORY_DIR)/$(REPOSITORY_NAME) -n $(REMOTE_TESTPROVIDERNS) -p $(TESTPROVIDERNS) -l localhost
endif


unregister:
	$(MAKE) -i unregisterproviders


unregisterproviders:
	cimprovider -r -m IndicationStressTestProviderModule
	cimprovider -r -m IndicationTestProviderModule
	cimprovider -r -m MethodTestProviderModule
	cimprovider -r -m CLITestProviderModule
	cimprovider -r -m TestLocalizedProviderModule
ifeq ($(PEGASUS_ENABLE_CMPI_PROVIDER_MANAGER), true)
	cimprovider -r -m TestCMPIIndicationProviderModule
	cimprovider -r -m TestCMPIMethodProviderModule
	cimprovider -r -m TestCMPIFail_1ProviderModule
	cimprovider -r -m TestCMPIFail_2ProviderModule
	cimprovider -r -m TestCMPIFail_3ProviderModule 
	cimprovider -r -m TestCMPIFail_4ProviderModule 
	cimprovider -r -m cmpiPerf_TestClassProviderModule
	cimprovider -r -m cmpiPerf_TestClassAProviderModule
	cimprovider -r -m cmpiPerf_TestAssocClassProviderModule
	
ifdef PEGASUS_ENABLE_EXECQUERY
	cimprovider -r -m TestCMPIInstanceProviderModule
endif
endif 
ifndef PEGASUS_DISABLE_PROV_USERCTXT
	cimprovider -r -m UserContextRequestorModule
	cimprovider -r -m UserContextPrivilegedModule
	cimprovider -r -m UserContextDesignatedModule
	cimprovider -r -m UserContextCIMServerModule
endif

	cimprovider -r -m EmbeddedInstanceProviderModule

depend:

sub:

misc:

tests:

poststarttests:

general:

clean:

