<? createHeader("barry - BlackBerry &trade; synchronization for Linux"); ?>

<? include ("barry.inc"); ?>

<div class="subHeader">Overview</div>

<p>Linux users who also use a BlackBerry &trade; now have an option for
managing their BlackBerry directly from Linux.</p>

<p>Barry is an Open Source application that will allow synchronization,
backup, restore and program management for BlackBerry &trade; devices.
Barry is primarily developed on Linux, but is intended as a cross platform
library and application set, targetting Linux, BSD, 32/64bit, and big/little
endian systems.
</p>

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
	<li> use the Blackberry as a modem (experimental) </li>
</ul>
</p>

<div class="subHeader">Status</div>

<p>
<ul>
	<li>Latest release: <b>0.13</b></li>
	<li>Download source and binary packages from
		<a href="http://sourceforge.net/project/showfiles.php?group_id=153722">Sourceforge</a></li>
	<li>General <a href="http://sourceforge.net/projects/barry/">Sourceforge project page</a></li>
</ul>
</p>

<div class="subHeader">Getting Started</div>

<p>How do I...
<ul>
	<li><? createLink("install", "Install Barry from binary packages"); ?></li>
	<li><? createLink("cvs", "Install Barry from CVS"); ?></li>
	<li><? createLink("backups", "Make backups of my Blackberry data"); ?></li>
	<li><? createLink("sync", "Sync my Blackberry with Evolution"); ?></li>
	<li><? createLink("modem", "Use my Blackberry as a modem to surf the net"); ?></li>
	<li><? createLink("troubleshooting", "Find troubleshooting help"); ?></li>
	<li><? createLink("bugs", "Report a bug"); ?></li>
	<li><? createLink("patches", "Submit a patch"); ?></li>
	<li><? createLink("contact", "Contact the developers"); ?></li>
</ul>
</p>

<p>Some helpful lists:
<ul>
	<li><? createLink("requirements", "System requirements"); ?></li>
	<li><? createLink("dependencies", "Software dependencies"); ?></li>
</ul>
</p>


<div class="subHeader">How to Help</div>

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

<div class="subHeader">Design</div>

<p>If you are interested in helping out or just interested in how it works,
check out our <? createLink("design", "design notes"); ?>.

<div class="subHeader">Roadmap</div>

<p>We have put together a document that describes where we are and where we
want to take Barry. Check out our <? createLink("roadmap", "roadmap"); ?>
to see what's happening.</p>

