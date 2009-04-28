#!/bin/sh

AUTOMAKE=automake
ACLOCAL=aclocal

# aclocal bug?
mkdir m4 &>/dev/null

AUTOMAKE=$AUTOMAKE ACLOCAL=$ACLOCAL autoreconf --install
