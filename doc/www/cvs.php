<? createHeader("Installing Barry"); ?>

<? include ("barry.inc"); ?>


<div class="subHeader">Dependencies</div>

<p>See the <? createLink("dependencies", "software dependencies"); ?> page for
detailed information on the software that Barry needs.</p>


<div class="subHeader">Getting the Source</div>

<p>You can get the source code by downloading the tarball from the
<a href="http://sourceforge.net/project/showfiles.php?group_id=153722">download page</a></li>...</p>

<p>... Or a better option is to download it directly from CVS.</p>

<p>Up to date instructions for connecting to Sourceforge CVS repositories
are available on the
<a href="http://sourceforge.net/cvs/?group_id=153722">CVS page</a>.  This
usually involves commands like this:
<pre>
	cvs -d:pserver:anonymous@barry.cvs.sourceforge.net:/cvsroot/barry login
	(press enter when it asks for password)
	cvs -d:pserver:anonymous@barry.cvs.sourceforge.net:/cvsroot/barry co -P barry
</pre>
</p>

<p>This will place the Barry sources in the barry directory.  To update
your source tree periodically, do the following:
<pre>
	cd barry
	./buildgen.sh cleanall      (optional)
	cvs update -Pd
</pre>
</p>



<div class="subHeader">Preparing CVS Sources for Configure</div>

<p>If you're using the CVS tree, you'll need to build the usual ./configure
script before you can proceed.  To do this, you will need autoconf, automake,
and libtool as stated on the dependencies page.  The correct sequence
of commands to build ./configure is already stored in the ./buildgen.sh
shell script in the root level directory of the Barry tree.
<pre>
	cd barry
	./buildgen.sh
</pre>




<div class="subHeader">Building the Source</div>

<p>At this point, or if you are using a source tarball, building Barry
is a matter of the common set of commands:
<pre>
	./configure
	make
	make install          (possibly as root)
</pre>
</p>

<p> The top level configure script has two options:
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
within the src/ directory.  The resulting files will be in doc/doxygen/html/.
Doxygen 1.4.5 has been used to do this, but presumably more recent versions
will work as well.</p>



<div class="subHeader">Build Everything!</div>

<p>An example that will build everything, including the Boost features in
btool:
<pre>
	cd barry
	./buildgen.sh cleanall         (this will make a pristine tree)
	./buildgen.sh                  (this creates configure)
	./configure --with-boost=/usr --enable-gui --enable-opensync-plugin
	make
	make install
	cd src && doxygen
</pre>

<p>This will give you a set of command line tools (bcharge, btool, breset,
bidentify, pppob), as well as the backup GUI (barrybackup), and will install the
opensync plugin into the system directory for opensync plugins (usually
/usr/lib/opensync/plugins).  Available man pages are also installed.</p>

<p>You can use 'btool' to explore your device from the command line.
Use the -h switch for help on its command line options.  Some good ones to
start with are <code>-l</code> to list the devices found, and <code>-t</code>
to list the Database Database.</p>


<div class="subHeader">Configure udev to Run bcharge Automatically</div>

<p>The makefiles do not install udev rules automatically.  There are sample
udev rules files in the udev/ directory.  For a Debian system, copy the
udev/10-blackberry.rules.Debian file to /etc/udev/rules.d/10-blackberry.rules,
and copy the file modprobe/blacklist-berry_charge to
/etc/modprobe.d/blacklist-berry_charge.
<pre>
	cd barry
	(become root)
	cp udev/10-blackberry.rules.Debian /etc/udev/rules.d/10-blackberry.rules
	cp modprobe/blacklist-berry_charge /etc/modprobe.d/blacklist-berry_charge
</pre>
</p>

<p>Make sure that bcharge was installed to /usr/sbin.  If you used a different
--prefix option on the ./configure command line, you will need to update
your 10-blackberry.rules file to match.</p>



<div class="subHeader">Building Barry RPMs from CVS</div>

<p> Paul Dugas reports on the mailing list that he uses the following
steps for building RPMs from CVS:</p>

<pre>
	$ cd ~/work
	$ cvs ... login
	$ cvs ... co barry
	$ cd barry
	$ ./buildgen.sh
	$ ./configure
	$ make dist
	$ rpmbuild -tb barry-0.12.tar.gz --with gui --with opensync

	I prefer running rpmbuild from the tarball as it's typically the way
	non-developers would build them.  I have ~/.rpmmacros setting %_topdir
	to %(echo $HOME)/.rpmbuild so the RPM building can run as me and not
	root.  The resulting RPMs end up in ~/.rpmbuild/RPMS/x86_64.
</pre>


<div class="subHeader">Building Barry DEBs from CVS</div>

<p>Once you have ./configure generated as detailed above, you can
create Debian-style binary packages for your system by running the
following:
<pre>
	cd barry
	fakeroot -- debian/rules binary
</pre>
</p>

