#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([misaka], [0.1], [www.lancelot.top])
AM_INIT_AUTOMAKE
AC_PROG_RANLIB
AC_CONFIG_SRCDIR([include/misaka.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
LT_INIT
AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lev':
AC_CHECK_LIB([ev], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h paths.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/time.h syslog.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm dup2 gettimeofday inet_ntoa memset setenv socket strchr strerror strrchr strstr strtol])

AC_CONFIG_FILES([Makefile app/Makefile lib/Makefile task/Makefile app/echo/Makefile link/Makefile])
AC_OUTPUT
