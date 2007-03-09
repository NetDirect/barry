## pass in '--with gui' to build the GUI tools
# ex.: rpmbuild -ba barry.spec --with gui
#
## pass in '--with opensync' to build the opensync plugin
# ex.: rpmbuild -ba barry.spec --with opensync

# newer bcond_with() macros are not available on RHEL4/CentOS4 and below
%{?_with_gui: %define with_gui 1}
%{!?_with_gui: %define with_gui 0}
%{?_with_opensync: %define with_opensync 1}
%{!?_with_opensync: %define with_opensync 0}

Summary: BlackBerry(tm) Desktop for Linux
Name: barry
Version: 0.7
Release: 1
Group: Applications/Productivity
License: GPL
Source: %{name}-%{version}.tar.gz
URL: http://www.netdirect.ca/downloads/barry
Vendor: Net Direct Inc.
BuildRoot: %{_tmppath}/%{name}-%{release}-%{version}-root
BuildRequires: libusb-devel, gcc-c++, pkgconfig, boost-devel, openssl-devel

%define barryroot %{_builddir}/%{name}-%{version}

%description
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)


%package -n libbarry
Summary: BlackBerry(tm) Desktop for Linux - libbarry libraries
Group: Development/Libraries
Requires: libusb openssl boost

%description -n libbarry
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the library files, license agreement, README file,
and most other assorted documentation common to all sub-packages. You most
likely want to also install barry-util and barry-gui.


%package -n libbarry-devel
Summary: BlackBerry(tm) Desktop for Linux - libbarry libraries
Group: Development/Libraries
Requires: libbarry libusb-devel openssl-devel boost-devel

%description -n libbarry-devel
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the development library files for Barry, libbarry.


%package util
Summary: BlackBerry(tm) Desktop for Linux - bcharge, btool, breset and others
Group: Applications/Productivity
Requires: libbarry
Conflicts: barry-bcharge

%description util
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the commandline tools bcharge, btool, breset and others
which will enable you to charge your device with a proper 500mA and be able
to access the data on the device in many ways.


%if %{with_gui}
%package gui
Summary: BlackBerry(tm) Desktop for Linux - bcharge, btool, breset and others
Group: Applications/Productivity
Requires: libbarry gtkmm24 libglademm24 libtar
BuildRequires: gtkmm24-devel libglademm24-devel libtar-devel

%description gui
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the GUI applications built on top of libbarry.
%endif


%if %{with_opensync}
%package opensync
Summary: BlackBerry(tm) Desktop for Linux - opensync plugin
Group: Applications/Productivity
Requires: libbarry libopensync
BuildRequires: libopensync-devel

%description opensync
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the opensync plugin.
%endif

%prep
%setup -q

%build
# main tree
%{configure} --with-boost=%{_prefix}
%{__make} %{?_smp_mflags}

# gui tree
%if %{with_gui}
cd gui/
%{configure} PKG_CONFIG_PATH="../" CXXFLAGS="-I../.." LDFLAGS="-L../../src"
%{__make} %{?_smp_mflags}
cd ../
%endif

# opensync tree
%if %{with_opensync}
cd opensync-plugin/
%{configure} PKG_CONFIG_PATH="../" CXXFLAGS="-I../.." LDFLAGS="-L../../src"
%{__make} %{?_smp_mflags}
cd ../
%endif

%install
# main tree
%{__make} DESTDIR=%{buildroot} install
%{__mkdir_p} %{buildroot}%{_sysconfdir}/udev/rules.d
%{__cp} udev/10-blackberry.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
%{__mkdir_p} %{buildroot}%{_sysconfdir}/security/console.perms.d
%{__cp} udev/10-blackberry.perms %{buildroot}%{_sysconfdir}/security/console.perms.d/

# gui tree
%if %{with_gui}
cd gui/
%{__make} DESTDIR=%{buildroot} install
cd ../
%endif

# opensync tree
%if %{with_opensync}
cd opensync-plugin/
%{__make} DESTDIR=%{buildroot} install
cd ../
%endif

%files -n libbarry
%defattr(-,root,root)
%attr(-,root,root) %{_libdir}/*.so*
%doc AUTHORS ChangeLog COPYING NEWS README

%files -n libbarry-devel
%defattr(-,root,root)
%doc examples/*.cc examples/*.am
%attr(0644,root,root) %{_includedir}/barry/*
%attr(0644,root,root) %{_libdir}/*.a
%attr(0755,root,root) %{_libdir}/*.la
%attr(0644,root,root) %{_libdir}/pkgconfig/*.pc
%doc COPYING TODO doc/*

%files util
%defattr(-,root,root)
%attr(0755,root,root) %{_sbindir}/bcharge
%attr(0755,root,root) %{_sbindir}/breset
%attr(0755,root,root) %{_bindir}/btool
%attr(0755,root,root) %{_bindir}/upldif
%attr(0755,root,root) %{_bindir}/ktrans
%attr(0755,root,root) %{_bindir}/translate
%attr(0644,root,root) %{_mandir}/man1/btool*
%attr(0644,root,root) %{_mandir}/man1/bcharge*
%attr(0644,root,root) %config %{_sysconfdir}/udev/rules.d/*
%attr(0644,root,root) %config %{_sysconfdir}/security/console.perms.d/*
%doc COPYING

%if %{with_gui}
%files gui
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/barrybackup
%attr(0644,root,root) %{_datadir}/barry/glade/*.glade
%doc COPYING
%endif

%if %{with_opensync}
%files opensync
%defattr(-,root,root)
%attr(0755,root,root) %{_libdir}/opensync/plugins/*
%attr(0644,root,root) %{_datadir}/opensync/defaults/*
%doc COPYING
%endif

%clean
[ "%{buildroot}" != "/" ] && %{__rm} -rf %{buildroot}
[ "%{barryroot}" != "/" ] && %{__rm} -rf %{barryroot}

%post -n libbarry
/sbin/ldconfig

%postun -n libbarry
/sbin/ldconfig

%changelog
* Thu Mar 08 2007 Chris Frey <cdfrey@foursquare.net> 0.7-1
- removed barry base package that only contained docs, and put docs in libbarry*
- changed barrybackup reference to barry-gui
- removed the patch step, as version 0.7 shouldn't need it
- added license file to each package

* Sun Mar 04 2007 Troy Engel <tengel@users.sourceforge.net> 0.6-1
- initial build
- adding udev and console perms patch for raw 0.6

