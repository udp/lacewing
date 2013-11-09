AC_PREREQ([2.49])

AC_INIT([liblacewing], [0.5.4], [])
VERSION_MAJOR="0.5"

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],
		 [Build a debug version of the library @<:@default=disabled@:>@])],
	[enable_debug="$enableval"],
	[enable_debug=no]
)

AC_ARG_ENABLE([spdy],
	[AS_HELP_STRING([--enable-spdy],
		 [Enable SPDY support @<:@default=enabled@:>@])],
	[enable_spdy="$enableval"],
	[enable_spdy=yes]
)

AC_ARG_ENABLE([cxx],
	[AS_HELP_STRING([--enable-cxx],
		 [Build the C++ wrappers @<:@default=enabled@:>@])],
	[enable_cxx="$enableval"],
	[enable_cxx=yes]
)

AC_ARG_ENABLE([ssl],
	[AS_HELP_STRING([--enable-ssl],
		 [Build with SSL support @<:@default=enabled@:>@])],
	[enable_ssl="$enableval"],
	[enable_ssl=yes]
)

CFLAGS="$CFLAGS -DHAVE_CONFIG_H -Wno-deprecated-declarations"

if test "x$enable_debug" = xyes; then
	CFLAGS="$CFLAGS -DDEBUG -D_lacewing_debug -g -O0"
else
	CFLAGS="$CFLAGS -O2"
fi

if test "x$enable_cxx" = xyes; then
    AC_PROG_CXX
    AC_SUBST(ENABLE_CXX, 1)
fi

if test "x$enable_ssl" = xyes; then
    AC_SUBST(ENABLE_SSL, 1)
else
    CFLAGS="$CFLAGS -D_lacewing_no_ssl"
fi

SO_EXT="so"

if test "x$enable_spdy" = xyes; then
    AC_SUBST(ENABLE_SPDY, 1)
	AC_CHECK_LIB([z], [inflate])
else
    CFLAGS="$CFLAGS -D_lacewing_no_spdy"
fi

if test "x$EXEEXT" = "x.exe"; then

    AC_SUBST(PLATFORM, windows)

    SO_EXT="dll"
    LIBS="$LIBS -lmswsock -lws2_32 -lcrypt32 -lsecur32"

else

    AC_SUBST(PLATFORM, unix)

    if test "x$enable_ssl" = xyes; then
	    AC_CHECK_LIB([ssl], [SSL_library_init])
        AC_SEARCH_LIBS([MD5_Init], [ssl crypto])
    fi

	CFLAGS="$CFLAGS -fPIC -fvisibility=hidden"

	AC_CHECK_HEADERS([malloc.h sys/epoll.h sys/prctl.h sys/sendfile.h \
						 netdb.h sys/timerfd.h])

	AC_CHECK_HEADERS([CoreServices/CoreServices.h], [[LIBS="$LIBS -framework CoreServices"]
		[SO_EXT="dylib"]])

	AC_CHECK_FUNCS([timegm kqueue vasprintf])

	AC_CHECK_LIB([pthread], [pthread_create])
	AC_CHECK_LIB([rt], [clock_gettime])

	AC_CHECK_FUNCS([clock_gettime])

	AC_CHECK_DECLS([PR_SET_NAME], [], [], [AC_INCLUDES_DEFAULT
	[#include <sys/prctl.h>]])

	AC_CHECK_DECLS([TCP_CORK, TCP_NOPUSH, MSG_NOSIGNAL, SO_NOSIGPIPE], [], [], [AC_INCLUDES_DEFAULT
	[#include <sys/socket.h>
	 #include <netinet/tcp.h>]])

	AC_CHECK_DECLS([EPOLLRDHUP], [], [], [AC_INCLUDES_DEFAULT
	[#include <sys/epoll.h>]])
fi

CXXFLAGS="$CFLAGS -fno-exceptions -fno-rtti"
CFLAGS="-std=gnu99 -Wall -Wno-format $CFLAGS"

SO_NAME=liblacewing.$SO_EXT.$VERSION_MAJOR

if test "x$SO_EXT" = "xso"; then
    REAL_NAME=liblacewing.so.$PACKAGE_VERSION
else
    REAL_NAME=liblacewing.$PACKAGE_VERSION.$SO_EXT
fi

AC_SUBST(SO_NAME, $SO_NAME)
AC_SUBST(SO_EXT, $SO_EXT)
AC_SUBST(REAL_NAME, $REAL_NAME)

if test "x$host_alias" = "x"; then
    AC_SUBST(AR, ar)
else
    AC_SUBST(AR, $host_alias-ar)
fi

if test $SO_EXT = "so"; then
	AC_SUBST(SONAME_FLAGS, [-Wl,-soname,$SO_NAME])
else
	AC_SUBST(SONAME_FLAGS, [  ])
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

