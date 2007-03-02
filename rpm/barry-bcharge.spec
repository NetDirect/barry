Summary: BlackBerry(tm) Desktop for Linux  - bcharge utility
Name: barry-bcharge
License: GPL
Version: 0.7
Release: 1
Group: Applications/Productivity
Source: barry-0.7.tar.gz
URL: http://www.netdirect.ca/downloads/barry
Vendor: Net Direct Inc.
BuildRoot: /var/tmp/%{name}-buildroot
Requires: libusb
BuildRequires: libusb-devel, gcc-c++

%description
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This packages contains the USB charge utility, bcharge. The bcharge utility
will adjust the USB port current from 100mA to 500mA when a BlackBerry device
is detected. In the case of a newer BlackBerry with USB Mass Storage support,
it will also reset the device with the secret handshake to expose the USB
interface descriptor that btool will need in order to operate.

%prep

%setup -n barry-%{version}

%build
%{__cxx} -Wall -g -o tools/bcharge tools/bcharge.cc -lusb

%install
%{__mkdir_p} %{buildroot}%{_sbindir}
%{__cp} tools/bcharge %{buildroot}%{_sbindir}
%{__mkdir_p} %{buildroot}%{_mandir}/man1
%{__cp} man/bcharge.1 %{buildroot}%{_mandir}/man1
%{__mkdir_p} %{buildroot}%{_sysconfdir}/udev/rules.d
%{__cp} udev/10-blackberry.rules %{buildroot}%{_sysconfdir}/udev/rules.d
%{__cp} udev/99-barry-perms %{buildroot}%{_sysconfdir}/udev/rules.d

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING NEWS README TODO
%attr(0755,root,root) %{_sbindir}/*
%attr(0644,root,root) %{_mandir}/man1/*
%attr(0644,root,root) %config %{_sysconfdir}/udev/rules.d/*

%changelog
* Fri Mar 02 2007 Chris Frey <cdfrey@foursquare.net> 0.7-1
- bumped version number
- removed patch dependency, as patch has been applied

* Thu Mar 01 2007 Troy Engel <tengel@users.sourceforge.net> 0.6-1
- renamed build RPM to barry-bcharge and enhanced description
- reworked spec file to build right out of official releases
- use _cxx RPM macro instead of raw g++ to compile
- added udev patch to support 0x0006 (Pearl) (sf.net patch #1663986)
- added man page, new udev rules and official docs

* Mon Feb 19 2007 Troy Engel <tengel@users.sourceforge.net> 0.2-1cvs20070219
- build new package with HEAD code and name 0.2 (unofficial)
- specfile updates to be more RPM-esque

* Sat Jan 06 2007 Troy Engel <tengel@users.sourceforge.net> 0.1-1pearl
- apply patches to support productID 0x0006, Pearl

