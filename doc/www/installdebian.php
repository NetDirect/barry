<? include ("barry.inc"); ?>

<? createHeader("Installing Barry on Debian or Ubuntu or Mint"); ?>

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

<p>Everything depends on the following 3 packages: libbarry18, libopensync0,
and/or libopensync1.  Remove them, and the rest will follow.

