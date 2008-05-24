<? createHeader("barry - Software dependencies"); ?>

<? include ("barry.inc"); ?>

<div class="subHeader">Distro Specific</div>

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
</ul>
</p>

<div class="subHeader">Master List of Dependencies</div>

<p>The following list contains all software that Barry depends on, and
the reason for it.  Some are only needed for building the source, and
some are only needed for building CVS.

<ul>
	<li><b>C and C++ compilers</b> - 4.1.x or higher, for the tr1 includes (source build)</li>
	<li><b>pkg-config</b> (source build: so configure can autodetect library locations) </li>
	<li><b>libusb, stable (0.1.x)</b> - found at <a href="http://libusb.sourceforge.net">http://libusb.sourceforge.net/</a> </li>
	<li><b>openssl</b> (needed for password hashing) - found at <a href="http://www.openssl.org/">http://www.openssl.org/</a></li>
	<li><b>pthread</b>
	<li><b>boost</b> version 1.33 or higher (optional, needed for the serialization library, which you need if you want to save downloads for later uploads to the device, using btool) <a href="http://www.boost.org/">http://www.boost.org</a></li>
	<li><b>automake</b> version 1.9 (CVS builds only) </li>
	<li><b>autoconf</b> version 2.61 (CVS builds only) </li>
	<li><b>doxygen</b> suggested version 1.4.5, only for building API documentation</li>
	<li><b>gtkmm, glademm, glibmm</b> C++ versions of the GTK libraries (needed for the barrybackup GUI)</li>
	<li><b>libtar</b> (barrybackup GUI) </li>
	<li><b>zlib</b> (barrybackup GUI) </li>
	<li><b>libopensync</b> version 0.22 <i>only</i> (needed for syncing) </li>
	<li><b>sqlite, glib2, libxml2</b> (needed for syncing, required by OpenSync) </li>
</ul>
</p>

<div class="subHeader">The Case of the Broken libtar</div>

<p>Well meaning people, in efforts to port the libtar examples to 64-bit
systems have introduced a bug that causes libtar to mismatch standard
read() and write() function call prototypes.</p>

<p>This bug has been seen in the Mandriva, ArchLinux, and Gentoo distros.
Depending on your system, and how up to date it is, it may already have been
fixed.</p>

<p>The curious can read more about this bug
<a href="http://sourceforge.net/mailarchive/message.php?msg_id=20070803200729.GA7068%40foursquare.net">here</a> and
<a href="http://sourceforge.net/mailarchive/message.php?msg_name=20080417204336.GA15423%40foursquare.net">here</a>.</p>

<p>Of course, you probably don't want to read the intricate details of
distro bugs.  You just want it to work!  For such systems, I usually
grab the libtar source RPM package from
<a href="ftp://ftp.nrc.ca/pub/systems/linux/redhat/fedora/linux/releases/8/Everything/source/SRPMS/libtar-1.2.11-9.fc8.src.rpm">here</a> and then run:
<pre>
rpm -i libtar-1.2.11-9.fc8.src.rpm
(become root)
cd /usr/src/packages/SPECS
rpmbuild -ba libtar.spec
rpm -i ../RPMS/*/libtar*rpm
</pre>
</p>


<div class="subHeader">Dependency Packages for Common Distros</div>

<p>The following is a list of packages you'll need to install to build Barry
from source, if you are using one of the below common distributions.  Other
distributions should have similar package names.

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

