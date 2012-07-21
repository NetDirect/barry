%dump

#
# Note: This spec file is intended to be as cross-platform as possible.
#       As such, it skips listing the BuildRequires lines, expecting to
#       be built on a system with rpmbuild -ba only, and dependencies
#       pre-installed.  See the ../maintainer/depscripts/ directory
#       in the Barry source package for scripts to automate installation
#       of dependencies.
#

# enable GUI using: --with gui
%define with_gui 0%{?_with_gui:1}

# enable opensync 0.2x using: --with opensync
%define with_opensync 0%{?_with_opensync:1}

# enable opensync 0.4x using: --with opensync4x
%define with_opensync4x 0%{?_with_opensync4x:1}

# enable desktop using: --with desktop
%define with_desktop 0%{?_with_desktop:1}

# enable kdesu for desktop instead of beesu: --with kdesu
%define with_kdesu 0%{?_with_kdesu:1}

%if %{with_kdesu}
%define guisu_name kdesu
%define guisu_package kdebase4-runtime
%else
%define guisu_name beesu
%define guisu_package beesu
%endif


Summary: BlackBerry(tm) Desktop for Linux
Name: barry
Version: 0.19.0
Release: 0
Group: Applications/Productivity
License: GPLv2+
Source: %{name}-%{version}.tar.bz2
URL: http://www.netdirect.ca/software/packages/barry
Vendor: Net Direct Inc.
BuildRoot: %{_tmppath}/%{name}-%{release}-%{version}-root

#BuildRequires: libusb-devel, gcc-c++, pkgconfig, boost-devel, fuse-devel, zlib-devel

#%if %{with_gui}
#BuildRequires: desktop-file-utils
#%endif

# desktop tree
#%if %{with_desktop}
#BuildRequires: wxGTK-devel, evolution-data-server-devel
#%endif

%define barryroot %{_builddir}/%{name}-%{version}

%description
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)


%package -n libbarry0
Summary: BlackBerry(tm) Desktop for Linux - libbarry libraries
Group: Development/Libraries
#Requires: libusb boost

%description -n libbarry0
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the library files, license agreement, README file,
and most other assorted documentation common to all sub-packages. You most
likely want to also install barry-util and barry-gui.


%package -n libbarry-devel
Summary: BlackBerry(tm) Desktop for Linux - libbarry libraries
Group: Development/Libraries
Requires: libbarry0 boost-devel

%description -n libbarry-devel
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the development library files for Barry, libbarry.


%package util
Summary: BlackBerry(tm) Desktop for Linux - bcharge, btool, breset, bio and others
Group: Applications/Productivity
Requires: libbarry0 fuse
Conflicts: barry-bcharge

%description util
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the commandline tools bcharge, btool, breset, bio and
others which will enable you to charge your device with a proper 500mA
and be able to access the data on the device in many ways.


%if %{with_gui}
%package gui
Summary: BlackBerry(tm) Desktop for Linux - bcharge, btool, breset and others
Group: Applications/Productivity
Requires: libbarry0
#BuildRequires: gtkmm24-devel libglademm24-devel libtar-devel

%description gui
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the GUI applications built on top of libbarry.
%endif

# opensync 0.2x
%if %{with_opensync}
%package opensync
Summary: BlackBerry(tm) Desktop for Linux - opensync plugin
Group: Applications/Productivity
Requires: libbarry0, libopensync >= 0.22
#BuildRequires: libopensync-devel

%description opensync
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the opensync plugin.
%endif

# opensync 0.4x
%if %{with_opensync4x}
%package opensync4x
Summary: BlackBerry(tm) Desktop for Linux - opensync 0.4x plugin
Group: Applications/Productivity
Requires: libbarry0, libopensync1 >= 0.39
#BuildRequires: libopensync1-devel

%description opensync4x
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the opensync 0.4x plugin.
%endif

# desktop tree
%if %{with_desktop}
%package desktop
Summary: BlackBerry(tm) Desktop Panel GUI for Linux
Group: Applications/Productivity
Requires: libbarry0 barry-util ppp xterm %{guisu_package}
#BuildRequires: wxGTK-devel

%description desktop
Barry is a desktop toolset for managing your BlackBerry(tm) device. (BlackBerry
is a registered trademark of Research in Motion Limited.)

This package contains the desktop panel GUI.
%endif

%prep
[ "%{buildroot}" != "/" ] && %{__rm} -rf %{buildroot}
%setup -q

%build
# some systems have an rpath checker, and need their own versions of
# configure built on the same system in order to pass... in particular,
# Fedora 14's 64bit version needs its own configure, for some reason,
# in order to recognize that /usr/lib64 is a system path and therefore
# there is no reason to use an rpath... unfortunately, configure's
# --disable-rpath option seems to have no effect whatsoever. :-(
%if "%{?_lib}" == "lib64"
	./buildgen.sh cleanall
	./buildgen.sh
%endif

# Generate configure if it does not exist already (for binary-meta)
if [ ! -f ./configure ] ; then
	./buildgen.sh
fi

# setup the environment if there are additions (for binary-meta)
export ORIG_PKG_CONFIG_PATH="$PKG_CONFIG_PATH"
if [ -n "$ADD_TO_PKG_CONFIG_PATH" ] ; then
	export PKG_CONFIG_PATH="$ADD_TO_PKG_CONFIG_PATH:$ORIG_PKG_CONFIG_PATH"
fi
set

# main tree
%{configure} --enable-boost --enable-nls --with-zlib --with-libusb --enable-rpathhack
%{__make} %{?_smp_mflags}

# gui tree
%if %{with_gui}
cd gui/
%{configure} PKG_CONFIG_PATH="..:$PKG_CONFIG_PATH" CXXFLAGS="-I../.." LDFLAGS="-L../../src" --enable-nls --enable-rpathhack
%{__make} %{?_smp_mflags}
cd ../
%endif

# opensync tree
%if %{with_opensync}
# if there is a special pkgconfig for opensync 0.2x, use it
if [ -n "$OSYNC2X_PKG_CONFIG_PATH" ] ; then
	export PKG_CONFIG_PATH="$OSYNC2X_PKG_CONFIG_PATH:$ORIG_PKG_CONFIG_PATH"
fi
cd opensync-plugin/
%{configure} PKG_CONFIG_PATH="..:$PKG_CONFIG_PATH" CXXFLAGS="-I../.." LDFLAGS="-L../../src" --enable-nls --enable-rpathhack
%{__make} %{?_smp_mflags}
cd ../
%endif

# opensync4x tree
%if %{with_opensync4x}
# if there is a special pkgconfig for opensync 0.4x, use it
if [ -n "$OSYNC4X_PKG_CONFIG_PATH" ] ; then
	export PKG_CONFIG_PATH="$OSYNC4X_PKG_CONFIG_PATH:$ORIG_PKG_CONFIG_PATH"
fi
cd opensync-plugin-0.4x/
%{configure} PKG_CONFIG_PATH="..:$PKG_CONFIG_PATH" CXXFLAGS="-I../.." LDFLAGS="-L../../src" --enable-nls --enable-rpathhack
%{__make} %{?_smp_mflags}
cd ../
%endif

# desktop tree
%if %{with_desktop}
# if there is a special pkgconfig for both opensync 0.4x and 0.2x, use it
if [ -n "$OSYNCBOTH_PKG_CONFIG_PATH" ] ; then
	export PKG_CONFIG_PATH="$OSYNCBOTH_PKG_CONFIG_PATH:$ORIG_PKG_CONFIG_PATH"
fi
cd desktop/
%{configure} PKG_CONFIG_PATH="..:$PKG_CONFIG_PATH" CXXFLAGS="-I../.." LDFLAGS="-L../../src" --enable-nls --enable-rpathhack --with-evolution --with-guisu=/usr/bin/%{guisu_name}
%{__make} %{?_smp_mflags}
cd ../
%endif

%install
# main tree
%{__make} DESTDIR=%{buildroot} install
# delete some test-only programs
%{__rm} -f %{buildroot}%{_bindir}/bdptest
%{__rm} -f %{buildroot}%{_bindir}/bjvmdebug
# delete the .la files
%{__rm} -f %{buildroot}%{_libdir}/*.la
# proceed as usual...
%{__mkdir_p} %{buildroot}%{_sysconfdir}/udev/rules.d
%{__cp} udev/10-blackberry.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
%{__cp} udev/99-blackberry-perms.rules %{buildroot}%{_sysconfdir}/udev/rules.d/
%{__mkdir_p} %{buildroot}%{_sysconfdir}/modprobe.d
%{__cp} modprobe/blacklist-berry_charge.conf %{buildroot}%{_sysconfdir}/modprobe.d/
%{__mkdir_p} %{buildroot}%{_sysconfdir}/ppp/peers
%{__cp} ppp/barry-rogers %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-minimal %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-verizon %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-sprint %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-telus %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-o2ireland %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-emobile %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-tmobileus %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-att_cingular %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-chinamobile %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-kpn %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-orange-spain %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-orangeuk %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-mts %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-optus-au %{buildroot}%{_sysconfdir}/ppp/peers/
%{__cp} ppp/barry-vodafone-au %{buildroot}%{_sysconfdir}/ppp/peers/
%{__mkdir_p} %{buildroot}%{_sysconfdir}/chatscripts
%{__cp} ppp/barry-rogers.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-minimal.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-verizon.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-sprint.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-telus.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-o2ireland.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-emobile.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-tmobileus.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-att_cingular.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-chinamobile.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-kpn.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-orange-spain.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-orangeuk.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-mts.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-optus-au.chat %{buildroot}%{_sysconfdir}/chatscripts/
%{__cp} ppp/barry-vodafone-au.chat %{buildroot}%{_sysconfdir}/chatscripts/
# Install hal fdi config
%{__mkdir_p} %{buildroot}%{_datadir}/hal/fdi/information/10freedesktop
%{__mkdir_p} %{buildroot}%{_datadir}/hal/fdi/policy/10osvendor
%{__cp} hal/fdi/information/10freedesktop/10-blackberry.fdi %{buildroot}%{_datadir}/hal/fdi/information/10freedesktop
%{__cp} hal/fdi/policy/10osvendor/19-blackberry-acl.fdi %{buildroot}%{_datadir}/hal/fdi/policy/10osvendor
# Install hal support script
%{__mkdir_p} %{buildroot}%{_libdir}/barry
%{__cp} hal/hal-blackberry %{buildroot}%{_libdir}/barry
# Install bash completion scripts
%{__mkdir_p} %{buildroot}%{_sysconfdir}/bash_completion.d
%{__cp} bash/bjavaloader %{buildroot}%{_sysconfdir}/bash_completion.d
%{__cp} bash/btool %{buildroot}%{_sysconfdir}/bash_completion.d

# gui tree
%if %{with_gui}
cd gui/
%{__make} DESTDIR=%{buildroot} install
# Install barry logo icon (name of icon must match the *.desktop file!)
cd ../
%{__mkdir_p} %{buildroot}%{_datadir}/pixmaps
%{__cp} logo/barry_logo_icon.png %{buildroot}%{_datadir}/pixmaps/barry_backup_menu_icon.png
desktop-file-install --vendor netdirect \
   --dir %{buildroot}%{_datadir}/applications \
   menu/barrybackup.desktop
%endif

# opensync tree
%if %{with_opensync}
cd opensync-plugin/
%{__make} DESTDIR=%{buildroot} install
# remove .la files
%{__rm} -f %{buildroot}%{_libdir}/opensync/plugins/*.la
cd ../
%endif

# opensync4x tree
%if %{with_opensync4x}
cd opensync-plugin-0.4x/
%{__make} DESTDIR=%{buildroot} install
# remove .la files
%{__rm} -f %{buildroot}%{_libdir}/libopensync1/plugins/*.la
cd ../
%endif

# desktop tree
%if %{with_desktop}
cd desktop/
%{__make} DESTDIR=%{buildroot} install
# remove .la files
%{__rm} -f %{buildroot}%{_libdir}/*.la
cd ../
%{__mkdir_p} %{buildroot}%{_datadir}/pixmaps
%{__cp} logo/barry_logo_icon.png %{buildroot}%{_datadir}/pixmaps/barry_desktop_menu_icon.png
desktop-file-install --vendor netdirect \
   --dir %{buildroot}%{_datadir}/applications \
   menu/barrydesktop.desktop
%endif

%files -n libbarry0
%defattr(-,root,root)
%attr(-,root,root) %{_libdir}/libbarry.so.*
%attr(-,root,root) %{_libdir}/libbarrydp.so.*
%attr(-,root,root) %{_libdir}/libbarryjdwp.so.*
%attr(-,root,root) %{_libdir}/libbarrysync.so.*
%attr(-,root,root) %{_libdir}/libbarrybackup.so.*
%attr(-,root,root) %{_libdir}/libbarryalx.so.*
%doc AUTHORS ChangeLog KnownBugs COPYING NEWS README

%files -n libbarry-devel
%defattr(-,root,root)
%attr(0644,root,root) %{_includedir}/barry*/barry/*
%attr(0644,root,root) %{_libdir}/libbarry.a
%attr(0644,root,root) %{_libdir}/libbarry.so
%attr(0644,root,root) %{_libdir}/libbarrydp.a
%attr(0644,root,root) %{_libdir}/libbarrydp.so
%attr(0644,root,root) %{_libdir}/libbarryjdwp.a
%attr(0644,root,root) %{_libdir}/libbarryjdwp.so
%attr(0644,root,root) %{_libdir}/libbarrysync.a
%attr(0644,root,root) %{_libdir}/libbarrysync.so
%attr(0644,root,root) %{_libdir}/libbarrybackup.a
%attr(0644,root,root) %{_libdir}/libbarrybackup.so
%attr(0644,root,root) %{_libdir}/libbarryalx.a
%attr(0644,root,root) %{_libdir}/libbarryalx.so
%attr(0644,root,root) %{_libdir}/pkgconfig/*.pc
%doc COPYING TODO KnownBugs doc/* examples/*.cc

%files util
%defattr(-,root,root)
%attr(0755,root,root) %{_sbindir}/bcharge
%attr(0755,root,root) %{_sbindir}/breset
%attr(0755,root,root) %{_sbindir}/pppob
%attr(0755,root,root) %{_bindir}/btool
%attr(0755,root,root) %{_bindir}/bwatch
%attr(0755,root,root) %{_bindir}/bio
%attr(0755,root,root) %{_bindir}/btardump
%attr(0755,root,root) %{_bindir}/btarcmp
%attr(0755,root,root) %{_bindir}/bfuse
%attr(0755,root,root) %{_bindir}/bjavaloader
%attr(0755,root,root) %{_bindir}/balxparse
%attr(0755,root,root) %{_bindir}/bjdwp
%attr(0755,root,root) %{_bindir}/brawchannel
%attr(0755,root,root) %{_bindir}/bs11nread
%attr(0755,root,root) %{_bindir}/bidentify
%attr(0755,root,root) %{_bindir}/brecsum
%attr(0755,root,root) %{_bindir}/upldif
%attr(0755,root,root) %{_libdir}/barry/hal-blackberry
%attr(0644,root,root) %{_mandir}/man1/btool*
%attr(0644,root,root) %{_mandir}/man1/bwatch*
%attr(0644,root,root) %{_mandir}/man1/bio*
%attr(0644,root,root) %{_mandir}/man1/btardump*
%attr(0644,root,root) %{_mandir}/man1/btarcmp*
%attr(0644,root,root) %{_mandir}/man1/bfuse*
%attr(0644,root,root) %{_mandir}/man1/bjavaloader*
%attr(0644,root,root) %{_mandir}/man1/balxparse*
%attr(0644,root,root) %{_mandir}/man1/bjdwp*
%attr(0644,root,root) %{_mandir}/man1/brawchannel*
%attr(0644,root,root) %{_mandir}/man1/bs11nread*
%attr(0644,root,root) %{_mandir}/man1/bidentify*
%attr(0644,root,root) %{_mandir}/man1/bcharge*
%attr(0644,root,root) %{_mandir}/man1/pppob*
%attr(0644,root,root) %{_mandir}/man1/brecsum*
%attr(0644,root,root) %{_mandir}/man1/breset*
%attr(0644,root,root) %{_mandir}/man1/upldif*
%attr(0644,root,root) %{_datadir}/locale/*/LC_MESSAGES/barry.mo
%attr(0644,root,root) %{_datadir}/hal/fdi/information/10freedesktop/10-blackberry.fdi
%attr(0644,root,root) %{_datadir}/hal/fdi/policy/10osvendor/19-blackberry-acl.fdi
%attr(0644,root,root) %config %{_sysconfdir}/udev/rules.d/*
%attr(0644,root,root) %config %{_sysconfdir}/modprobe.d/blacklist-berry_charge.conf
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-rogers
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-minimal
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-verizon
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-sprint
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-telus
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-o2ireland
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-emobile
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-tmobileus
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-att_cingular
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-chinamobile
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-kpn
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-orange-spain
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-orangeuk
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-mts
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-optus-au
%attr(0644,root,root) %config %{_sysconfdir}/ppp/peers/barry-vodafone-au
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-rogers.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-minimal.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-verizon.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-sprint.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-telus.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-o2ireland.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-emobile.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-tmobileus.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-att_cingular.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-chinamobile.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-kpn.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-orange-spain.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-orangeuk.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-mts.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-optus-au.chat
%attr(0640,root,root) %config %{_sysconfdir}/chatscripts/barry-vodafone-au.chat
%attr(0640,root,root) %config %{_sysconfdir}/bash_completion.d/bjavaloader
%attr(0640,root,root) %config %{_sysconfdir}/bash_completion.d/btool
%doc COPYING
%doc zsh
%doc ppp/README

%if %{with_gui}
%files gui
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/barrybackup
%attr(0644,root,root) %{_datadir}/barry/glade/*.glade
%attr(0644,root,root) %{_datadir}/pixmaps/barry_backup_menu_icon.png
%attr(0644,root,root) %{_datadir}/applications/*barrybackup.desktop
%attr(0644,root,root) %{_mandir}/man1/barrybackup*
%attr(0644,root,root) %{_datadir}/locale/*/LC_MESSAGES/barry-backup.mo
%doc COPYING
%endif

%if %{with_opensync}
%files opensync
%defattr(-,root,root)
%attr(0755,root,root) %{_libdir}/opensync/plugins/barry_sync.so
%attr(0644,root,root) %{_datadir}/opensync/defaults/barry-sync
%attr(0644,root,root) %{_datadir}/locale/*/LC_MESSAGES/barry-opensync-plugin.mo
%doc COPYING
%endif

%if %{with_opensync4x}
%files opensync4x
%defattr(-,root,root)
%attr(0755,root,root) %{_libdir}/libopensync1/plugins/barry_sync.so
%attr(0644,root,root) %{_datadir}/libopensync1/defaults/barry-sync
%attr(0644,root,root) %{_datadir}/locale/*/LC_MESSAGES/barry-opensync-plugin-0-4x.mo
%doc COPYING
%endif

# desktop tree
%if %{with_desktop}
%files desktop
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/barrydesktop
%attr(0755,root,root) %{_libexecdir}/barrydesktop/bsyncjail
%attr(0755,root,root) %{_libexecdir}/barrydesktop/blistevo
%attr(0644,root,root) %{_datadir}/barry/desktop/0.22/*
%attr(0644,root,root) %{_datadir}/barry/desktop/0.40/*
%attr(0644,root,root) %{_datadir}/barry/desktop/images/*.png
%attr(0644,root,root) %{_datadir}/pixmaps/barry_desktop_menu_icon.png
%attr(0644,root,root) %{_datadir}/applications/*barrydesktop.desktop
%attr(0644,root,root) %{_mandir}/man1/barrydesktop*
%attr(0644,root,root) %{_datadir}/locale/*/LC_MESSAGES/barrydesktop.mo
%attr(0644,root,root) %{_datadir}/locale/*/LC_MESSAGES/barryosyncwrap.mo
%attr(-,root,root) %{_libdir}/libosyncwrap.so.*
%attr(-,root,root) %{_libdir}/libosyncwrap.a
%attr(-,root,root) %{_libdir}/libosyncwrap.so
%attr(0644,root,root) %{_includedir}/barry*/osyncwrap/*
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
* Fri Jul 20 2012 Chris Frey <cdfrey@foursquare.net> 0.19.0-0
- version bump

* Mon Aug 20 2012 Chris Frey <cdfrey@foursquare.net> 0.18.4-0
- version bump
- added spanish translation, and fixed locations of .mo files, so
  barry.mo goes to utils, and barry-backup.mo goes to backup
- since we are building with binary-meta which contains depscripts,
  removed all BuildRequires lines, to make spec file more portable
  across Fedora and openSUSE
- use kdesu for openSUSE builds, beesu for Fedora
- added desktop's barrydesktop.mo and barryosyncwrap.mo files
- added .mo files for both plugins
- added ppp chatscripts for eMobile Ireland

* Tue May 15 2012 Chris Frey <cdfrey@foursquare.net> 0.18.3-0
- version bump
- renamed icon filenames to match .desktop file

* Tue May 15 2012 Chris Frey <cdfrey@foursquare.net> 0.18.2-0
- version bump

* Tue May  8 2012 Chris Frey <cdfrey@foursquare.net> 0.18.1-0
- version bump

* Mon Aug 31 2011 Chris Frey <cdfrey@foursquare.net> 0.18.0-0
- version bump
- removed dependency of libbarry-devel on libusb(-devel)
- added osyncwrap headers
- removed opensuse special cases
- back to optional --with behaviour
- added opensync 0.4x package support
- removed .la files
- split up dev libraries a little better (-devel should have the dev libs)
- put desktop library in desktop package
- added orangeuk and mts ppp chatscript files
- added bwatch
- added code to clean the buildroot at start
- added evolution dependencies for desktop build
- removed libopensync dependency from barry-desktop (can be either, both, or none)
- renamed 0.4x plugin dependency to libopensync1
- removed extraneous library dependencies, which should be handled automatically
- added btarcmp
- removed brimtrans, bktrans, and btranslate (devel tools)
- added beesu dependency for barrydesktop, for modem mode, to run pppd

* Fri May 28 2010 Chris Frey <cdfrey@foursquare.net> 0.17.0-0
- version bump
- added NLS support
- cleaned up conditionals
- added Fedora 13 support
- added new ppp chat scripts for Orange Spain, Optus and Vodafone AU
- added copy of barry-sprint as barry-telus
- added brawchannel, btardump, bio, and balxparse
- added desktop support
- cleaned up desktop variables

* Sat Sep 29 2009 Chris Frey <cdfrey@foursquare.net> 0.16-0
- version bump
- added new ppp chat script for KPN Nederland
- using new udev rules set
- added bjdwp and manpage, and removed some test-only programs
- added bash and zsh completion scripts
- added .desktop file and icon for barrybackup

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

