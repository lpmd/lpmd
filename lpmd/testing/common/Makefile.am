
#
#
#

version.h:
	../update-svnrev.sh ..

install:
	../update-svnrev.sh ..
	$(MAKE) install-am

install-exec:
	../update-svnrev.sh ..
	$(MAKE) install-exec-am

CLEANFILES = version.h

#
#
#

lib_LIBRARIES = liblpmdcommon.a
liblpmdcommon_a_SOURCES = input.h input.cc cmdline.h cmdline.cc handler.h handler.cc

