#!/bin/bash
set -u

gcc -Wall -O2 simple-test.c -o simple-test || exit 1

TF="$(mktemp)" || exit 1

for wbin in tiny2 wrapper-* ; do
	echo "== $wbin =="
	echo
	for try in {1..3} ; do
		{ time ./simple-test "./$wbin" |uniq -c ; } >"$TF" 2>&1
		fgrep -q ' 100000 hello, world!' "$TF" || echo "FAILED TO FIND THE UNIQ MARKER" 
		grep -Pv '^( 100000 hello, world\!$|user|sys|\s*$)' "$TF"
	done
	echo
done

rm "$TF"
