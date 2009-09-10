<? include ("barry.inc"); ?>

<? createHeader("Barry - Roadmap"); ?>

<p>This roadmap is taken from the
<a href="http://repo.or.cz/w/barry.git?a=blob;f=TODO;hb=HEAD">TODO file</a>
in the source tree, and while relatively up to date, the source tree is the
definitive copy.</p>

<? createSubHeader("Target: release version 0.16 (Oct 1)"); ?>
<ul>
	<li>incorporate barrybackup GUI changes by Ryan Li (done)</li>
	<li>incorporate Nicolas's java debugger patches</li>
	<li>incorporate Martin Owens' Barry logo/icon into Barry</li>
</ul>

<? createSubHeader("Target: release version 0.17 (Nov 1)"); ?>
<ul>
	<li>website documentation</li>
</ul>

<? createSubHeader("Target: release version 0.18 (Dec 1)"); ?>
<ul>
	<li>test and fix all build and functionality issues on:</li>
		<ul>
			<li>Fedora 11</li>
			<li>Fedora 12 (scheduled release on Nov 10)</li>
			<li>Ubuntu 8.10, 9.04</li>
			<li>Ubuntu 9.10 (scheduled release on Oct 29)</li>
			<li>openSUSE 11.1</li>
			<li>openBSD</li>
		</ul>
	<li>support our own repositories for apt/yum/zypper installs</li>
</ul>

<? createSubHeader("Target: release version 0.19 (Jan 1, 2010)"); ?>
<ul>
	<li>polish up bfuse, and add feature to split out fields</li>
	<li>add record classes for Content Store</li>
</ul>

