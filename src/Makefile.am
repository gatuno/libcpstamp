# Automake file for LibCPStamp

gamedatadir = $(pkgdatadir)/data

lib_LTLIBRARIES = libcpstamp.la
libcpstamp_la_SOURCES = cpstamp.c cpstamp.h path.c path.h gettext.h

libcpstampdir = $(includedir)/libcpstamp
libcpstamp_HEADERS = cpstamp.h

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = cpstamp.pc

libcpstamp_la_CPPFLAGS = -DGAMEDATA_DIR=\"$(gamedatadir)/\" -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
libcpstamp_la_CFLAGS = $(SDL_CFLAGS) $(SDL_image_CFLAGS) $(SDL_mixer_CFLAGS) $(SDL_ttf_CFLAGS) $(AM_CFLAGS)
if MACOSX
# En MAC OS X, hay que ligar/compilar contra los frameworks
libcpstamp_la_LIBADD = $(SDL_LIBS)
else
libcpstamp_la_LIBADD = $(SDL_LIBS) $(SDL_image_LIBS) $(SDL_mixer_LIBS) $(SDL_ttf_LIBS)
endif
libcpstamp_la_LDFLAGS = -version-info 3:0:0 -no-undefined
LDADD = $(LIBINTL)
