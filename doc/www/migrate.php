<? include ("barry.inc"); ?>

<? createHeader("Migrating Device Data"); ?>

<p>
<center>
<img src="desktop-migrate.png" width="480" height="152" border="1"
	alt="Barry Desktop migrate dialog" />
</center>

<p>The Migrate dialog provides an easy method of backing up the device's
databases, and restoring them to another device.

<p>It currently only supports the database data, and does not migrate any
of the Java applications that might be on the existing device.

<p>There are three main combo boxes: the source device, the destination
device, and the write mode.  If both devices are plugged in at the
same time, it is possible to select them both at the beginning, but
if you only have one USB cable, let it prompt you for the new device.

<p>The available write modes are described below:

<p>
<ul>
	<li> <b>Erase all, then restore</b> - (default) Erases all existing
		data for each database restored before uploading records.</li>
	<li> <b>Add new, and overwrite existing</b> - Only deletes existing
		records if there is a conflict in the record ID, and adds
		all other records.</li>
	<li> <b>Add new, don't overwrite existing</b> - Only adds records,
		never deletes or overwrites.  Will skip uploading records
		where the IDs already exist.  This may mean that not all the
		data from the old device will make it to the new one.</li>
	<li> <b>Add every record as a new entry</b> - Deletes nothing, and
		adds everything new.  This may cause duplicates, depending
		on the data in both devices.</li>
</ul>

<p>Use the default write mode if you are unsure which option to use.


<? createSubHeader("Internal Details"); ?>

<p>The Migrate dialog will basically perform a full backup of the source
device, and will create a new backup tarball in the usual location, with
the label "migrate".

