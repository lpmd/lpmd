#!/bin/sh

rm -rf configure Makefile Makefile.in .deps .libs libtool config.* depcomp config.guess config.sub ltmain.sh INSTALL config.h.in autom4te.cache missing aclocal.m4 install-sh stamp-h1

if [ "$1" != "clean" ]; then
   if [ -f /usr/share/libtool/libtool.m4 ]; then
      cp /usr/share/libtool/libtool.m4 acinclude.m4
   elif [ -f /usr/share/aclocal/libtool.m4 ]; then
      cp /usr/share/aclocal/libtool.m4 acinclude.m4
   else
      echo "Could not find libtool.m4!" 
   fi
   aclocal
   libtoolize --copy --force
   autoconf
   autoheader
   automake --add-missing --copy
fi

