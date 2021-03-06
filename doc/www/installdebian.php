<? include ("barry.inc"); ?>

<? createHeader("Installing Barry on Debian or Ubuntu or Mint"); ?>

<? createSubHeader("Warning"); ?>

<p>There are two recent non-Barry bugs popping up that are making this
install process just a little bit more tricky for Debian/Ubuntu/Mint users.

<p>One is a bug in apt-get, which causes downloads to stop with an error
if one of the HTTP header lines is longer than 360 characters.  Normally,
this bug is not encountered, but it is encountered on sourceforge.net.

<p>The Barry repositories on netdirect are setup to redirect all large binary
file downloads to the appropriate file on sourceforge.  The apt repo metadata,
such as signatures and checksums are downloaded from netdirect's server
itself, and the binary files are checked against them.

<p>Sourceforge.net includes a cookie in one HTTP header line that is
about 550 characters long.  This has been causing trouble for a number
of users.

<p>Fortunately, the 360 character limit should be removed eventually,
as this has been fixed upstream, according to this
<a href="http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=658346">Debian bug report</a>.

<? createSubHeader("Workaround"); ?>

<p>If, after following the usual steps below, you encounter the
"360 char limit" error, there is a workaround.  Go to
<a href="http://sourceforge.net/projects/barry/files/barry/barry-0.18.3/">the Barry file download page</a>
and navigate to your distro and platform.  For example, if you are using
squeeze on 64bit, navigate through: bmbuild/dists/squeeze/main/binary-amd64.
Then download all the .deb files into your /var/cache/apt/archives/ directory,
and install again. Apt will notice the existing files, check them, and
resolve dependencies for you.

<p>Sorry for the inconvenience.  We now return to the usual documentation...


<? createSubHeader("The Setup"); ?>

<p>Starting with the 0.18.x version series, Barry and OpenSync binary
packages are available via apt-get.

<p>To install the latest version of Barry onto Debian Squeeze, add
the following line to your /etc/apt/sources.list file:

<pre>
deb http://download.barry.netdirect.ca/barry-latest/ squeeze main
</pre>

<p>If you only want to use a specific version of Barry, you can change
the URL from "barry-latest" to, for example, "barry-0.18.3".
Check out the latest available versions at the
<a href="http://sourceforge.net/projects/barry/files/barry/">Sourceforge
file list page</a>.

<p>There are multiple versions of Ubuntu available.  Replace the word
"squeeze" above with one of the following:

<ul>
	<li>ubuntu1004</li>
	<li>ubuntu1104</li>
	<li>ubuntu1110</li>
	<li>ubuntu1204</li>
</ul>

<p>For Mint, pick the version of Debian or Ubuntu that your version of
Mint is based on.

<p>Finally, you will need to update your apt keyring with the following
key:

<pre>
	82DE DE11 74C1 0EA7 C55D  5679 3B52 35AE B6C2 250E
</pre>

<p>One way to fetch this key is by using gpg:

<pre>
	gpg --keyserver pgp.mit.edu --recv-key B6C2250E
	gpg --armor --export B6C2250E &gt; barry.key
	apt-key add barry.key
</pre>


<? createSubHeader("The Packages"); ?>

<p>Barry is split up into multiple binary packages.  For example,
if you want the GUI backup program, you will also need the Barry library.

<p>For most non-development systems, you will need:
<ul>
	<li>libbarry18</li>
	<li>barry-util</li>
	<li>barrybackup-gui</li>
	<li>barrydesktop</li>
</ul>
</p>

<p>For syncing, you will also need one of the available versions of
OpenSync (either 0.2x, or 0.39).  Note that OpenSync 0.39 is sometimes
called 0.4x.

<p>The 0.2x series includes the following packages:

<ul>
	<li>libopensync0</li>
	<li>msynctool</li>
	<li>opensync0-plugin-barry</li>
	<li>opensync0-plugin-evolution</li>
	<li>opensync0-plugin-file</li>
	<li>opensync0-plugin-kdepim</li>
</ul>

<p>The 0.4x series includes the following packages:

<ul>
	<li>libopensync1</li>
	<li>osynctool</li>
	<li>opensync1-plugin-barry</li>
	<li>opensync1-plugin-evolution</li>
	<li>opensync1-plugin-evolution3</li>
	<li>opensync1-plugin-file</li>
	<li>opensync1-plugin-vformat</li>
	<li>opensync1-plugin-xmlformat</li>
</ul>

<p>There are 3 convenience packages: binarymeta2x, binarymeta4x, and
binarymeta-everything.  Install these if you want to install everything.

<p>For development systems, you will need the following additional
packages:
<ul>
	<li>libbarry-dev</li>
</ul>


<? createSubHeader("Removal"); ?>

<p>Everything exists in or depends on the following 4 packages: libbarry18,
libopensync0, libopensync1, and barry-doc.  Remove them, and the rest will
follow.

