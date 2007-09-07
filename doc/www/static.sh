#!/bin/sh

for f in `cat content_list` ; do
	echo "Generating $f.html"
	cat prepend.php $f.php append.php | php > $f.html
done

