#
#    zm-asset - Asset management component
#
#    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
#    of zmon.it, the fast and scalable monitoring system.                           
#                                                                                   
#    This Source Code Form is subject to the terms of the Mozilla Public License, v.
#    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
#    one at http://mozilla.org/MPL/2.0/.                                            
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
Name:           zm-asset
Version:        1.0.0
Release:        1
Summary:        asset management component
License:        MIT
URL:            http://example.com/
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRequires:  malamute-devel
BuildRequires:  zm-proto-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
zm-asset asset management component.


%prep
%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%{_bindir}/zmasset
%config(noreplace) %{_sysconfdir}/zm-asset/zmasset.cfg
/usr/lib/systemd/system/zmasset.service
%dir %{_sysconfdir}/zm-asset
%if 0%{?suse_version} > 1315
%post
%systemd_post zmasset.service
%preun
%systemd_preun zmasset.service
%postun
%systemd_postun_with_restart zmasset.service
%endif

%changelog
