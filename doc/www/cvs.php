<? include ("barry.inc"); ?>

<? createHeader("Installing Barry"); ?>

<? createSubHeader("Dependencies"); ?>

<p>See the <? createLink("dependencies", "software dependencies"); ?> page for
detailed information on the software that Barry needs.</p>


<? createSubHeader("Getting the Source"); ?>

<p>There are 2 ways to get the source code:
<ul>
	<li>download the release tarball from the
		<a href="http://sourceforge.net/project/showfiles.php?group_id=153722">download page</a></li>
	<li>download the development tree
		<a href="http://repo.or.cz/w/barry.git">using git</a></li>
</ul>
</p>


<? createSubHeader("Using The Tarball Release"); ?>

<p>There are multiple source packages available on the Sourceforge download page.
The main tarball is always the tar.bz2 package.  This contains everything in
git, including pre-built configure scripts and website documentation.</p>

<p>As an alternative, the tar.gz packages are Debian-specific.  This comes as
an "orig" tarball, with the debian/ subdirectory contained in the diff.gz file.
A .dsc file is included as well, for ease of Debian package building.  These
files can be expanded into a Debian style source tree with the following
command, for example:
<pre>
	dpkg-source -x barry_0.15-0.dsc
</pre>
</p>

<p>Once the tar.gz and diff.gz files are combined, you will have a tree
identical to what comes with the tar.bz2 package.</p>

<p>See <i>Building the Source</i> below for instructions on building the
resulting tree.</p>

<p>Alternatively, there are rpm source packages, in the form of src.rpm files.
These packages can be installed and built as usual, using the rpm and rpmbuild
commands.</p>


<? createSubHeader("Using git"); ?>

<p>The same development tree is also available via git, and can be browsed
on the web at the <a href="http://repo.or.cz/w/barry.git">Barry git page</a>.
You can clone the repository like this:

<pre>
	git clone git://repo.or.cz/barry.git barry
</pre>
</p>

<p>This will place the Barry sources in the barry directory.  To update
your source tree periodically, do the following:

<pre>
	cd barry
	./buildgen.sh cleanall      (optional)
	git checkout master
	git pull origin
</pre>
</p>





<? createSubHeader("Preparing Development Sources for Configure"); ?>

<p>If you're using a development tree, you'll need to build the usual
./configure script before you can proceed.  To do this, you will need
autoconf, automake, and libtool as stated on the dependencies page.
The correct sequence of commands to build ./configure is already stored
in the ./buildgen.sh shell script in the root level directory of the
Barry tree.

<pre>
	cd barry
	./buildgen.sh
</pre>




<? createSubHeader("Building the Source"); ?>

<p>At this point, or if you are using a source tarball, building Barry
is a matter of the common set of commands:
<pre>
	./configure
	make
	make install          (possibly as root)
</pre>
</p>

<p> The top level configure script has two sub-package options:
<ul>
	<li> --enable-gui </li>
	<li> --enable-opensync-plugin </li>
</ul>
</p>

<p>Each option will recurse into the gui/ (Backup application) and
opensync-plugin/ directories respectively, and build the subprojects
located there automatically.  Make sure you have the needed software
dependencies installed beforehand.</p>

<p> If you want to generate doxygen documentation, run 'doxygen' from
the root source directory.  The resulting files will be in
doc/www/doxygen/html/.  Doxygen 1.5.6 has been used to do this,
but presumably more recent versions will work as well.</p>



<? createSubHeader("Build Everything!"); ?>

<p>An example that will build everything, including the Boost features in
btool:
<pre>
	cd barry
	./buildgen.sh cleanall         (this will make a pristine tree)
	./buildgen.sh                  (this creates configure)
	./configure --enable-boost --enable-gui --enable-opensync-plugin
	make
	make install
	doxygen
</pre>

<p>This will give you a set of command line tools (bcharge, btool, breset,
bidentify, bjavaloader, pppob), as well as the backup GUI (barrybackup),
and will install the opensync plugin into the system directory for opensync
plugins (usually /usr/lib/opensync/plugins).  Available man pages are also
installed.</p>

<p>You can use 'btool' to explore your device from the command line.
Use the -h switch for help on its command line options.  Some good ones to
start with are <code>-l</code> to list the devices found, and <code>-t</code>
to list the Database Database.</p>


<? createSubHeader("Configure udev to Run bcharge Automatically"); ?>

<p>The makefiles do not install udev rules automatically.  There are sample
udev rules files in the udev/ directory.  For a Debian system, copy the
udev/debian/10-blackberry.rules file to /etc/udev/rules.d/10-blackberry.rules,
and copy the file modprobe/blacklist-berry_charge to
/etc/modprobe.d/blacklist-berry_charge.
<pre>
	cd barry
	(become root)
	cp udev/debian/10-blackberry.rules /etc/udev/rules.d/10-blackberry.rules
	cp modprobe/blacklist-berry_charge /etc/modprobe.d/blacklist-berry_charge
</pre>
</p>

<p>Make sure that bcharge was installed to /usr/sbin.  If you used a different
--prefix option on the ./configure command line, you will need to update
your 10-blackberry.rules file to match.</p>


<? createSubHeader("Configure PPP chat scripts for your system"); ?>

<p>The source tree comes with sample PPP chat scripts for using your
Blackberry as a modem.  These sample scripts are located under ppp/ in
your source directory.
The binary packages install all the ppp options files under /etc/ppp/peers/
and all the chat scripts (with the *.chat extensions) under
/etc/chatscripts/.  These directories are important, since the pppd
program expects to find options files under peers/ and the options files
reference the chatscripts.</p>

<p>Copy the above samples to their appropriate directories to install
modem support for your system.  Make sure you have pppd installed as well.</p>

<p>If you install Barry in a location other than /usr, you will need
to edit the options files to correct the hard coded paths in these
files.  The files assume that pppob is located in /usr/sbin/pppob.</p>

<p>See the <? createLink("modem", "modem usage"); ?> page for more
information on using your Blackberry as a modem.</p>


<? createSubHeader("Building Barry RPMs from git"); ?>

<p> Paul Dugas reports on the mailing list that he uses the following
steps for building RPMs from git:</p>

<pre>
	$ cd ~/work
	$ git clone...
	$ cd barry
	$ ./buildgen.sh
	$ ./configure --enable-gui --enable-opensync-plugin
	$ make dist
	$ rpmbuild -tb barry-0.13.tar.gz

	I prefer running rpmbuild from the tarball as it's typically the way
	non-developers would build them.  I have ~/.rpmmacros setting %_topdir
	to %(echo $HOME)/.rpmbuild so the RPM building can run as me and not
	root.  The resulting RPMs end up in ~/.rpmbuild/RPMS/x86_64.
</pre>

<p> On an RPM based system, install <b>rpm-build</b> and <b>rpmdevtools</b>,
then run <b>rpmdev-setuptree</b> to create an "rpmbuild" directory in your
home directory. </p>


<? createSubHeader("Building Barry DEBs from Source"); ?>

<p>Once you have ./configure generated as detailed above, you can
create Debian-style binary packages for your system by running the
following:
<pre>
	cd barry
	fakeroot -- debian/rules binary
</pre>
</p>

