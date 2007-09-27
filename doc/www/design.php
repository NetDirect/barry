<? createHeader("barry - Design"); ?>

<? include("barry.inc"); ?>

<div class="subHeader">Architecture</div>

<p>Barry is designed to be modular in many ways.

<p>First the core protocols will be stored in a library so that it can
be used in other applications.

<p>Second, a command-line tool will be used to present the user with a
scriptable option in dealing with the BlackBerry &trade; device. This
will come in handy in conjunction with hotplug.

<p>Third, a GUI tool will be used to allow users to directly manage
devices and make backups of their data.

<p>Fourth, an <a href="http://www.opensync.org/">OpenSync Plugin</a>
will provide general synchronization support for Contacts, Calendar,
Tasks, and Memos; and the OpenSync framework will provide the synchronization
support for the other side othe equation. (Evolution, Thunderbird, etc.)


<div class="subHeader">Design Goals</div>

<p>Flexible Synchronization Targets: The main goal of barry is to
synchronize data.  This is complicated by the fact that the data on a
user's computer could be stored in many different formats.  Email, contact,
calendar entries, notes, and bookmarks are the main data that we are
concerned with and users may be using one of many programs to manage
their data.  To address this barry needs to be flexible in how it supports
synchronization.  It should be easy to add new synchronization targets
to barry and easy to select these targets from the user interface.  </p>

<p>Open Data Formats: Backup of the BlackBerry &trade; data should be
stored and read in open data formats.  LDIF for example is a suitable
format for contacts, mbox format is a perfect format for email data,
text files are a good format for notes, and HTML is a suitable format
for bookmarks.  The backup GUI stores its raw backup data in gzipped tar file
format.</p>

<p>Multi-Device: A user may need to manage more than one device
on a single user profile.  Barry will be able to treat each device
independantly to allow it to sync differently, backup/restore to different
targets, etc.  This will come in handy when users are upgrading to a
new device, or when one user handles backup for more than one person.

<p>Device Support: initially we will be supporting the 7750 devices,
because that's what we are using.  We do have a 6750 here as well,
but since it is a discontinued product we are not testing on it. The
7750 has a USB interface to the PC and we expect that there will not be
significant changes between devices.  </p>

