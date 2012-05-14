#  1999 06 06
#  Definicje INTELSWP    - tryb reprezetacji danych do procesorow INTEL'a
#  Definicja UNIXPATH    - / jako separotor scieszki
#  Definicja LINUX       - funkcje rozwijalne w mjejscu wywolania
#            MULTITHREAD - wersja wielowatkowa               
                     
LIBS            = -lm
DISTFILES       = ./SCRIPT/* ./OBJ/dummy ./BIN/dummy Makefile hmpp.pro \
	          phmpp.pro
TMAKE_LINK      = gcc
DESTDIR         = ./BIN
INCLUDEPATH     = .
TMAKE_LFLAGS    = -s
OBJECTS_DIR     = ./OBJ/
TEMPLATE	= app
CONFIG		= warn_on
TARGET          = phmpp
TMAKE_CFLAGS    = -O6 -pedantic -mpentium -DINTELSWP -DUNIXPATH \
                  -DLINUX -malign-double -fomit-frame-pointer -DMULTITHREAD

HEADERS		= asciitab.h \
		  formulc.h \
		  gif_hash.h \
		  gif_lib.h \
		  iobook.h \
		  proto.h \
		  rand2d.h \
		  shell.h \
		  typdef.h \
		  wigmap.h
SOURCES		= egif_lib.c \
		  fft.c \
		  formulc.c \
		  gif_err.c \
		  gif_hash.c \
		  iobook.c \
		  iosignal.c \
		  main.c \
		  mceeg.c \
		  mp.c \
		  rand2d.c \
		  shell.c \
		  sourtime.c \
		  utils.c \
		  wig2gif.c \
		  wigmap.c \
		  wigmapps.c











