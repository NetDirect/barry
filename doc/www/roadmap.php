<? include ("barry.inc"); ?>

<? createHeader("Barry - Roadmap"); ?>

<p>This roadmap is taken from the
<a href="http://repo.or.cz/w/barry.git?a=blob;f=TODO;hb=HEAD">TODO file</a>
in the source tree, and while relatively up to date, the source tree is the
definitive copy.</p>

<? createSubHeader("Target: release version 0.16 (Sept 3)"); ?>
<ul>
	<li>incorporate Nicolas's java debugger patches</li>
	<li>incorporate barrybackup GUI changes by Ryan Li</li>
</ul>

<? createSubHeader("Target: release version 0.17 (Oct 1)"); ?>
<ul>
<li>test and fix all build and functionality issues on:</li>
	<ul>
		<li>Fedora 11</li>
		<li>Ubuntu 8.10, 9.04</li>
		<li>openSUSE 11.1</li>
		<li>openBSD</li>
	</ul>
<li>website documentation:</li>
	<ul>
		<li>add a more detailed set of instructions for how to
			contribute to the project using git</li>
		<li>incorporate Bill Paul's modem HOWTO for FreeBSD
			into web docs</li>
		<li>add docs for opensync 0.4x plugin</li>
	</ul>
</ul>

<? createSubHeader("Target: release version 0.18 (Nov 13)"); ?>
<ul>
	<li>polish up bfuse, and add feature to split out fields</li>
	<li>add record classes for Content Store, SMS Messages, based on
		Martin Owens' doc/barry-research.ods</li>
</ul>

