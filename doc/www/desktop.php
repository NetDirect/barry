<? include ("barry.inc"); ?>

<? createHeader("The Barry Desktop GUI"); ?>

<? createSubHeader("Main Screen"); ?>

<p>Below is the main screen of the Barry Desktop GUI.  (Click the buttons
in the image to get more detailed help.)

<p>
<center>
<? createImageEx("desktop-main.png", 'width="480" height="352" alt="Barry Desktop main screen" usemap="#desktopmainmap"'); ?>
<map name="desktopmainmap">
	<area shape="rect" coords="6,39 152,121"
		<? createHref("backups"); ?> alt="Launch Backup GUI" />
	<area shape="rect" coords="166,39 313,121"
		<? createHref("guisync"); ?> alt="Sync Mode" />
	<area shape="rect" coords="326,39, 472,121"
		<? createHref("guimodem"); ?> alt="Modem Launcher" />
	<area shape="rect" coords="6,134 152,217"
		<? createHref("migrate"); ?> alt="Migrate Device" />
	<area shape="rect" coords="166,134 312,217"
		<? createHref("browse"); ?> alt="Database Browse Mode" />
</map>
</center>

<p>The screen is focused on the available modes that the Desktop supports.
Each button will either launch an application, or open a new mode or dialog
box to accomplish its task.

<p>The red berry in the top left corner is the main menu.  It supports the
following operations:
<ul>
	<li>Enable / disable verbose logging</li>
	<li>Give a name to the currently selected device</li>
	<li>Perform a USB reset on the currently selected device</li>
	<li>Re-scan the USB bus for new devices</li>
	<li>Display the About dialog</li>
	<li>Exit</li>
</ul>

<p>The currently selected device is chosen by the dropdown list in the lower
right corner of the screen.  This determines which device will be opened
for Syncing, Modem, and Database Browsing.  Backup and Migrate (Device Switch)
have their own methods for selecting devices.

<p>If your device is not password protected, a screenshot is taken and
displayed in the lower right hand corner of the screen, as shown.

