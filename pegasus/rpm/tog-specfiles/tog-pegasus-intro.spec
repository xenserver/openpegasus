# Start of section pegasus/rpm/tog-specfiles/tog-pegasus-intro.spec
#
%{?!PEGASUS_BUILD_TEST_RPM:   %define PEGASUS_BUILD_TEST_RPM        0}
# do "rpmbuild --define 'PEGASUS_BUILD_TEST_RPM 1'" to build test RPM.
#
%{?!AUTOSTART:   %define AUTOSTART        0}
# Use "rpm -[iU]vh --define 'AUTOSTART 1'" in order to have cimserver enabled
# (chkconfig --level=345 tog-pegasus on) after installation.
#
# Use "rpmbuild --define 'JMPI_PROVIDER_REQUESTED 1'" to include JMPI support.
%{?!JMPI_PROVIDER_REQUESTED: %define JMPI_PROVIDER_REQUESTED 0}

Summary:   OpenPegasus WBEM Services for Linux
Name:      %{Flavor}-pegasus
Group:     Systems Management/Base
License:   Open Group Pegasus Open Source
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
URL:       http://www.openpegasus.org

Source:    %{name}-%{version}-%{packageVersion}.tar.gz
#
# End of section pegasus/rpm/tog-specfiles/tog-pegasus-intro.spec
