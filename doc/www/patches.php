<? include ("barry.inc"); ?>

<? createHeader("Submitting Patches"); ?>

<? createSubHeader("Coding Guidelines"); ?>

<p>If you are submitting code, please have a look at the
<? createLink("codingguide", "Coding Guidelines page"); ?>.</p>

<p>Please keep some things in mind when preparing your patches
for submission:
<ul>
	<li>use one patch per logical change</li>
	<li>test all coding changes</li>
	<ul>
		<li>If it is a change to the build system, make sure that
			the test/buildtest.sh script still works.</li>
	</ul>
	<li>include some commentary above your patch in your email</li>
	<li>when mailing patches, try to keep one patch per email</li>
	<li>do not cut and paste patches... either read them in
		directly to your mail body (preferred),
		or send as an attachment</li>
	<li>add a [PATCH] prefix to your subject line</li>
</ul>
</p>


<? createSubHeader("Why Submit Patches?"); ?>

<p>Submitting your changes via patch is a good thing.  It may seem like
an extra bit of work to create a patch and post it to the mailing list,
or to make your work available in a public git repo, but there are good
reasons why Open Source works that way:</p>

<ul>
	<li>Patches tell the maintainer that you <b>want</b> your change
		to be added to the tree.  It is often too easy for busy
		programmers to misunderstand someone's intentions if
		they just send a random file.  If intentions are not
		clear, work gets dropped on the floor.</li>
	<li>Patches show that you have worked with the source code,
		and hopefully have tested your change.</li>
	<li>Patches show that you have given some thought to where
		your changes should go in the tree.</li>
	<li>Patches to a public mailing list encourage peer review, and show
		that you are ok with your code being included in a public
		project.</li>
	<li>Patches to a public mailing list or a public repository become
		part of history, showing who did what, and when.</li>
	<li>Patches usually get top priority from developers.</li>
	<li>Patches make life easier for the developers, freeing up their
		time for more features and bug fixes.</li>
	<li>Patches turn you into a developer.  Your name can be added
		to the AUTHORS file.</li>
</ul>


<? createSubHeader("Generating Patches"); ?>

<p>Generating patches depends on the method you used to get the source code.
<ul>
	<li>If you are using a tarball, expand the tarball once into
		a pristine directory, and again into your "working
		directory."  When you are finished and
		ready to patch, do:
<pre>
	cd barry-work
	./buildgen.sh cleanall
	cd ..
	diff -ruN barry-orig barry-work > patchfile
</pre>
	</li>

	<li>If you are using the git tree, you can make your changes
		in your own branch, and then create patches for each
		commit you've made:
<pre>
	cd barry-git
	git format-patch origin/master
</pre>

	</li>

</ul>


<? createSubHeader("Methods for Submitting Patches"); ?>

<p>Submitting changes can happen in one of three methods:

<ul>
	<li>Send a patch to the
	<a href="http://sourceforge.net/mail/?group_id=153722">mailing list</a>.
	</li>

	<li>Publish your own git repository (perhaps by creating a
		forked tree on
		<a href="http://repo.or.cz/">repo.or.cz</a>)
		and notify the mailing list, indicating the
		branch you want people to pull from when
		you're ready.</li>

	<li>Use the "mob" branch on <a href="http://repo.or.cz/w/barry.git">
		Barry's git repository</a>, and....
		send a notification to the mailing list.</li>
</ul>
</p>


<? createSubHeader("Creating a Forked Tree on repo.or.cz"); ?>

<p>The git repo site repo.or.cz lets anyone create a forked tree based
off the official Barry repo.  This saves space on repo.or.cz, and
adds your fork to a list at the bottom of the official Barry page.</p>

<p>This way, users and developers can look at everyone's changes and
test and mix them as needed.</p>

<p>To create a forked tree, visit the <a href="http://repo.or.cz/w/barry.git">
Barry repo</a> and click "fork" at the top.</p>



<? createSubHeader("Using the Mob Branch"); ?>

<p>The public git repository service at repo.or.cz provides an interesting
feature, which allows anyone to push to a "mob" branch of a repository,
if so configured by the admin.</p>

<p> It would go something like this:
<pre>
        # clone with mob user
        git clone git+ssh://mob@repo.or.cz/srv/git/barry.git barry

        cd barry
        git checkout -b mob origin/mob
        git diff origin/master..mob             # make sure master == mob
        &lt;make changes&gt;
        git add ... && git commit
        git push origin mob
        &lt;send email to the list, include the SHA1 sum of the commit&gt;
</pre>
</p>

<p> This is a novel idea, as well as a security risk for anyone who blindly
runs whatever is in the mob branch.  Hence the recommended diff check
above, to make sure you're working on an official branch.</p>

<p> The mob user can only push to the mob branch, so all other branches
are read-only, and have been reviewed at least once by the project
maintainer.</p>

<p>  But the mob branch frees people up to use git, who may not have
their own hosting, or who may not want to bother setting up their
own git repo.  People can use it to collaborate on a feature as well.
Let your imagination run wild.</p>

<p>You can read more about the ideas behind the mob branch at
<a href="http://repo.or.cz/h/mob.html">the repo.or.cz mob page</a></p>

