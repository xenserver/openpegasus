include $(B_BASE)/common.mk
include $(B_BASE)/rpmbuild.mk

REPONAME := openpegasus
REPO := $(call git_loc,$(REPONAME))

PEGASUS_VERSION := 2.10.0
XS_PATCH_VERSION := xs1
PEGASUS_ROOT := $(REPO)/pegasus
PEGASUS_HOME := /opt/openpegasus
CMPI_DIR := $(PEGASUS_ROOT)/src/Pegasus/Provider/CMPI

PEGASUS_RPM := $(MY_OUTPUT_DIR)/RPMS/x86_64/openpegasus-$(PEGASUS_VERSION)-$(XS_PATCH_VERSION).x86_64.rpm
PEGASUS_RPM_LINK := $(MY_OUTPUT_DIR)/openpegasus.x86_64.rpm

PEGASUS_SPEC := $(MY_OBJ_DIR)/openpegasus.spec
PEGASUS_RPM_TMP_DIR := $(MY_OBJ_DIR)/RPM_BUILD_DIRECTORY/tmp/openpegasus

CIM_SCHEMA_ZIP := $(CARBON_DISTFILES)/cim/cim_schema_2.23.0Experimental-MOFs.zip
CIM_SCHEMA_DIR := $(PEGASUS_HOME)/share/Schemas/CIM223
PG_SCHEMA_DIR := $(PEGASUS_HOME)/share/Schemas/Pegasus

LIBXENSERVER_BIN := $(PROJECT_OUTPUTDIR)/xen-api-sdk/libxenserver-bin.tar.bz2
LIBXENSERVER_SRC := $(PROJECT_OUTPUTDIR)/xen-api-sdk/libxenserver-src.tar.bz2

LIBXENSERVER_DIR := $(MY_OBJ_DIR)/libxenserver
LIBXENSERVER_H := $(LIBXENSERVER_DIR)/include/xen/api/xen_all.h
LIBXENSERVER_SO := $(LIBXENSERVER_DIR)/libxenserver.so

OUTPUT := $(PEGASUS_RPM) \
	  $(PEGASUS_RPM_LINK) \
	  $(MY_OUTPUT_DIR)/cmpi-headers.tar.bz2 \
	  $(MY_SOURCES)/openpegasus-src.tar.bz2 \
	  $(MY_SOURCES)/MANIFEST

.PHONY: build
build: $(OUTPUT)
	@:

$(PEGASUS_RPM_LINK): $(PEGASUS_RPM)
	ln -s $(patsubst $(@D)/%,%,$^) $@

$(MY_OUTPUT_DIR)/cmpi-headers.tar.bz2:
	tar -C $(CMPI_DIR)/.. -cjf $@ $(notdir $(CMPI_DIR))

$(PEGASUS_RPM): $(PEGASUS_SPEC) pegasus $(RPM_DIRECTORIES)
	mkdir -p $(@D)
	$(call mkdir_clean, $(PEGASUS_RPM_TMP_DIR)$(CIM_SCHEMA_DIR))
	rsync -av --delete $(PEGASUS_HOME)/bin $(PEGASUS_HOME)/lib \
		$(PEGASUS_RPM_TMP_DIR)/$(PEGASUS_HOME)
	unzip -d $(PEGASUS_RPM_TMP_DIR)$(CIM_SCHEMA_DIR) $(CIM_SCHEMA_ZIP)
	rsync -av --delete $(REPO)/pegasus/Schemas/Pegasus/ \
		$(PEGASUS_RPM_TMP_DIR)/$(PG_SCHEMA_DIR)
	rsync -av --exclude \*~ $(REPO)/overlay/ $(PEGASUS_RPM_TMP_DIR)/
	mkdir -p $(PEGASUS_RPM_TMP_DIR)/opt/openpegasus/providers
	mkdir -p $(PEGASUS_RPM_TMP_DIR)/opt/openpegasus/trace
	cd $(PEGASUS_RPM_TMP_DIR) && tar zcvf $(RPM_SOURCESDIR)/openpegasus.tar.gz *
	$(RPMBUILD) -bb $<

$(PEGASUS_SPEC): $(REPO)/openpegasus.spec.in pegasus
	mkdir -p $(@D)
	sed -e 's/@PEGASUS_VERSION@/$(PEGASUS_VERSION)/g' \
	    -e 's/@XS_PATCH_VERSION@/$(XS_PATCH_VERSION)/g' \
	    $< >$@

.PHONY: pegasus
pegasus: $(LIBXENSERVER_SO) $(LIBXENSERVER_H)
	$(call mkdir_clean, $(PEGASUS_HOME))
	rm -f $(REPO)/pegasus/src/xen
	ln -s $(LIBXENSERVER_DIR)/include/xen $(REPO)/pegasus/src/xen
	PEGASUS_ROOT=$(PEGASUS_ROOT) \
	PEGASUS_HOME=$(PEGASUS_HOME) \
	PEGASUS_PLATFORM=LINUX_X86_64_GNU \
	PEGASUS_HAS_SSL=yes \
	PEGASUS_ENABLE_SSLV2=yes \
	PEGASUS_ENABLE_PROTOCOL_WSMAN=true \
	PEGASUS_ENABLE_CQL=true \
	PEGASUS_ENABLE_EXECQUERY=yes \
	PEGASUS_REPOSITORY_MODE=BIN \
	PEGASUS_ENABLE_COMPRESSED_REPOSITORY=yes \
	PEGASUS_PAM_AUTHENTICATION=yes \
	PEGASUS_ALWAYS_USE_PAM=yes \
	PEGASUS_USE_PAM_STANDALONE_PROC=yes \
	PEGASUS_ENABLE_PRIVILEGE_SEPARATION=false \
	$(MAKE) -C $(PEGASUS_ROOT)
#	PEGASUS_ENABLE_DEBUG=false \
#	PEGASUS_DISABLE_DEPRECATED_INTERFACES=yes \
#	PEGASUS_DISABLE_PROV_USERCTXT=yes \
#	PEGASUS_USE_RELEASE_CONFIG_OPTIONS=yes \
#	PEGASUS_ENABLE_USERGROUP_AUTHORIZATION=false \
#	PEGASUS_DISABLE_PERFINST=no \

$(LIBXENSERVER_SO): $(LIBXENSERVER_BIN)
	tar -C $(MY_OBJ_DIR) -xjf $^

$(LIBXENSERVER_H): $(LIBXENSERVER_SRC)
	tar -C $(MY_OBJ_DIR) -xjf $^

$(MY_SOURCES)/MANIFEST:
	mkdir -p $(@D)
	echo "openpegasus mit local openpegasus-src.tar.bz2" >$@

$(MY_SOURCES)/openpegasus-src.tar.bz2:
	mkdir -p $(@D)
	tar -C $(PEGASUS_ROOT)/.. -cjf $@ $(notdir $(PEGASUS_ROOT))

clean:
	rm -rf $(MY_OBJ_DIR)/*
