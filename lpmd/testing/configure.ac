
AC_INIT
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(lpmd, 0.5.1)
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_STDC_HEADERS

dnl Set the LPMDDIR directory
AC_ARG_WITH(lpmd,
    [  --with-lpmd=DIR         liblpmd installation directory (default /usr/local)],
    [ if test "${with_lpmd}" != "no"
      then
        LPMDDIR="${with_lpmd}"
      fi ])

if test -z "${LPMDDIR}"; then
  LPMDDIR="/usr/local"
fi

CPPFLAGS="${CPPFLAGS} -I${LPMDDIR}/include"
LDFLAGS="${LDFLAGS} -L${LPMDDIR}/lib"

AC_CHECK_LIB([lpmd], [main], [LIBS="${LIBS} -llpmd -ldl"], 
             [
	      AC_MSG_ERROR([Could not find the lpmd library. You may want to specify the installation directory with --with-lpmd]) 
	     ], [-ldl])
AC_OUTPUT(common/Makefile main/Makefile analyzer/Makefile converter/Makefile Makefile)

