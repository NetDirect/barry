#!/bin/sh

if [ -z "$1" ] ; then
	echo "Using Barry default web doc config..."
	for f in `cat content_list` ; do
		echo "Generating $f.html"
		cat prepend.php php_conf1.php $f.php append.php | php > $f.html
	done
elif [ "$1" = "netdirect" ] ; then
	echo "Using NetDirect web doc config..."
	mkdir -p www.netdirect.ca/sites/www.netdirect.ca/files/barry/doxygen
	mkdir -p www.netdirect.ca/sites/www.netdirect.ca/files/images/barry
	cp *.png www.netdirect.ca/sites/www.netdirect.ca/files/images/barry

	mkdir -p www.netdirect.ca/pastefiles
	for f in `cat content_list` ; do
		echo "Generating $f.html"
		cat php_conf2.php $f.php | php > www.netdirect.ca/pastefiles/$f.html
	done
fi

