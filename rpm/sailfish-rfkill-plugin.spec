Name: sailfish-rfkill-plugin
Version: 1.0.1
Release: 1
Summary: Sailfish Connman rfkill plugin
License: GPLv2
URL: https://github.com/mer-hybris/sailfish-connman-plugin-rfkill
Source: %{name}-%{version}.tar.bz2
Requires: bluez5
Requires: bluez5-libs
Requires: connman >= 1.37
BuildRequires: bluez5-libs-devel
BuildRequires: pkgconfig(connman) >= 1.37
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%define connmanplugindir %(pkg-config --variable=plugindir connman)

%description
This package contains the Sailfish Connman rfkill plugin library.

%prep
%setup -q -n %{name}-%{version}

%build
make %{?jobs:-j%jobs} release

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} LIBDIR=%{_libdir} install

mkdir -p %{buildroot}/%{connmanplugindir}

%preun

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{connmanplugindir}/sailfish-rfkill-plugin.so
