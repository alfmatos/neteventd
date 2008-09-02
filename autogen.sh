#!/bin/sh

AUTOMAKE=automake
ACLOCAL=aclocal

AUTOMAKE=$AUTOMAKE ACLOCAL=$ACLOCAL autoreconf --install
