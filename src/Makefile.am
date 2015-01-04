# Automake file for LibCPStamp

gamedatadir = $(pkgdatadir)/data

lib_LTLIBRARIES = libcpstamp.la
libcpstamp_la_SOURCES = cpstamp.c cpstamp.h

libcpstampdir = $(includedir)/libcpstamp
libcpstamp_HEADERS = cpstamp.h

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = cpstamp.pc

libcpstamp_la_CPPFLAGS = -DGAMEDATA_DIR=\"$(gamedatadir)/\" -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
libcpstamp_la_CFLAGS = $(SDL_CFLAGS) $(SDL_image_CFLAGS) $(SDL_mixer_CFLAGS) $(SDL_ttf_LIBS) $(AM_CFLAGS)
libcpstamp_la_LIBADD = $(SDL_LIBS) $(SDL_image_LIBS) $(SDL_mixer_LIBS) $(SDL_ttf_LIBS)
libcpstamp_la_LDFLAGS = -version-info 1:0:0

