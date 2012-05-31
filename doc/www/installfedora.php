<? include ("barry.inc"); ?>

<? createHeader("Installing Barry on Fedora"); ?>

<? createSubHeader("The Setup"); ?>

<p>Starting with the 0.18.x version series, Barry and OpenSync binary
packages are available via yum.

<p>To install the latest version of Barry onto Fedora 16, create a
new file (for example, barry.repo) in your /etc/yum.repos.d/ directory
with the following contents:

<hr/>
<pre>
[Barry]
name=Barry for Fedora $releasever - $basearch
failovermethod=priority
baseurl=http://download.barry.netdirect.ca/barry-latest/dists/fedora$releasever/i686/
enabled=1
metadata_expire=7d
gpgcheck=1
gpgkey=http://download.barry.netdirect.ca/barry-latest/dists/fedora$releasever/i686/RPM-GPG-KEY-barry

[Barry-source]
name=Barry sources for Fedora $releasever - Source
failovermethod=priority
baseurl=http://download.barry.netdirect.ca/barry-latest/dists/fedora$releasever/source-i686/
enabled=1
metadata_expire=7d
gpgcheck=1
gpgkey=http://download.barry.netdirect.ca/barry-latest/dists/fedora$releasever/source-i686/RPM-GPG-KEY-barry
</pre>
<hr/>

<p>If you only want to use a specific version of Barry, you can change
the URLs from "barry-latest" to, for example, "barry-0.18.3".
Check out the latest available versions at the
<a href="http://sourceforge.net/projects/barry/files/barry/">Sourceforge
file list page</a>.

<p>Currently, only Fedora 14 and 16 are supported.


<? createSubHeader("The Packages"); ?>

<p>Barry is split up into multiple binary packages.  For example,
if you want the GUI backup program, you will also need the Barry library.

<p>For most non-development systems, you will need:
<ul>
	<li>libbarry0</li>
	<li>barry-util</li>
	<li>barry-gui</li>
	<li>barry-desktop</li>
</ul>
</p>

<p>For syncing, you will also need one of the available versions of
OpenSync (either 0.2x, or 0.39).  Note that OpenSync 0.39 is sometimes
called 0.4x.

<p>The 0.2x series includes the following packages:

<ul>
	<li>libopensync</li>
	<li>msynctool</li>
	<li>barry-opensync</li>
	<li>opensync0-plugin-evolution</li>
</ul>

<p>The 0.4x series includes the following packages:

<ul>
	<li>libopensync1</li>
	<li>osynctool</li>
	<li>barry-opensync4x</li>
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
	<li>libbarry-devel</li>
	<li>libopensync-devel or libopensync1-devel</li>
</ul>




<? createSubHeader("Removal"); ?>

<p>Everything depends on the following 3 packages: libbarry0, libopensync,
and/or libopensync1.  Remove them, and the rest will follow.

