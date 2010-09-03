<? include ("barry.inc"); ?>

<? createHeader("Coding Guidelines"); ?>

<? createSubHeader("Style Guide"); ?>

<p>I use plain old vim for editing files.  As such, tabs are standard 8
spaces wide.  When aligning the code, indents are always tabs.
<pre>
	if( something ) {
		// tab to indent
	}
</pre>
</p>

<p>Sometimes I need spaces to align function call parameters:
<pre>
	void ClassName::Function(int lots,
				 int of,
				 int parameters,
				 int that_need_spaces_and_tabs,
				 int to_align_perfectly)
	{
		// tab again
	}
</pre></p>


<p>I even use tabs to align the beginning parts of comments:
<pre>
	///
	/// \file	tab_to_filename.cc
	///		Tab to the doxygen description
	///
</pre></p>

<p>I also use tabs to align things like simple #define numbers:
<pre>
	#define ASDF_NAME	0x01	// tab between name and number
	#define ASDF_BODY	0x02	// tab between number and comment
</pre>
</p>

<p>The main place where I <b>don't</b> use tabs is inside tables that have to
be aligned, especially where there's not enough space to fit things in
one line when using tabs.  For example, in the record classes, those
FieldLink&lt;&gt; tables might use tabs for the initial indent,
but <b>everything</b> else is spaces, to keep things lined up, and compact:
<pre>
	FieldLink<Task> TaskFieldLinks[] = {
	   { TSKFC_TITLE,      "Summary",     0, 0, &Task::Summary, 0, 0 },
	   { TSKFC_NOTES,      "Notes",       0, 0, &Task::Notes, 0, 0 },
	   { TSKFC_START_TIME, "Start Time",  0, 0, 0, 0, &Task::StartTime },
	   { TSKFC_DUE_TIME,   "Due Time",    0, 0, 0, 0, &Task::DueTime },
	   { TSKFC_ALARM_TIME, "Alarm Time",  0, 0, 0, 0, &Task::AlarmTime },
	   { TSKFC_CATEGORIES, "Categories",  0, 0, &Task::Categories, 0, 0 },
	   { TSKFC_END,        "End of List", 0, 0, 0, 0, 0 },
	};
</pre></p>

<p>As for coding style, I keep opening braces on the statement line:
<pre>
	for( ... ) {
	}

	if( something ) {
	}
	else {
	}
</pre></p>

<p>Except for switches, because that's just wrong. :-)
<pre>
	switch( something )
	{
	case 1:
		break;
	case 2:
		break;
	default:
		break;
	}
</pre></p>

<p>I put spaces inside the parentheses too.</p>

<p>For reeeeeally long lines, I sometimes favour keeping it all on one line
and things wrap.  This is flexible... whichever looks best.  But also
remember that grep is broken by wrapped lines, so if you're writing
code that could conceivably be grepped later, decide whether breaking
the line is worth it.  I usually try to keep error message strings on
one line, even if they are long, since it makes it easier to grep for
them when bug reports come in.
<pre>
        // example error message....
	dout("Error 1234: too many rules");
</pre></p>

<p>For pointer and reference variables, the pointer and reference symbol
goes next to the variable, not the the type:

<pre>
	Data* data;           // wrong

	Data *data;           // right
</pre>

The reason for this becomes obvious when you consider what a multi-variable
declaration looks like.  The first style confuses things.  The second
flows naturally:

<pre>
	Data* block1, block2;  // wrong, declares a pointer and an object

	Data *block1, *block2; // right, declares two pointers
</pre>

<p>I think that covers it.  You may see some funky for() statements sometimes,
due to size:
<pre>
	for(	FieldLink&lt;Task&gt; *b = TaskFieldLinks;
		b->type != TSKFC_END;
		b++ )
	{
	}
</pre></p>

<p>As long as it is clear to read, I'm generally ok.  You'll notice the
fixation on tabs again in this example.  I'm less fussy about that,
if it's clear to read.
</p>

<p>Sometimes spaces are used to align ostream output as well:
<pre>
	os &lt;&lt; "something"
	   &lt;&lt; "something more"
	   &lt;&lt; std::hex &lt;&lt; some_number;
</pre></p>

<p>Chris Frey</p>

