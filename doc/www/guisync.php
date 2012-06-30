<? include ("barry.inc"); ?>

<? createHeader("Syncing your BlackBerry, using the Desktop"); ?>

<? createSubHeader("Sync Interface"); ?>

<p>Below is the sync mode screen of the Barry Desktop GUI:

<p>
<center>
<? createImageEx("desktop-sync.png", 'width="480" height="352" alt="Barry Desktop sync screen"'); ?>
</center>


<? createSubHeader("Introduction"); ?>

<p>The Sync screen displays a list of all connected devices as well as
all devices that have an OpenSync configuration (whether those devices
are connected or not).

<p>Each device is setup to sync with a given application, which correspondes
to the available plugins on the OpenSync side.

<p>In addition, if multiple versions of the OpenSync engine library are
installed, the specific version to use for syncing can be selected during
configuration, and is shown in the list.

<p>The last sync date is also shown.  This timestamp is maintained
independently of the one maintained in OpenSync and is only for information
purposes.

<p>Multiple devices can be selected in order to sync them all at once.
The Sync Now button begins the sync, and each sync will be performed in
sequential order.


<? createSubHeader("The Run App button"); ?>

<p>The Run App button will, if available, launch the application that
the currently selected device is configured to sync against.  For example,
if you are syncing your BlackBerry to Evolution, Evolution will be launched.


<? createSubHeader("The Configure... button"); ?>

<p>The Configure... button opens the following dialog:

<p>
<center>
<? createImageEx("desktop-sync-config.png", 'width="449" height="370" border="1" alt="Barry Desktop sync configure dialog"'); ?>
</center>

<p>The Configure dialog displays settings for: the OpenSync engine's
library version to use, the two sync components (the BlackBerry and the
Application), sync options, and conflict resolution options.

<p>If only one version of OpenSync is installed, the engine options will
not be displayed.

<p>Configure the Barry side of the sync by entering the device password,
if needed.  The rest is optional.

<p>Configure the Application side by selecting the Application to sync
against, and then pressing the Configure button.  Depending on the
Application, the Configure button will either automatically configure
everything for you, or open a new dialog to adjust parameters for that
specific plugin.

<p>The available Applications / plugins are:
<ul>
	<li> Evolution </li>
	<li> KDEPIM (0.2x engine only) </li>
</ul>

<p>Depending on the available objects on both the Barry side of the sync
and the Application side, there will be one or more sync options to
choose from: Contacts, Events, Notes (Memos), and To-dos (Tasks).

<p>By default, Barry will ask you to resolve any conflicts, but it is
possible to override this using the conflict resolution choices at the
bottom of the dialog.  If you select Favour Device, then device data
will automatically overwrite the application data's record where there
is a conflict.


<? createSubHeader("The 1 Way Reset button"); ?>

<p>Back on the main Sync screen, there is one more button to discuss:
1 Way Reset.

<p>Due to the way OpenSync is designed, it is possible to get into a
"slow sync" state if one of the plugins crashes or if there is a serious
problem during the sync.  Unfortunately, this is normally not easy to
recover from.

<p>The best way to recover is to re-create the entire OpenSync configuration
for that particular device, delete all the data on one side of the sync,
and sync fresh from the beginning again.

<p>This recovery is what the 1 Way Reset button automates.  It will guide
you step by step through the process, and automatically reset the OpenSync
configuration, so that the next time you press Sync Now, a fresh sync
will start.

<p>Normally either your device or the Application is authoritative most
of the time.  For example, you mostly add data to your device, and sync
with your application, moving data from device to desktop.  The authoritative
side of the sync the side you want to keep, deleting the data
on the other side.

<p>If the Application is authoritative, 1 Way Reset will offer to delete
the data from your device for you, in preparation for the fresh sync.
Alternatively, if the device authoritative, 1 Way Reset will launch the
application so you can remove old records manually.

<p>Obviously, it is good to have backups first, both of your application data
and of your BlackBerry, should something go wrong.  The 1 Way Reset feature
is designed to help you manage OpenSync, in case you do end up in
"slow sync" land.

