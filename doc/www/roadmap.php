<? createHeader("Barry - Roadmap"); ?>

<? include ("barry.inc"); ?>

<div class="subHeader">Milestone 1 - mostly complete</div>
<ul>
	<li> autoconf the project
	<li> handle USB interface and configuration numbers dynamically
	<li> handle USB endpoint numbers dynamically (appears to use the endpoint numbers in ascending order: 1 read, 1 write)
	<li> make the old/new DBDB commands dynamic
	<li> flesh out LDIF support (should read/import them as well)
	<li> reverse engineer email header data
	<li> use SWIG to make the API available in Python
</ul>

<div class="subHeader">Milestone 2 - complete for all but email</div>
<ul>
<li> reliable command line backup and restore of:
	<ul>
		<li> Contacts / Address Book
		<li> Calendar
		<li> Email
	</ul>
</ul>

<div class="subHeader">Milestone 3 - complete</div>
<ul>
	<li> reliable backup and restore of all databases </li>
</ul>

<div class="subHeader">Milestone 4</div>
<ul>
	<li> reliable command line backup and restore of Java program modules
</ul>

<div class="subHeader">Milestone 5 - currently implemented through OpenSync</div>
<ul>
	<li>design and implement sync functionality (must support data from any external data source)
	<li>command line sync tool
</ul>

<div class="subHeader">Milestone 6 - currently implemented through OpenSync</div>
<ul>
	<li>GUI sync tool
</ul>

<div class="subHeader">Milestone 7 - currently implemented through OpenSync</div>
<ul>
	<li>Evolution sync
</ul>

<div class="subHeader">Milestone 8 - currently implemented through OpenSync</div>
<ul>
	<li>Mozilla sync
</ul>

<div class="subHeader">Milestone 9 - currently implemented through OpenSync</div>
<ul>
	<li>LDAP sync
</ul>

