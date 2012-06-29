<? include ("barry.inc"); ?>

<? createHeader("Modem Mode in the Desktop"); ?>

<? createSubHeader("Introduction"); ?>

<p>The command line modem documentation can be found
<? createLink("modem", "here"); ?>.  The Desktop automates much of that
detail, using the following dialog:

<p>
<center>
<img src="desktop-modem.png" width="241" height="294" border="1"
	alt="Barry Desktop modem dialog" />
</center>

<p>This dialog assumes that you have pppd installed and that the system
permissions allow you to run it, or that you have one of the supported
GUI sudo packages installed (gksu or beesu).

<p>It lists the available ppp scripts for the known providers.  It will
also remember which one you chose last.

<p>The password is optional, depending on your device setup.

<p>Upon launching the modem connection, the pppd output will appear in
a separate xterm window.  You can cancel your connection at any time
by pressing Ctrl-C in that window.

