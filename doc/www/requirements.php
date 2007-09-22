<? createHeader("barry - Requirements"); ?>

<? include ("barry.inc"); ?>

<div class="subHeader">Charging and the Kernel</div>

<p>One of the main features of Barry is the ability to control the charging
modes of the Blackberry, as well as changing configuration modes on
Pearl-like devices.</p>

<p>In order to achieve proper charging, udev is setup to run the bcharge
program every time you plug in your Blackberry.</p>

<p>Recent kernels have a module called berry_charge, which does similar
things from the kernel level.  These two methods can conflict if both
run at the same time.  Pick which one works for you, and disable the
other.</p>

<ul>
	<li>To disable berry_charge, either rename the module under
		/lib/modules, or recompile your kernel without that
		module enabled.</li>
	<li>To disable bcharge, comment out the udev commands in
		/etc/udev/rules.d/10-blackberry.rules.</li>
</ul>




<div class="subHeader">Power and the Kernel</div>

<p>Recent kernels also have the ability to put the USB bus and its devices
into suspend mode.  Kernels included in the latest Ubuntu 7.04 and
Fedora 7 have this turned on by default.</p>

<p>When bcharge runs, it successfully changes the Blackberry to use 500mA
(its normal charge power level), but then the kernel puts the device
into suspend mode.  This can have various undefined effects, such as
the charge icon disappearing on the device, or having your device lose
its charge in an accelerated manner.</p>

<p>Bcharge attempts to work around this by writing to the
/sys/.../device/power/state file to attempt to turn this suspend off,
but this is not always successful.  To be sure to have a working charge
setup, recompile your kernel with CONFIG_USB_SUSPEND disabled.</p>


<div class="subHeader">Device Ownership and Permissions</div>

<p>The Barry toolset performs all its actions through the /proc and/or
/sysfs filesystems, using the libusb library.  This requires that you
have permissions to write to the USB device files setup by the kernel.</p>

<p>This is handled differently on various systems:</p>

<ul>
	<li>On Debian based systems, there is a group called plugdev, which
		is used to control permissions for pluggable devices.
		When the barry-util deb package is installed, udev is
		configured to set Blackberry device permissions to
		the plugdev group.  Make sure your user is in the plugdev
		group.</li>
	<li>On Fedora based systems, ownership is controlled by the
		ConsoleKit package.  This changes ownership of pluggable
		devices to the user currently logged into the console,
		on the theory that anyone at the console should have
		control of the devices he plugs in.  No special
		support is needed by Barry if you have this package
		installed.</li>
</ul>

