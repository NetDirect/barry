<? include ("barry.inc"); ?>

<? createHeader("Use Blackberry USB channels with Barry"); ?>

<? createSubHeader("Introduction"); ?>

<p>The Blackberry provides the ability for programs to send and receive data over the USB port via named channels.
On device there are two APIs which provide access to these channels:
<ul>
	<li><a href="http://www.blackberry.com/developers/docs/5.0.0api/net/rim/device/api/system/USBPort.html">net.rim.device.api.system.USBPort</a></li>
	<li><a href="http://www.blackberry.com/developers/docs/5.0.0api/javax/microedition/io/Connector.html">javax.microedition.io.Connector</a></li>
</ul>
</p>

<p>Barry provides APIs and tools to access the PC side of these USB
channels. The Barry APIs provide similar functionality to the <a
href="http://docs.blackberry.com/en/developers/deliverables/1093/Desktop_API_Reference_Guide_46.pdf">Blackberry
Desktop Manager API</a>.</p>

<p>To use USB channels with Barry you will also need the following:
<ul>
	<li> a working Barry install, version 0.17 or later </li>
	<li> a program which uses USB channels installed on your Blackberry </li>
</ul>
</p>

<? createSubHeader("Using USB channels to talk to 'USB Demo'"); ?>

<p>As part of the examples provided with the Blackberry JDEs there is a demo
application called 'USB Demo' which can be found in <tt>samples/com/rim/samples/device/usbdemo</tt> in the
installation folder of the JDE. You should build this and install it on your Blackberry.</p>

<p>The Blackberry JDEs come with an example application for the Blackberry Desktop Manager API in
<tt>samples/usbclient</tt> in the installation folder. The equivalent PC side usbclient can be
found in the Barry source in <tt>examples/usbclient.cc</tt></p>

<p>Once the example is compiled it should be used by first running the
'USB Demo' application on the Blackberry and selecting one of the
'Connect' options from the menu. Once the 'USB Demo' application is
waiting then it's possible to run the <tt>usbclient</tt> example to
communicate with the Blackberry application.

<? createSubHeader("Using USB channels as a stream"); ?>

<p>The raw channel API provided by Barry exposes the channel interface
as sending and receiving of packets of data. While this closely
reflects the USB channel protocol, it can be unhelpful for
applications which are designed with <a
href="http://en.wikipedia.org/wiki/Stream_(computing)">streams</a> as
their communication mechanisms. To accommodate this Barry provides
a tool called <tt>brawchannel</tt>. This tool redirects the named
channel over STDIN and STDOUT, allowing easy communication with
pre-existing tools.</p>

<p>For example the following shows an example of using
<tt>brawchannel</tt> to talk to the 'USB Demo' application mentioned
in the previous section.</p>

<p>First it's necessary to run 'USB Demo' on the Blackberry and select
the 'Connect (low level)' menu option.  With that running it's then
possible to run <tt>brawchannel</tt> by supplying the channel name on
the command line. For the 'USB Demo' application this channel name is
<tt>JDE_USBClient</tt>.</p>

<pre>user@machine:~$ brawchannel JDE_USBClient</pre>

<p>If all goes well then you should see nothing but a flashing
cursor. The 'USB Demo' application expects us to say hello, so
type:</p>

<pre>Hello from Barry.&lt;RET&gt;</pre>

<p>You should then see two things, firstly your terminal should now
look like the following:</p>

<pre>user@machine:~$ brawchannel JDE_USBClient
Hello from Barry.
Hello from Device</pre>

<p>Secondly the message you typed should appear on the device (along
with the demo application complaining that it's not the expected
text).  Next we need to say goodbye to the device, so type:</p>

<pre>Goodbye from Barry.&lt;RET&gt;</pre>

<p>The device should then say goodbye back, followed by it closing the connection.</p>

<p>It's also possible to use other programs, such as <tt>socat</tt> to
use the <tt>brawchannel</tt> tool to communicate with the USB channel
via other mechanisms such as TCP/IP sockets.</p>

<p>For further information please see the man page for <tt>brawchannel</tt>.</pre>

<? createSubHeader("Using USB channels in your application"); ?>

<p>It is also possible to use USB channels directly in your
application by linking to the Barry library. To make use of these USB
channels you will need to create an instance of the
<tt>Barry::Mode::RawChannel</tt> class. The Barry source code
demonstrates creation of a RawChannel in
<tt>examples/usbclient.cc</tt> and <tt>tools/brawchannel.cc</tt>.</p>

<p>In it's most basic form this can be done with the following code:</p>
<pre>Barry::Init();
Barry::Probe probe;
if( probe.GetCount() <= 0 ) {
    // No blackberry found
    return 1;
}

auto_ptr<SocketRoutingQueue> router;
router.reset(new SocketRoutingQueue());
router->SpinoffSimpleReadThread();

Barry::Controller con(probe.Get(0), *router);

Barry::Mode::RawChannel rawChannel(con);

rawChannel.Open("", CHANNEL_NAME);</pre>

<p>With data then sent and received using <tt>rawChannel.Send()</tt> and <tt>rawChannel.Receive()</tt>.</p>
