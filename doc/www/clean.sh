#!/bin/sh

for f in `cat content_list` ; do
	rm -f $f.html
done

