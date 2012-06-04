<? include ("barry.inc"); ?>

<? createHeader("barry - Software dependencies"); ?>

<? createSubHeader("System Specific"); ?>

<p>Fedora systems:
<ul>
	<li><b>ConsoleKit</b> - required for accessing the
		Blackberry without root privileges.</li>
</ul>
</p>

<p>Debian systems:
<ul>
	<li><b>fakeroot</b> - optional program to assist building your own
		Debian binary packages without root privileges</li>
	<li><b>cdbs</b> and <b>debhelper</b> - required to build binary
		packages</li>
	<li><b>apt-utils</b> - optional, if you wish to create a binary package
		repository</li>
</ul>
</p>

<p>OpenBSD systems:
<ul>
	<li><b>Uberry</b> - the uberry kernel module conflicts with the ugen
		interface that libusb uses to talk to the device.  To work
		around this, you will need to boot your kernel with "boot -c"
		and disable the uberry module.  Suggestions for better ways
		to work around this conflict are welcome.</li>
</ul>
</p>

<p>Mac OSX systems:
<ul>
	<li><b>gettext</b> - the default gettext autoconf scripts do not
		always do a very good job of detecting gettext and libintl
		libraries on the Mac.  If it cannot find libintl, it will
		automatically disable NLS support.  If this is important
		to you, add --with-libintl-prefix=/opt/local/ to the
		configure command line when building from source.</li>
</ul>
</p>

<? createSubHeader("Master List of Dependencies"); ?>

<p>The following list contains all software that Barry depends on, the
minimal version required, and the reason for its dependency.
Some dependencies are only needed for building from source, and
some are only needed for building from git.

<p><b>Note:</b> In the Barry source tree, under maintainer/depscripts/ you will
find a number of distro-specific scripts which contain the appropriate
apt-get or yum command lines to install all these dependencies for you.
Review the script closest to your system.  It may save you time.

<ul>
	<li><b>git</b> - for retrieving source from repositories</li>
	<li><b>C and C++ compilers</b> - 4.1.x or higher, for the tr1
		includes (source build)</li>
	<li><b>ccache</b> - completely optional, but very useful if you
		plan on compiling repeatedly</li>
	<li><b>pkg-config</b> - source build only, so configure can
		autodetect library locations) </li>
	<li><b>libusb</b> - both versions 0.1.x and 1.x are supported and
		autodetected by the configure script.  Found at
		<a href="http://www.libusb.org/">http://www.libusb.org/</a></li>
	<li><b>pthread</b>
	<li><b>boost</b> - optional dependency for serialization support.
		Use version 1.33 or higher
		<a href="http://www.boost.org/">http://www.boost.org</a></li>
	<li><b>automake</b> version 1.9 (git builds only) </li>
	<li><b>autoconf</b> version 2.61 (git builds only) </li>
	<li><b>autoconf-archive</b></li>
	<li><b>libtool</b> version 1.5.22 (git builds only) </li>
	<li><b>autopoint</b> on some systems, this is a separate package, yet
		on others, it is part of gettext (git builds only) </li>
	<li><b>doxygen</b> suggested version 1.5.6, only for building API
		documentation</li>
	<li><b>gtkmm, glademm, glibmm</b> - version 2.4 or compatible - C++
		versions of the GTK libraries, which are needed for the
		barrybackup GUI</li>
	<li><b>libtar</b> - needed for libbarrybackup, and therefore the
		(barrybackup GUI as well</li>
	<li><b>zlib</b> - needed for CRC32 checksums in library COD file
		support, and also used by the barrybackup GUI to compress
		backup files</li>
	<li><b>libopensync</b> version 0.2x or version 0.39 devel</li>
	<li><b>sqlite, glib2, libxml2</b> - needed for syncing, required by
		OpenSync</li>
	<li><b>libfuse</b> - version 2.5 or higher (optional)</li>
	<li><b>libiconv</b> - needed for international charset conversions.
		Most Linux distros have this as part of libc.  If you are
		using another OS such as FreeBSD, you'll have to install
		this separately.</li>
	<li><b>libxml++</b> - version 2.6 for the Desktop</li>
	<li><b>gettext</b> - needed for the iconv.m4 file, on some systems,
		when building from git to generate configure</li>
	<li><b>php5</b> - optional, needed for generating static HTML
		documentation (git builds only) </li>
	<li><b>rpmdevtools</b> and <b>rpm-build</b> - if building RPMs
		yourself</li>
	<li><b>wxWidgets</b> - version 2.8, required by the Desktop GUI</li>
	<li><b>libgcal</b> - version 0.9.6 or higher, if building the
		Barry Desktop</li>
	<li><b>libSDL</b> - optional, needed by the bwatch program</li>
	<li><b>evolution-data-server</b> and <b>libebook, libedata*</b>
		libraries - needed by the OpenSync evolution plugin</li>
	<li><b>gksu</b> or <b>beesu</b> or equivalent GUI sudo - needed by
		the Desktop for modem functionality</li>
</ul>
</p>

<? createSubHeader("The Case of the Broken libtar"); ?>

<p>Well meaning people, in efforts to port the libtar examples to 64-bit
systems have introduced a bug that causes libtar to mismatch standard
read() and write() function call prototypes.</p>

<p>This bug has been seen in the Mandriva, ArchLinux, and Gentoo distros.
Depending on your system, and how up to date it is, it may already have been
fixed.</p>

<p>The curious can read more about this bug
<a href="http://www.mail-archive.com/barry-devel@lists.sourceforge.net/msg00746.html">here</a> and
<a href="http://www.mail-archive.com/barry-devel@lists.sourceforge.net/msg00754.html">here</a>.</p>

<p>This bug has been fixed for a long time, or never existed, on distros
like Fedora or Debian, and you can probably grab sources from those
distros if you really need them.  If you run into this problem and
require help to solve it, please email the <? createLink("contact",
"mailing list"); ?>.

<p>The latest version of libtar can be found <a href="http://repo.or.cz/w/libtar.git">here</a>.


<? createSubHeader("Dependency Packages for Common Distros"); ?>

<p>I used to maintain lists of packages for common distros here, but
it is much more useful to have a script that just works.

<p>In the <a href="http://repo.or.cz/w/barry.git/tree/HEAD:/maintainer/depscripts">Barry source tree</a>, under maintainer/depscripts/ you will
find a number of distro-specific scripts which contain the appropriate
apt-get or yum command lines to install all these dependencies for you.
Review the script closest to your system.  It may save you time.

