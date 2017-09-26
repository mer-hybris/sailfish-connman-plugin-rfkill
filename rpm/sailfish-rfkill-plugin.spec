Name: sailfish-rfkill-plugin
Version: 1.0.0
Release: 1
Summary: Sailfish Connman rfkill plugin
Group: Development/Libraries
License: GPLv2
URL: https://github.com/mer-hybris/sailfish-connman-plugin-rfkill
Source: %{name}-%{version}.tar.bz2
Requires: bluez
Requires: bluez-libs
Requires: connman >= 1.31+git44
BuildRequires: bluez-libs-devel
BuildRequires: connman-devel >= 1.31+git44
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
This package contains the Sailfish Connman rfkill plugin library.

%prep
%setup -q -n %{name}-%{version}

%build
make %{?jobs:-j%jobs} release

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/%{_libdir}/connman/plugins
%preun

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/connman/plugins/sailfish-rfkill-plugin.so
