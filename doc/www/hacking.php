<? createHeader("Hacking Barry"); ?>

<? include ("barry.inc"); ?>

<div class="subHeader">Introduction</div>


<p>Barry is a GPL C++ library to interface with USB BlackBerry handheld devices
on Linux.  This is one of Net Direct Inc.'s (<a href="http://www.netdirect.ca/">http://www.netdirect.ca</a>)
open source projects.</p>

<p>The SourceForge project page can be found at:<br>
<ul>
        <a href="http://sourceforge.net/projects/barry/">http://sourceforge.net/projects/barry/</a>
</ul>

<p>Barry is currently in early development, but is reaching stages of usefulness.
For example, it is possible to retrieve Address Book contact data, and export
it in text or LDAP LDIF format.</p>


<div class="subHeader">Installation:</div>

<p>You need:
<ul>
        libusb, devel branch    <a href="http://libusb.sourceforge.net">http://libusb.sourceforge.net/</a>
</ul>

<p>Check out the 2005/11/26 V1_0_DEVEL tag/branch from CVS, or download it
as a tarball from the Barry file download page.</p>

<p>Barry uses the asynchronous library calls, which is why the development
branch is used.  The libusb devel branch is well along in its development
cycle, so don't be afraid to play with it.</p>

<p>This version of Barry has been tested using the 2005/11/26 libusb CVS tree.</p>

<p>To build Barry:

<ul>
<li>Edit Makefile.conf to point to the proper location for libusb headers and library files
<li> Enter the src/ directory and type 'make'
<li> If you want to generate doxygen documentation, run 'doxygen' as well.  The resulting files will be in doc/doxygen/html/ This has been tested with Doxygen 1.4.5
</ul>

<p>This will give you a command line tool called 'btool'.  Use the -h
switch for help on its command line options.  Some good ones to start with
are -l to list the devices found, and -t to list the Database Database.</p>


<div class="subHeader">BlackBerry protocol</div>

<p>No BlackBerry-related protocol project would be complete without referencing
the fine documentation from the Cassis project, which tackled the earlier
serial protocol.  You can find this documentation at:

<ul>
<a href="http://off.net/cassis/protocol-description.html">http://off.net/cassis/protocol-description.html</a>
</ul>

<p>There were some major and minor differences found between the serial
protocol and the USB protocol.  Some of the new handheld devices use new
database record access commands, and in these cases the record format changes.
See the code for more detailed information.</p>

<p>Further documentation on the USB protocol is planned.  Stay tuned.</p>


<div class="subHeader">Playing with the protocol</div>

<p>The USB captures were performed on a Windows XP Pro system running UsbSnoop
from <a href="http://benoit.papillault.free.fr/usbsnoop/index.php">http://benoit.papillault.free.fr/usbsnoop/index.php</a></p>

<p>You can use the convo.awk and translate.cc tools to turn these very verbose
logs into something more manageable.  Other than the normal USB control
commands at the beginning of each conversation, it was found that only
USB Bulk Transfers were used.</p>

<p>The btool utility is at the stage where it can be used instead of UsbSnoop,
for database operations.  You can use the -v switch to turn on data packet
dumping, which will display the sent and received packets in canonical hex
format as btool talks to the device.  You can use this in combination with
the -d switch to capture new database records to reverse engineer.</p>

<p>If you reverse engineer some of the unimplemented packet formats, please
send patches and/or documentation to the mailing list!</p>

<p>See the Hacking file for more information on getting started reverse
engineering the protocol.</p>


<div class="subHeader">Some notes on code architecture</div>

<p>Lowest level:
<ul>
        Lowest level is the libusb software, currently using the DEVEL branch
</ul>

<p>USB layer:
<ul>
<table border=0>
<tr>
	<td valign=top>usbwrap.{h,cc}</td>
	<td valign=top>C++ wrapper for libusb</td>
</tr><tr>
	<td valign=top>data.{h,cc}</td>
	<td valign=top>C++ data class for buffer management and hex log file input and output</td>
</tr><tr>
	<td valign=top>connect.cc</td>
	<td valign=top>low level USB test program, capable of using data file scripts to talk to a device via bulk read/write</td>
</tr><tr>
	<td valign=top>debug.h</td>
	<td valign=top>general debugging output support</td>
</table>
</ul>


<p>Barry low level layer:
<ul>
<table border=0>
<tr>
	<td valign=top>protostructs.h</td>
	<td valign=top>low level, packed structs representing the USB protocol</td>
</tr><tr>
	<td valign=top>time.{h,cc}</td>
	<td valign=top>time conversions between 1900-based minutes and C's 1970-based time_t</td>
</table>
</ul>


<p>Barry API layer:
<ul>
<table border=0>
<tr>
	<td valign=top>base64.{h,cc}</td>
	<td valign=top>base64 encoding and decoding (for LDIF)</td>
</tr><tr>
	<td valign=top>builder.h</td>
	<td valign=top>C++ virtual wrappers to connect record and controller in a generic way</td>
</tr><tr>
	<td valign=top>error.{h,cc}</td>
	<td valign=top>common exception classes for Barry layer</td>
</tr><tr>
	<td valign=top>probe.{h,cc}</td>
	<td valign=top>USB probe class to find Blackberry devices</td>
</tr><tr>
	<td valign=top>protocol.{h,cc}</td>
	<td valign=top>structs and defines for packets seen on wire</td>
</tr><tr>
	<td valign=top>common.{h,cc}</td>
	<td valign=top>general API and utilities</td>
</tr><tr>
	<td valign=top>socket.{h,cc}</td>
	<td valign=top>socket class encapsulating the Blackberry logical socket</td>
</tr><tr>
	<td valign=top>record.{h,cc}</td>
	<td valign=top>programmer-friendly record classes</td>
</tr><tr>
	<td valign=top>parser.{h,cc}</td>
	<td valign=top>C++ virtual wrappers to connect record and controller in a generic way</td>
</tr><tr>
	<td valign=top>controller.{h,cc}</td>
	<td valign=top>high level API class</td>
</tr><tr>
	<td valign=top>s11n-boost.h</td>
	<td valign=top>serialization functions for record.h classes</td>
</tr><tr>
	<td valign=top>barry.h</td>
	<td valign=top>application header (only one needed)</td>
</tr>
</table>
</ul>

<p>Misc utilities:
<ul>
<table border=0>
<tr>
	<td valign=top>btool.cci</td>
	<td valign=top>command line testing utility</td>
</tr>
<tr>
	<td valign=top>convo.awk</td>
	<td valign=top>script to convert UsbSnoop log files into trimmed-down request/response conversations</td>
</tr>
<tr>
	<td valign=top>translate.cc</td>
	<td valign=top>translate UsbSnoop log file data into hex+ascii dumps</td>
</tr>
</table>
</ul>

<p>Enjoy!</p>
