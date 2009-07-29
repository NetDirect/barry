<? include ("barry.inc"); ?>

<? createHeader("Barry - Roadmap"); ?>

<? createSubHeader("Milestone 1 - mostly complete"); ?>
<ul>
	<li> autoconf the project
	<li> handle USB interface and configuration numbers dynamically
	<li> handle USB endpoint numbers dynamically (appears to use the endpoint numbers in ascending order: 1 read, 1 write)
	<li> make the old/new DBDB commands dynamic
	<li> flesh out LDIF support (should read/import them as well)
	<li> reverse engineer email header data
	<li> use SWIG to make the API available in Python
</ul>

<? createSubHeader("Milestone 2 - complete for all but email"); ?>
<ul>
<li> reliable command line backup and restore of:
	<ul>
		<li> Contacts / Address Book
		<li> Calendar
		<li> Email
	</ul>
</ul>

<? createSubHeader("Milestone 3 - complete"); ?>
<ul>
	<li> reliable backup and restore of all databases </li>
</ul>

<? createSubHeader("Milestone 4 - complete"); ?>
<ul>
	<li> reliable command line backup and restore of Java program modules
</ul>

<? createSubHeader("Milestone 5 - currently implemented through OpenSync"); ?>
<ul>
	<li>design and implement sync functionality (must support data from any external data source)
	<li>command line sync tool
</ul>

<? createSubHeader("Milestone 6 - currently implemented through OpenSync"); ?>
<ul>
	<li>GUI sync tool
</ul>

<? createSubHeader("Milestone 7 - currently implemented through OpenSync"); ?>
<ul>
	<li>Evolution sync
</ul>

<? createSubHeader("Milestone 8 - currently implemented through OpenSync"); ?>
<ul>
	<li>Mozilla sync
</ul>

<? createSubHeader("Milestone 9 - currently implemented through OpenSync"); ?>
<ul>
	<li>LDAP sync
</ul>

