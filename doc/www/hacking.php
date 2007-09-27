<? createHeader("Hacking Barry"); ?>

<? include ("barry.inc"); ?>

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
send patches and/or documentation to the
<a href="http://sourceforge.net/mail/?group_id=153722">mailing list</a>!</p>

<p>See the doc/Hacking file for more information on getting started reverse
engineering the protocol.</p>


<div class="subHeader">Some notes on code architecture</div>

<p>Lowest level:
<ul>
        Lowest level is the libusb software.
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
	<td valign=top>debug.h</td>
	<td valign=top>general debugging output support</td>
</table>
</ul>


<p>Barry low level layer:
<ul>
<table border=0>
<tr>
	<td valign=top>packet.{h,cc}</td>
	<td valign=top>low level packet builder class, having knowledge of
		specific protocol commands in order to hide protocol details
		behind an API</td>
</tr><tr>
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
	<td valign=top>endian.h</td>
	<td valign=top>big/little endian defines... only used for compiling
		the library, never installed</td>
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
	<td valign=top>controller.{h,cc}, controllertmpl.h</td>
	<td valign=top>high level API class</td>
</tr><tr>
	<td valign=top>version.h</td>
	<td valign=top>library version information and API</td>
</tr><tr>
	<td valign=top>s11n-boost.h</td>
	<td valign=top>serialization functions for record.h classes</td>
</tr><tr>
	<td valign=top>barry.h</td>
	<td valign=top>application header (only one needed)</td>
</tr><tr>
	<td valign=top>cbarry.h</td>
	<td valign=top>C application header (incomplete)</td>
</tr>
</table>
</ul>

<p>Misc utilities:
<ul>
<table border=0>
<tr>
	<td valign=top>btool.cc</td>
	<td valign=top>command line testing utility</td>
</tr><tr>
	<td valign=top>bcharge.cc</td>
	<td valign=top>set device to use 500mA, and also enables database access for Blackberry Pearl devices</td>
</tr><tr>
	<td valign=top>breset.cc</td>
	<td valign=top>does a USB level software reset on all Blackberry devices found</td>
</tr><tr>
	<td valign=top>convo.awk</td>
	<td valign=top>script to convert UsbSnoop log files into trimmed-down request/response conversations</td>
</tr><tr>
	<td valign=top>ktrans.cc</td>
	<td valign=top>turns USB kernel capture logs from 2.6 kernels into hex+ascii dumps</td>
</tr><tr>
	<td valign=top>translate.cc</td>
	<td valign=top>translate UsbSnoop log file data into hex+ascii dumps</td>
</tr><tr>
	<td valign=top>upldif.cc</td>
	<td valign=top>takes an ldap LDIF file on stdin and uploads contact data to the Blackberry, overwriting existing contacts</td>
</tr>
</table>
</ul>

<p>Example code:
<ul>
<table border=0>
<tr>
	<td valign=top>addcontact.cc</td>
	<td valign=top>example for adding a contact record to the device</td>
</tr>
</table>
</ul>


<p>Enjoy!</p>

