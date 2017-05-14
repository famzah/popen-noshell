#!/bin/bash
set -u

gcc -Wall -O2 -pthread simple-test.c -o simple-test || exit 1

TF="$(mktemp)" || exit 1

for try in {1..3} ; do
	{ time ./simple-test |uniq -c ; } >"$TF" 2>&1
	fgrep -q ' 100000 hello, world!' "$TF" || echo "FAILED TO FIND THE UNIQ MARKER" 
	grep -Pv '^( 100000 hello, world\!$|user|sys|\s*$)' "$TF"
done

rm "$TF"
