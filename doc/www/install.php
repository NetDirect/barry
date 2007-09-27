<? createHeader("Installing Barry"); ?>

<? include ("barry.inc"); ?>


<div class="subHeader">Introduction</div>

<p>Barry is a GPL C++ library to interface with USB BlackBerry handheld devices
on Linux.  This is one of Net Direct Inc.'s (<a href="http://www.netdirect.ca/">http://www.netdirect.ca</a>)
open source projects.</p>

<p>The SourceForge project page can be found at:<br>
<ul>
        <a href="http://sourceforge.net/projects/barry/">http://sourceforge.net/projects/barry/</a>
</ul>

<p>Barry is currently in early development, but is reaching stages of
usefulness.  For example, it is possible to:

<ul>
	<li> retrieve Address Book, Email, Calendar, Service Book, Memos,
		Tasks, PIN Messages, Saved Email, and Folders </li>
	<li> export Address Book contacts in text or LDAP LDIF format </li>
	<li> make full data backups and restores of your device using
		a GUI</li>
	<li> synchronize contacts and calendar items using the
		<a href="http://www.opensync.org/">OpenSync</a>
		framework (experimental) </li>
</ul>
</p>


<div class="subHeader">Dependencies</div>

<p>You need:
<ul>
	<li> pkg-config (for configure to autodetect library locations) </li>
	<li> libusb, stable (0.1.x) <a href="http://libusb.sourceforge.net">http://libusb.sourceforge.net/</a> </li>
	<li> openssl (needed for password hashing) <a href="http://www.openssl.org/">http://www.openssl.org/</a></li>
	<li> boost 1.33 (optional, needed for the serialization library, which you need if you want to save downloads for later uploads to the device, using btool) <a href="http://www.boost.org/">http://www.boost.org</a></li>
</ul>

<p>If building directly from CVS instead of from a release tarball, you
also need:

<ul>
	<li> automake (1.9) </li>
	<li> autoconf (2.61) </li>
</ul>

<p> If you want to generate doxygen documentation, run 'doxygen' from
within the src/ directory.  The resulting files will be in doc/doxygen/html/.
This has been tested with Doxygen 1.4.5</p>

<p> The top level configure script has two options:
<ul>
	<li> --enable-gui </li>
	<li> --enable-opensync-plugin </li>
</ul>

<p>Each option will recurse into the gui/ (Backup application) and
opensync-plugin/ directories respectively, and build the subprojects
located there automatically.  For each of these projects, they have
additional dependencies:

<p><b>GUI:</b></p>
<ul>
	<li> gtkmm 2.4 - C++ GTK+ wrapper library </li>
	<li> glademm 2.4 - C++ glade wrapper library </li>
	<li> glibmm 2.4 - C++ glib wrapper library </li>
	<li> libtar 1.2.x - TAR file library </li>
	<li> zlib - compression library </li>
	<li> libbarry itself </li>
</ul>

<p><b>Opensync plugin:</b></p>
<ul>
	<li> libopensync-0.22 - OpenSync framework library </li>
	<li> sqlite, glib2, and libxml2 - Required by OpenSync </li>
	<li> libbarry itself </li>
</ul>


<div class="subHeader">Building Barry</div>

<p> If starting from CVS, run <code>buildgen.sh</code> with no arguments.

<p> Assuming all dependencies are available, the following
commands will build and install Barry on your system, with all available
compenents.  The doxygen docs will be placed in doc/doxygen/html/.</p>

<pre>
	./configure --with-boost=/usr --enable-gui --enable-opensync-plugin
	make
	make install
	cd src && doxygen
</pre>

<p>This will give you a set of command line tools (bcharge, btool, breset),
as well as the backup GUI (barrybackup), and will install the opensync
plugin into the system directory for opensync plugins (usually
/usr/lib/opensync/plugins).  Man pages are available for bcharge and
btool.</p>

<p>You can use 'btool' to explore your device from the command line.
Use the -h switch for help on its command line options.  Some good ones to
start with are <code>-l</code> to list the devices found, and <code>-t</code>
to list the Database Database.</p>



<p>Enjoy!</p>


<div class="subHeader">Dependency Packages for Common Distros</div>

<p>The following is a list of packages you'll need to install to build Barry,
if you are using one of the below common distributions.  Other distributions
should have similar package names.

<ul>

<p><b>Fedora 5 and 6:</b></p>
<p>Use the yum package manager to install the following:
<ul>
	<li> pkgconfig </li>
	<li> libusb-devel </li>
	<li> openssl-devel </li>
	<li> boost-devel (optional) </li>
	<li> libtar (libtar-devel on Fedora 6) </li>
	<li> gtkmm24-devel </li>
	<li> glibmm24-devel </li>
	<li> libglademm24-devel </li>
	<li> zlib-devel </li>
</ul>

<p><b>Debian stable:</b></p>
<p>Use the apt-get package manager to install the following:
<ul>
	<li> pkg-config </li>
	<li> libusb-dev </li>
	<li> libssl-dev </li>
	<li> libboost-serialization-dev </li>
	<li> libtar-dev </li>
	<li> libgtkmm-2.4-dev </li>
	<li> libglibmm-2.4-dev </li>
	<li> libglademm-2.4-dev </li>
	<li> zlib1g-dev </li>
</ul>

</ul>

