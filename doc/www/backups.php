<? include ("barry.inc"); ?>

<? createHeader("Backing up your BlackBerry Data"); ?>


<? createSubHeader("Introduction"); ?>

<p>Backups are currently performed using the Barry Backup GUI program, located
in the gui/ directory of the source tarball.  Instructions for building the
GUI can be found on the <? createLink("install", "Install"); ?> and
<? createLink("cvs", "CVS"); ?> pages.</p>


<? createSubHeader("GTK+ Themes"); ?>

<p>The Barry Backup GUI program is written using the GTK+ library.  If you
do not use the Gnome desktop, then the default theme for GTK+ applications
often looks terrible.</p>

<p>Themes can be chosen for GTK+ apps without Gnome's settings
or daemons interfering with your desktop of choice.</p>

<p>Search under the /usr/share/themes directory for 'gtkrc' files.  Pick
the one you like best, and then include it by reference in your .gtkrc-2.0
config file in your home directory.</p>

<p>For example, suppose you want to use the GTK+ Clearlooks theme.
Your .gtkrc-2.0 file would contain:</p>

<pre>
	include "/usr/share/themes/Clearlooks/gtk-2.0/gtkrc"
</pre>


<? createSubHeader("Backup Interface"); ?>


<p>When Backup starts, it scans the USB bus for Blackberry devices.  If it
finds one, it assumes that is the device you want to work with.  If it
finds two, it prompts you to select the device to backup.</p>

<p>When connecting to a device for the first time, you are prompted to
give the device a name.  This will be linked to the device's PIN number
for easy identification later.</p>

<p>The main screen presents you with two options: Backup and Restore.</p>

<? createImage("backup.png"); ?>

<p>When backing up your device, your data is saved in a compressed tar file
in your home directory, under ~/.barry, organized by PIN number.  Each tar
backup file is given a timestamp in its filename.</p>

<p>When the backup starts, you are prompted to give the backup an optional
name.  If you specify a name, it will be used as part of the tar filename.</p>



<? createSubHeader("Restore"); ?>

<p>Pressing the Restore button will show a File Open dialog, pointing
to your ~/.barry directory, for the current device PIN.</p>

<? createImage("restore.png"); ?>

<p>You are not limited to the tar files in this directory, nor are you
limited to backup files from the same device.  You can use this tool
to copy data from one device to another.</p>

<p>The restore is governed by your current configuration, which determines
what databases are restored.  Any database that is restored will be
completely erased before the backup data is re-written.</p>

<p>There are some databases that are read-only on the BlackBerry.
In addition, if your device is connected to a BES, you may not be
able to restore your data.  If you get errors when trying to
restore a certain read-only database, disable that database in
the Restore Configuration dialog, as described below.</p>



<? createSubHeader("Configuration"); ?>

<p>Configuration is performed via the Edit menu, and provides two
filters for determining what databases are accessed during backup and
restore operations.  You can also change the device name here, and turn
off the backup name prompt.</p>

<? createImage("config.png"); ?>

<p>Each configure button provides essentially the same dialog, containing a
list of databases available on your device, as well as checkboxes to
enable them.</p>

<p>For the backup filter, only the enabled databases will be saved.
For the restore filter, only the enabled databases will be restored,
even if the backup file contains more data.</p>

<? createImage("config-backup.png"); ?>

<p>This configuration is saved per device PIN.  If you backup a different
device, you will need to configure its backup strategy again.  You can only
change the configuration for the current device with these dialogs.</p>

