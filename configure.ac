AC_INIT([Club Penguin Stamp Library], [0.9.7], [], [libcpstamp])

AC_REVISION([Revision 1])

AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([src/cpstamp.c])

# For debian systems, /usr as default
AC_PREFIX_DEFAULT([/usr])

# We need Gcc
AC_PROG_CC

# and automake
AM_INIT_AUTOMAKE([-Wall -Werror])

# Translate this program
AM_GNU_GETTEXT_VERSION([0.19.3])
AM_GNU_GETTEXT([external])

AM_PROG_AR

# Es una libreria, necesitamos libtool
LT_INIT

AM_PROG_CC_C_O

# Check for pkg-config
PKG_PROG_PKG_CONFIG

SDL_VERSION=1.2.14
SDL_IMAGE_VERSION=1.2.10
SDL_TTF_VERSION=2.0.11
SDL_MIXER_VERSION=1.2.12

AC_MSG_CHECKING([if you have SDL installed on your system])
PKG_CHECK_EXISTS([sdl >= $SDL_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL not found in your system])])
PKG_CHECK_MODULES(SDL, [sdl >= $SDL_VERSION], [], [])

AC_MSG_CHECKING([if you have SDL_image installed on your system])
PKG_CHECK_EXISTS([SDL_image >= $SDL_IMAGE_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL_image not found in your system])])
PKG_CHECK_MODULES(SDL_image, [SDL_image >= $SDL_IMAGE_VERSION], [], [])

AC_MSG_CHECKING([if you have SDL_ttf installed on your system])
PKG_CHECK_EXISTS([SDL_ttf >= $SDL_TTF_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL_ttf not found in your system])])
PKG_CHECK_MODULES(SDL_ttf, [SDL_ttf >= $SDL_TTF_VERSION], [], [])

AC_MSG_CHECKING([if you have SDL_mixer installed on your system])
PKG_CHECK_EXISTS([SDL_mixer >= $SDL_MIXER_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL_mixer not found in your system])])
PKG_CHECK_MODULES(SDL_mixer, [SDL_mixer >= $SDL_MIXER_VERSION], [], [])

AC_CONFIG_HEADERS([config.h])

# Revisar el host
AC_CANONICAL_HOST

case $host_os in
  *mingw32* ) MINGW32=yes;;
          * ) MINGW32=no;;
esac
case $host_os in
  *cygwin* ) CYGWIN=yes;;
         * ) CYGWIN=no;;
esac
case $host_os in
  *linux* ) LINUX=yes;;
        * ) LINUX=no;;
esac
case $host_os in
 *darwin* ) MACOSX=yes;;
       * ) MACOSX=no;;
esac

AM_CONDITIONAL(MINGW32, test x$MINGW32 = xyes)
AM_CONDITIONAL(LINUX, test x$LINUX = xyes)
AM_CONDITIONAL(MACOSX, test x$MACOSX = xyes)

dnl Add -DMACOSX to CXXFLAGS and CFLAGS if working under darwin
if test "x$MACOSX" = xyes; then
 CPPFLAGS="$CPPFLAGS -DMACOSX"
fi

dnl Add special hack for Mac framework SDL after all other SDL stuff has been done
dnl Note that SDL development environment must be installed for the build but SDL frameworks must also be installed for the build
dnl If we had libSDLmain.a, which is not included in the framework, we should be able to get away with only installing framework
if test "x$MACOSX" = xyes; then
 AM_PATH_SDL(1.2.0)
 SDL_LIBS="-Wl,-rpath,@loader_path/../Frameworks -framework AudioToolbox -framework AudioUnit -framework Cocoa -framework CoreAudio -framework IOKit -framework CoreFoundation -framework Carbon -framework CoreServices -framework ApplicationServices -framework Foundation -framework AppKit -framework OpenGL -framework SDL -framework SDL_mixer -framework SDL_image -framework SDL_ttf"
AC_SUBST(SDL_STATIC_PREFIX)
fi

AC_CONFIG_FILES([
                 Makefile
                 src/Makefile
                 data/Makefile
                 src/cpstamp.pc
                 po/Makefile.in
])

AC_OUTPUT
