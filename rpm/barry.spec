Summary: BlackBerry (tm) Desktop for Linux
Name: barry
License: GPL
Version: 0.1
Release: 1bcharge
Group: Applications/Productivity
Source: barry-0.1.tar.gz
URL: http://www.netdirect.ca/downloads/barry
Vendor: Net Direct Inc.
BuildRoot: /var/tmp/%{name}-buildroot
BuildPrereq: libusb, libusb-devel, gcc-c++

%description
Barry is a desktop tool for managing your BlackBerry (tm) device. (BlackBerry is a registered trademark of Research in Motion Limited.)

%prep
%setup

%build
g++ -Wall -g -o bcharge bcharge.cc -lusb

%install
mkdir -p $RPM_BUILD_ROOT/usr/sbin $RPM_BUILD_ROOT/etc/udev/rules.d
cp bcharge $RPM_BUILD_ROOT/usr/sbin
cp 10-blackberry.rules $RPM_BUILD_ROOT/etc/udev/rules.d

%files
/usr/sbin/bcharge	
%config /etc/udev/rules.d/10-blackberry.rules
