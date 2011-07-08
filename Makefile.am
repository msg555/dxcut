AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
LIBRARY_VERSION = 1:1:0

lib_LTLIBRARIES = libdxcut.la libdxcutcc.la

libdxcut_la_LDFLAGS = -version-info $(LIBRARY_VERSION)
libdxcut_la_SOURCES = \
  src/access_flags.c \
  src/annotations.c \
  src/aux.c \
  src/classes.c \
  src/code.c \
  src/common.c \
  src/dalvik.c \
  src/debug.c \
  src/fields.c \
  src/file.c \
  src/handler.c \
  src/inline.c \
  src/methods.c \
  src/mutf8.c \
  src/protos.c \
  src/read.c \
  src/strings.c \
  src/try_block.c \
  src/types.c \
  src/values.c \
  src/write.c \
  src/util.c \
  src/annotations.h \
  src/aux.h \
  src/classes.h \
  src/code.h \
  src/common.h \
  src/dalvik.h \
  src/debug.h \
  src/fields.h \
  src/file.h \
  src/methods.h \
  src/mutf8.h \
  src/opt_write.h \
  src/protos.h \
  src/read.h \
  src/strings.h \
  src/types.h \
  src/values.h

libdxcut_la_includedir=$(includedir)/dxcut
libdxcut_la_include_HEADERS = \
  dxcut/access_flags.h \
  dxcut/annotation.h \
  dxcut/cc.h \
  dxcut/class.h \
  dxcut/code.h \
  dxcut/dalvik.h \
  dxcut/debug_info.h \
  dxcut/dex.h \
  dxcut/dxcut.h \
  dxcut/field.h \
  dxcut/file.h \
  dxcut/handler.h \
  dxcut/inline.h \
  dxcut/method.h \
  dxcut/try_block.h \
  dxcut/value.h \
  dxcut/util.h

libdxcutcc_la_LDFLAGS = -version-info $(LIBRARY_VERSION)
libdxcutcc_la_SOURCES = \
  $(libdxcut_la_SOURCES) \
  src/cc.cpp
libdxcutcc_la_includedir=$(includedir)/dxcut
libdxcutcc_la_include_HEADERS = \
  dxcut/cdxcut
