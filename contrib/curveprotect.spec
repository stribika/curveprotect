Name:		curveprotect
Version:	_VERSION_
Release:	1
Summary:	CurveProtect

Group:		Utilities/System
License:	Public Domain
URL:		http://mojzis.com/software/curveprotect/
Source0:	curveprotect-_VERSION_.tar.bz2

BuildRequires:	gcc
Requires:	python

%description
CurveProtect is set of tools which protect your network
communication.

%prep
%setup -q

%build
./do
%define cphomedir "_CURVEPROTECT_"

%install
./do install %{buildroot}

%post
cp -p %{cphomedir}/bin/_* %{cphomedir}/sbin/
%{cphomedir}"/sbin/_postinst"

%preun
%{cphomedir}"/sbin/_prerm"

%postun
%{cphomedir}"/sbin/_postrm"

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root, 0755)
_CURVEPROTECT_/*

%doc README

%changelog
* Tue Nov 06 2012 - radek.rada (at) gmail.com
- Initial implementation
