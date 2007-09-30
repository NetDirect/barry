<? createHeader("barry - BlackBerry &trade; synchronization for Linux"); ?>

<? include ("barry.inc"); ?>

<div class="subHeader">Overview</div>

<p>Linux users who also use a BlackBerry &trade; now have an option for
managing their BlackBerry directly from Linux.</p>

<p>Barry is an Open Source application that will allow synchronization,
backup, restore and program management for BlackBerry &trade; devices.
Barry is primarily developed on Linux, but is intended as a cross platform
library and application set, targetting Linux, BSD, 32/64bit, and big/little
endian systems.  You can find the source code at the
<a href="http://sourceforge.net/projects/barry/">Sourceforge project page</a>
</p>

<div class="subHeader">Getting Started</div>

<p>Barry comes in source and binary package format.  You can find the latest
downloads <a href="http://sourceforge.net/project/showfiles.php?group_id=153722">here</a>.</p>

<p>Before you start, you should check whether your system meets the following
requirements:</p>

<ul>
	<li>A kernel with the berry_charge module disabled</li>
	<li>If you are running a Fedora system, make sure you have the
		ConsoleKit package installed.  This will allow you to
		access the Blackberry without root privileges.</li>
</ul>

<p>See the <? createLink("requirements", "requirements"); ?> document for
more details on the above requirements.</p>

<p>See the <? createLink("install", "installation"); ?> page for
instructions on installing Barry from source.</p>

<div class="subHeader">Help Wanted</div>

<p>We can always use help in testing and developing extra features.
If you have any trouble getting Barry to work, please contact us on
the <a href="http://sourceforge.net/mail/?group_id=153722">mailing list</a>.
</p>

<p>There are dozens of databases that need to be documented and supported
in the library.  If you have a BlackBerry &trade; device and are interested
in helping decipher data, we have a
<? createLink("hacking", "barry hacking document"); ?> that will help you.
</p>

<p>We are also looking for help in many areas: general documentation,
howto documents, Python SWIG wrappers, or Python GUIs.  Feel free to
ask on the mailing list for more information.</p>

<div class="subHeader">Design</div>

<p>If you are interested in helping out or just interested in how it works,
check out our <? createLink("design", "design notes"); ?>.

<div class="subHeader">Roadmap</div>

<p>We have put together a document that describes where we are and where we
want to take barry. Check out our <? createLink("roadmap", "roadmap"); ?>
to see what's happening.</p>

