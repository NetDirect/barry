<? createHeader("Using your BlackBerry as a USB Modem"); ?>

<? include ("barry.inc"); ?>

FIXME - add what to do if using binary packages

<div class="subHeader">Introduction</div>

<p>The Blackberry has the ability to act like a Hayes compatible modem,
complete with AT commands.</p>

<p>In order to make use of this feature you will need the Barry command line
tools installed on your system.  See the <? createLink("install", "Install"); ?>
page for more details.</p>

<p>You will need three components to connect to the internet:
<ul>
	<li> a working Barry install, version 0.13 or later </li>
	<li> a working pppd and matching kernel </li>
	<li> an options file and chatscript for your Blackberry provider </li>
</ul>
</p>


<div class="subHeader">If You Installed Barry's Binary Packages...</div>

<p>If you installed Barry's binary packages, specifically barry-util,
then default ppp options and chat scripts are installed automatically
for you.  Currently there are scripts available for the following
providers:
<ul>
	<li> Rogers </li>
	<li> Verizon </li>
</ul>
</p>

<p>For example, to run the default Rogers connection script, use the
following command as root:</p>
<pre>
	pppd call barry-rogers
</pre>

<p>If none of the default script work, see the Troubleshooting section
below.</p>


<div class="subHeader">If You Installed Barry From Source...</div>

<b>Configuring PPPD</b>

<p>Sample configurations are provided in the source tarball, and if you are
using a binary package, they should already be installed for you under
/etc/ppp/peers and (depending on your system) /etc/chatscripts.</p>

<p>Copy the desired options file to /etc/ppp/peers and edit the file,
making sure that the paths are referencing the correct files.
<ul>
	<li> <b>pty</b> - must point to the location you installed Barry's
			pppob program. </li>
	<li> <b>connect</b> - must use the correct chatscript </li>
</ul>
</p>


<b>Establishing a Connection</b>

<p>If you named your options file "barry-rogers", then to connect to the
internet, run:</p>
<pre>
	pppd call barry-rogers
</pre>

<p>Some systems may provide wrapper scripts for running pppd.  For example,
Debian uses "pon" and "poff".</p>



<div class="subHeader">Troubleshooting</div>

<p>If you run into trouble, the first task is to increase debug output
and to log it.  PPPD supports increased logging if you add the following
lines to the options file:</p>
<pre>
	debug
	nodetach
</pre>

<p>Barry's <b>pppob</b> program also supports verbose protocol logging.
Change your options file and edit the pppob command line.  For example,
if your pppob is installed in /usr/sbin, set the <b>pty</b> option to:</p>

<pre>
	pty "/usr/sbin/pppob -l /tmp/pppob.log -v"
</pre>

<p>Be sure to remove these options once you have your connection working,
as the above causes your entire internet traffic to be logged to the
output file.</p>

<p>If your provider is not yet supported by the sample options files,
you will need to do a little digging on your own.  If you are able to
tether your Blackberry using a Windows system, you can duplicate the
login information in your chat script.</p>

<p>You may also find example configurations for your provider on the
various Blackberry forums available online.</p>

<p>If you are still having trouble, please send an email to the
<a href="http://sourceforge.net/mail/?group_id=153722">mailing list</a>,
including:
<ul>
	<li> the version of Barry you are using </li>
	<li> the output from pppd </li>
	<li> the log output from pppob </li>
	<li> the ppp options file and chatscript you used </li>
</ul>
</p>

<p>The logs from pppob can be fairly large, so limit the size to a single
run.</p>

