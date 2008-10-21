
AC_INIT
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(lpmd, 0.5.2)
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_STDC_HEADERS

AC_ARG_WITH(lpmd,
    [  --with-lpmd=DIR         liblpmd installation directory (default /usr)],
    [ if test "${with_lpmd}" != "no"
      then
        LPMDDIR="${with_lpmd}"
      fi ])

PKG_CHECK_MODULES(LPMD, liblpmd-1.0 > 1.0.1,
                  CPPFLAGS="$CPPFLAGS $LPMD_CFLAGS";
                  LDFLAGS="$LDFLAGS $LPMD_LIBS";
                  LPMDDIR="$LPMD_PREFIX", 
                  if test -z "${LPMDDIR}"; then
                     LPMDDIR="/usr/local"
                  fi
                  CPPFLAGS="${CPPFLAGS} -I${LPMDDIR}/include"
                  LDFLAGS="${LDFLAGS} -L${LPMDDIR}/lib"
                  AC_SUBST(CPPFLAGS)
                  AC_SUBST(LDFLAGS)
)

AC_CHECK_LIB([lpmd], [main], [LIBS="${LIBS} -llpmd -ldl"],
[
 AC_MSG_ERROR([Could not find the lpmd library. You may want to specify the installation directory with --with-lpmd])
], [-ldl])

AC_OUTPUT(common/Makefile main/Makefile analyzer/Makefile converter/Makefile visualizer/Makefile datacenter/lpmd-datacenter datacenter/Makefile Makefile)

