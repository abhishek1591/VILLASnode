Name:     villas-node
Version:  §VERSION§
Vendor:   Institute for Automation of Complex Power Systems
Packager: Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
Release:  §RELEASE§%{?dist}
Summary:  This is VILLASnode, a gateway for processing and forwardning simulation data between real-time simulators.

License:  GPLv3
URL:      https://git.rwth-aachen.de/VILLASframework/VILLASnode
Source0:  villas-node-§VERSION§-§RELEASE§.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: gcc pkgconfig make

Requires:      iproute kernel-modules-extra module-init-tools

BuildRequires: openssl-devel libconfig-devel libnl3-devel libcurl-devel jansson-devel libwebsockets-devel zeromq-devel nanomsg-devel libiec61850-devel librabbitmq-devel mosquitto-devel libbson
Requires:      openssl       libconfig       libnl3       libcurl       jansson       libwebsockets       zeromq       nanomsg       libiec61850       librabbitmq       mosquitto       libbson-devel

%description

%package doc

Summary:        HTML documentation for users and developers.
Group:          Documentation

%package devel

Summary:        Headers and libraries for building apps that use libvillas.
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description devel

The development headers for libvillas.

%description doc

%prep
%setup -q

%build
make DEBUG=1 PREFIX=/usr

%install
rm -rf %{?buildroot}
make DEBUG=1 PREFIX=/usr DESTDIR=%{?buildroot} install
make DEBUG=1 PREFIX=/usr DESTDIR=%{?buildroot} install-doc

%check
make DEBUG=1 run-unit-tests
make DEBUG=1 run-integration-tests

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%clean
rm -rf %{?buildroot}

%files
/usr/bin/villas
/usr/bin/villas-*
/usr/bin/conf2json
/usr/bin/zmq-keygen

/usr/lib/libvillas.so
/usr/lib/libvillas.so.*

/usr/lib/libvillas-ext.so
/usr/lib/libvillas-ext.so.*

/usr/share/villas/node/web/
/usr/share/villas/node/plugins/

%config /etc/villas/node/*.conf
%license COPYING.md

%files doc
%docdir /usr/share/villas/node/doc/
/usr/share/villas/node/doc/

%files devel
/usr/include/villas/

%changelog
* Fri Mar 17 2017 Steffen Vogel <stvogel@eonerc.rwth-aachen.de
- Initial RPM release
