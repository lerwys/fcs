# initial information about the project
AC_INIT([fcs],[1.0],[a.wojenski@elka.pw.edu.pl])

# check for C++ compiler
AC_PROG_CC
AC_PROG_CXX

LT_INIT

AM_INIT_AUTOMAKE([-Wall -Werror])

AC_PROG_INSTALL
AC_CONFIG_MACRO_DIR([m4])

# use the C++ compiler for the following checks
AC_LANG([C++])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([iostream])

AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# distribute additional compiler and linker flags
# --> set these variables instead of CXXFLAGS or LDFLAGS
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST([LIBS])

# test for C++ libraries
#SAVED_LIBS=$LIBS
#LIBS="$LIBS -lserial"
#LIBSERIAL=""
#AC_LINK_IFELSE(
#  [AC_LANG_PROGRAM([#include <SerialStream.h>],
#    [testing::SerialStream dummy])],
#  [LIBSERIAL="-lserial"] [HAVE_SERIAL_H=1],
#  [AC_MSG_WARN([libserial is not installed.])])
#LIBS=$SAVED_LIBS
#AC_SUBST(LIBSERIAL)
#
## Checks for Mini-XML library
#LIBMXML=""
#AC_CHECK_HEADER(mxml.h,
#  SAVELIBS="$LIBS"
#  AC_SEARCH_LIBS(mmxlNewElement,mxml,
#    AC_DEFINE(HAVE_MXML_H)
#    LIBMXML="-lmxml")
#  LIBS="$SAVELIBS")
#AC_SUBST(LIBMXML)

# files to generate via autotools (.am or .in source files)
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/chip/Makefile])
AC_CONFIG_FILES([src/commlink/Makefile])
AC_CONFIG_FILES([src/common/Makefile])
AC_CONFIG_FILES([src/interface/Makefile])
AC_CONFIG_FILES([src/wishbone/Makefile])

# generate the final Makefile etc.
AC_OUTPUT
