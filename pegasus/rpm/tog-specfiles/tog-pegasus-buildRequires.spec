# Start of section pegasus/rpm/tog-specfiles/tog-pegasus-buildRequires.spec
#
BuildRequires:      bash, sed, grep, coreutils, procps, gcc, gcc-c++
BuildRequires:      libstdc++, make, pam-devel
BuildRequires:      openssl-devel >= 0.9.6, e2fsprogs
%if %{JMPI_PROVIDER_REQUESTED}
BuildRequires:      gcc-java, libgcj-devel, libgcj, java-1.4.2-gcj-compat
Requires:           libgcj, java-1.4.2-gcj-compat
%endif
