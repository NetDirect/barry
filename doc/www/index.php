<? include ("barry.inc"); ?>

<? createHeader("barry - BlackBerry &trade; synchronization for Linux"); ?>

<? createSubHeader("Overview"); ?>

<p>Linux users who also use a BlackBerry &trade; now have an option for
managing their BlackBerry directly from Linux.</p>

<p>Barry is an Open Source application that will provide synchronization,
backup, restore and program management for BlackBerry &trade; devices.
Barry is primarily developed on Linux, but is intended as a cross platform
library and application set, targeting Linux, BSD, 32/64bit, and big/little
endian systems.
</p>

<p>The Barry project began in October 2005 and has steadily added features
and polish to Blackberry usage on Linux ever since.  We were the first to
reverse engineer the battery charging handshake via USB.</p>

<p>Today, it is possible to:

<ul>
	<li> charge your Blackberry's battery from your USB port </li>
	<li> parse the following database records: Address Book, Email,
		Calendar, Service Book, Memos, Tasks, PIN Messages,
		Saved Email, Folders, Phone Call Logs, and SMS messages </li>
	<li> build the following database records: Address Book, Calendar,
		Memos, Tasks </li>
	<li> export Address Book contacts in text or LDAP LDIF format </li>
	<li> make full data backups and restores of your device using
		a GUI</li>
	<li> synchronize contacts and calendar items using the
		<a href="http://www.opensync.org/">OpenSync</a>
		framework</li>
	<li> use the Blackberry as a modem</li>
	<li> install and manage Java applications from the command line </li>
	<li> take screenshots of your device </li>
	<li> set the device time from the command line </li>
	<li> ... and more </li>
</ul>
</p>

<div class="subHeader">Status</div>

<p>
<ul>
	<li>Latest release: <b>0.16</b>, released on 2009/10/01</li>
	<li>License: GPL v2 or later</li>
	<li>Download official source and binary packages from
		<a href="http://sourceforge.net/project/showfiles.php?group_id=153722">Sourceforge</a></li>
	<li>General <a href="http://sourceforge.net/projects/barry/">Sourceforge project page</a></li>
	<li><a href="http://repo.or.cz/w/barry.git">Barry git repo</a></li>
	<li>Additional binary packages for the following distributions are available from various volunteers.  Note that these packages may already be available in your distro of choice, using the normal package managers.
		<ul>
			<li><a href="http://packages.debian.org/unstable/main/barry-util">Debian, in the unstable tree, packaged by Jose Carlos Garcia Sogo</a></li>
			<li><a href="http://download.fedora.redhat.com/pub/fedora/linux/releases/">Fedora</a> (packaged by Nathanael Noblet) and the related RPM <a href="http://pkgs.fedoraproject.org/gitweb/?p=barry.git">git repo</a></li>
			<li>Mandriva's RPM <a href="http://svn.mandriva.com/cgi-bin/viewvc.cgi/packages/cooker/barry/current/">SVN</a></li>
			<li><a href="https://launchpad.net/~doctormo/+archive/barry-snapshot">Ubuntu, packaged by Martin Owens</a></li>
		</ul>
	</li>
	<li>Known Issues:
	<ul>
		<li>International characters in calendar and contact records
			cause some devices to switch to a different low-level
			protocol, which Barry does not yet support.</li>
		<li>Restoring backups for some databases on newer Blackberries doesn't work (for example, on the 8120, 8700g)</li>
		<li>Syncing is not supported on Fedora Core 9 or 10, since they packaged the OpenSync 0.3x devel tree (experimental support for the latest opensync SVN tree is included with a new plugin).  This has been fixed in Fedora 11, as they returned to version 0.22.</li>
		<li>Accessing the database (such as during a backup) while copying files using the usb_storage kernel module may cause some Blackberries to spontaneously reboot</li>
	</ul>
	</li>
</ul>
</p>

<? createSubHeader("Getting Started"); ?>

<p>How do I...
<ul>
	<li><? createLink("install", "Install Barry from binary packages"); ?></li>
	<li><? createLink("cvs", "Compile Barry from tarball, CVS, or git"); ?></li>
	<li><? createLink("backups", "Make backups of my Blackberry data"); ?></li>
	<li><? createLink("sync", "Sync my Blackberry with Evolution"); ?></li>
	<li><? createLink("modem", "Use my Blackberry as a modem to surf the net"); ?></li>
	<li><? createLink("rawchannel", "Use Blackberry USB channels with Barry"); ?></li>
	<li><? createLink("troubleshooting", "Find troubleshooting help"); ?></li>
	<li><? createLink("bugs", "Report a bug"); ?></li>
	<li><? createLink("patches", "Submit a patch"); ?></li>
	<li><? createLink("contact", "Contact the developers"); ?></li>
</ul>
</p>

<p>Some helpful pages:
<ul>
	<li><? createLink("requirements", "System requirements"); ?></li>
	<li><? createLink("dependencies", "Software dependencies"); ?></li>
	<li><? createFileLink("doxygen/html/index.html", "Barry's Doxygen documentation"); ?></li>
	<li><? createLink("devices", "Device compatibility list"); ?></li>
</ul>
</p>


<? createSubHeader("How to Help"); ?>

<p>If you are a C++ programmer, grab the source from
<? createLink("cvs", "CVS"); ?> and take a look at the TODO file.
Post a message to the
<a href="http://sourceforge.net/mail/?group_id=153722">mailing list</a>
when you start working on any of the listed features, so you can connect
with other developers, and avoid duplicating effort.</p>

<p>If you are a Python programmer, contact the mailing list, since we
would like to create and test a Python interface to the Barry library.</p>

<p>If you are not a programmer, but have a Blackberry, we can always use
help in testing.  Install Barry on your system, and
<? createLink("bugs", "report any bugs"); ?> you find.</p>

<p>If you prefer writing documentation, grab the source from
<? createLink("cvs", "CVS"); ?> and look under the doc/www directory.
Documentation is currently in html form, as well as doxygen-generated
API documentation found in the comments of the source code itself.
Patches updating either set of documentation are welcome.</p>

<p>There are dozens of databases that need to be documented and supported
in the library.  If you have a BlackBerry &trade; device and are interested
in helping decipher data, we have a
<? createLink("hacking", "Barry hacking document"); ?> that will help you.
</p>

<p>If you are interested in the low level USB protocol, you can download,
or contribute, USB logs to the
<? createLink("logs", "USB capture log archive"); ?>.</p>


<? createSubHeader("Design"); ?>

<p>If you are interested in helping out or just interested in how it works,
check out our <? createLink("design", "design notes"); ?>.


<? createSubHeader("Roadmap"); ?>

<p>We have put together a document that describes where we are and where we
want to take Barry. Check out our <? createLink("roadmap", "roadmap"); ?>
to see what's happening.</p>


<? createSubHeader("External Links"); ?>

<p>Barry users and others have contributed documentation and have
put these howto's on the web.  Below is a list of some of these pages.
These sites are not associated with NetDirect, and some of the information
may be out of date, but they may still be helpful to new users.</P>

<p>
<ul>
	<li><a href="http://www.chipbennett.net/wordpress/index.php/2008/05/synchronizing-a-blackberry-in-linux/">Synchronizing a Blackberry In Linux</a>, by Chip Bennett (2008/05/31)</li>
	<li><a href="http://www.linux.com/feature/123251">Syncing your Blackberry on Linux</a>, by Joe Barr (2007/12/21)</li>
</ul>
<ul>
	<li><a href="http://www.progweb.com/modules/blackberry/index-en.html">How to use a Blackberry device with Linux</a>, by Nicolas Vivien (2009/03/06), and <a href="http://www.progweb.com/en/">his development blog</a></li>
	<li><a href="http://www.slashdev.ca/2008/04/03/blackberry-development-using-linux/">Blackberry development using Linux</a>, by Josh Kropf (2008/04/03)</li>
</ul>

<ul>
	<li><a href="http://off.net/cassis/protocol-description.html">The RIM Blackberry Serial Protocol</a>, by Phil Schwan, Mike Shaver, and Ian Goldberg, of the Cassis project</li>
	<li><a href="http://www.blackberryforums.com/blackberry-guides/2019-user-howto-use-blackberry-modem-laptop.html">HOWTO: Blackberry as Modem for Laptop</a>, by Mark Rejhon, (not Linux specific)</li>
</ul>
</p>

