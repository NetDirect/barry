%dump

# always build with GUI
%define with_gui 1

# Fedora 9 doesn't support opensync 0.22
%if 0%{?fc9}
	%define with_opensync 0
%else
	%define with_opensync 1
%endif



Summary: BlackBerry(tm) Desktop for Linux
Name: barry
Version: 0.16
Release: 0
Group: Applications/Productivity
License: GPLv2+
Source: %{name}-%{version}.tar.bz2
URL: http://www.netdirect.ca/software/packages/barry
Vendor: Net Direct Inc.
BuildRoot: %{_tmppath}/%{name}-%{release}-%{version}-root

%if 0%{?suse_version}
BuildRequires: libusb, gcc-c++, pkgconfig, boost-devel, fuse-devel, zlib-devel
%else
BuildRequires: libusb-devel, gcc-c++, pkgconfig, boost-devel, fuse-devel, zlib-devel
%endif

%define barryroot %{_builddir}/%{name}-%{version}

%description
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)


%package -n libbarry0
Summary: BlackBerry(tm) Desktop for Linux - libbarry libraries
Group: Development/Libraries
Requires: libusb boost

%description -n libbarry0
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the library files, license agreement, README file,
and most other assorted documentation common to all sub-packages. You most
likely want to also install barry-util and barry-gui.


%package -n libbarry-devel
Summary: BlackBerry(tm) Desktop for Linux - libbarry libraries
Group: Development/Libraries
%if 0%{?suse_version}
Requires: libbarry0 libusb boost-devel
%else
Requires: libbarry0 libusb-devel boost-devel
%endif

%description -n libbarry-devel
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the development library files for Barry, libbarry.


%package util
Summary: BlackBerry(tm) Desktop for Linux - bcharge, btool, breset and others
Group: Applications/Productivity
Requires: libbarry0 fuse
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
%if 0%{?suse_version}
Requires: libbarry0 gtkmm2 libglademm libtar
BuildRequires: gtkmm2-devel libglademm-devel libtar-devel
%else
Requires: libbarry0 gtkmm24 libglademm24 libtar
BuildRequires: gtkmm24-devel libglademm24-devel libtar-devel
%endif

%description gui
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the GUI applications built on top of libbarry.
%endif


%if %{with_opensync}
%package opensync
Summary: BlackBerry(tm) Desktop for Linux - opensync plugin
Group: Applications/Productivity
Requires: libbarry0, libopensync >= 0.22
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
%{configure} --enable-boost --with-zlib
%{__make} %{?_smp_mflags}

# gui tree
%if %{with_gui}
cd gui/
%{configure} PKG_CONFIG_PATH="..:$PKG_CONFIG_PATH" CXXFLAGS="-I../.." LDFLAGS="-L../../src"
%{__make} %{?_smp_mflags}
cd ../
%endif

# opensync tree
%if %{with_opensync}
cd opensync-plugin/
%{configure} PKG_CONFIG_PATH="..:$PKG_CONFIG_PATH" CXXFLAGS="-I../.." LDFLAGS="-L../../src"
%{__make} %{?_smp_mflags}
cd ../
%endif

%install
# main tree
%{__make} DESTDIR=%{buildroot} install
# delete some test-only programs
%{__rm} -f %{buildroot}%{_bindir}/bdptest
%{__rm} -f %{buildroot}%{_bindir}/bjvmdebug
# proceed as usual...
%{__mkdir_p} %{buildroot}%{_sysconfdir}/udev/rules.d
%if 0%{?suse_version} == 1110
%{__mkdir_p} %{buildroot}%{_sysconfdir}/security/console.perms.d
%{__cp} udev/opensuse-11.1/65-blackberry.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
%{__cp} udev/opensuse-11.1/10-blackberry.perms %{buildroot}%{_sysconfdir}/security/console.perms.d/
%else
%{__cp} udev/10-blackberry.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
%{__cp} udev/99-blackberry-perms.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
%endif
%{__mkdir_p} %{buildroot}%{_sysconfdir}/modprobe.d
%{__cp} modprobe/blacklist-berry_charge.conf %{buildroot}%{_sysconfdir}/modprobe.d/
%{__mkdir_p} %{buildroot}%{_sysconfdir}/ppp/peers
%{__cp} ppp/barry-rogers %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-verizon %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-sprint %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-o2ireland %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-tmobileus %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-att_cingular %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-chinamobile %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-kpn %{buildroot}%{_sysconfdir}/ppp/peers/
%{__mkdir_p} %{buildroot}%{_sysconfdir}/chatscripts
%{__cp} ppp/barry-rogers.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-verizon.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-sprint.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-o2ireland.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-tmobileus.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-att_cingular.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-chinamobile.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-kpn.chat %{buildroot}%{_sysconfdir}/chatscripts/
# Install hal fdi config
%{__mkdir_p} %{buildroot}%{_datadir}/hal/fdi/information/10freedesktop
%{__mkdir_p} %{buildroot}%{_datadir}/hal/fdi/policy/10osvendor
%{__cp} hal/fdi/information/10freedesktop/10-blackberry.fdi %{buildroot}%{_datadir}/hal/fdi/information/10freedesktop
%{__cp} hal/fdi/policy/10osvendor/19-blackberry-acl.fdi %{buildroot}%{_datadir}/hal/fdi/policy/10osvendor
# Install hal support script
%{__mkdir_p} %{buildroot}%{_bindir}
%{__cp} hal/hal-blackberry %{buildroot}%{_bindir}
# Install bash completion scripts
%{__mkdir_p} %{buildroot}%{_sysconfdir}/bash_completion.d
%{__cp} bash/bjavaloader %{buildroot}%{_sysconfdir}/bash_completion.d
%{__cp} bash/btool %{buildroot}%{_sysconfdir}/bash_completion.d

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

%files -n libbarry0
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
%attr(0755,root,root) %{_sbindir}/pppob
%attr(0755,root,root) %{_bindir}/btool
%attr(0755,root,root) %{_bindir}/bfuse
%attr(0755,root,root) %{_bindir}/bjavaloader
%attr(0755,root,root) %{_bindir}/bjdwp
%attr(0755,root,root) %{_bindir}/bs11nread
%attr(0755,root,root) %{_bindir}/bidentify
%attr(0755,root,root) %{_bindir}/brecsum
%attr(0755,root,root) %{_bindir}/upldif
%attr(0755,root,root) %{_bindir}/bktrans
%attr(0755,root,root) %{_bindir}/btranslate
%attr(0755,root,root) %{_bindir}/brimtrans
%attr(0755,root,root) %{_bindir}/hal-blackberry
%attr(0644,root,root) %{_mandir}/man1/btool*
%attr(0644,root,root) %{_mandir}/man1/bfuse*
%attr(0644,root,root) %{_mandir}/man1/bjavaloader*
%attr(0644,root,root) %{_mandir}/man1/bjdwp*
%attr(0644,root,root) %{_mandir}/man1/bs11nread*
%attr(0644,root,root) %{_mandir}/man1/bidentify*
%attr(0644,root,root) %{_mandir}/man1/bcharge*
%attr(0644,root,root) %{_mandir}/man1/pppob*
%attr(0644,root,root) %{_mandir}/man1/brecsum*
%attr(0644,root,root) %{_mandir}/man1/breset*
%attr(0644,root,root) %{_mandir}/man1/upldif*
%attr(0644,root,root) %{_datadir}/hal/fdi/information/10freedesktop/10-blackberry.fdi
%attr(0644,root,root) %{_datadir}/hal/fdi/policy/10osvendor/19-blackberry-acl.fdi
%attr(0644,root,root) %config %{_sysconfdir}/udev/rules.d/*
%attr(0644,root,root) %config %{_sysconfdir}/modprobe.d/blacklist-berry_charge.conf
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-rogers
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-verizon
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-sprint
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-o2ireland
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-tmobileus
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-att_cingular
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-chinamobile
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-kpn
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-rogers.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-verizon.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-sprint.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-o2ireland.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-tmobileus.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-att_cingular.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-chinamobile.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-kpn.chat
%attr(0640,root,root) %config %{_sysconfdir}/bash_completion.d/bjavaloader
%attr(0640,root,root) %config %{_sysconfdir}/bash_completion.d/btool
%doc COPYING
%doc zsh

%if %{with_gui}
%files gui
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/barrybackup
%attr(0644,root,root) %{_datadir}/barry/glade/*.glade
%attr(0644,root,root) %{_mandir}/man1/barrybackup*
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

%post -n libbarry0
/sbin/ldconfig

%postun -n libbarry0
/sbin/ldconfig

%changelog
* Sat Sep 29 2009 Chris Frey <cdfrey@foursquare.net> 0.16-0
- version bump
- added new ppp chat script for KPN Nederland
- using new udev rules set
- added bjdwp and manpage, and removed some test-only programs
- added bash and zsh completion scripts

* Fri Apr 10 2009 Chris Frey <cdfrey@foursquare.net> 0.15-0
- version bump
- added HAL FDI scripts
- added bjavaloader and bfuse
- updated for udev directory reorganization in Barry source tree
- added zlib-devel to BuildRequires list
- added brimtrans

* Wed Sep 24 2008 Chris Frey <cdfrey@foursquare.net> 0.14-0
- version bump
- added new ppp chat script for T-Mobile US
- renamed libbarry to libbarry0

* Thu May 29 2008 Chris Frey <cdfrey@foursquare.net> 0.13-1
- version bump
- added brecsum
- added ppp options and chat scripts
- added manpages for pppob, brecsum, breset, upldif, barrybackup
- spec file now assumes gui and opensync, with conditional checks depending on host

* Fri Dec 07 2007 Chris Frey <cdfrey@foursquare.net> 0.12-1
- version bump

* Fri Nov 30 2007 Chris Frey <cdfrey@foursquare.net> 0.11-1
- version bump

* Fri Nov 30 2007 Chris Frey <cdfrey@foursquare.net> 0.10-1
- version bump
- removed ktrans and translate from rpm package
- added bidentify

* Thu Aug 09 2007 Chris Frey <cdfrey@foursquare.net> 0.9-1
- version bump

* Fri Aug 03 2007 Chris Frey <cdfrey@foursquare.net> 0.8-1
- version bump
- changed tarball to bz2

* Tue May 01 2007 Chris Frey <cdfrey@foursquare.net> 0.7-2
- added pppob to utils

* Thu Mar 08 2007 Chris Frey <cdfrey@foursquare.net> 0.7-1
- removed barry base package that only contained docs, and put docs in libbarry*
- changed barrybackup reference to barry-gui
- removed the patch step, as version 0.7 shouldn't need it
- added license file to each package

* Sun Mar 04 2007 Troy Engel <tengel@users.sourceforge.net> 0.6-1
- initial build
- adding udev and console perms patch for raw 0.6

