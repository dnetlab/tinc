dnl Check to find the lz4 headers/libraries

AC_DEFUN([tinc_LZ4],
[
  AC_ARG_ENABLE([lz4],
    AS_HELP_STRING([--disable-lz4], [disable lz4 compression support]))
  AS_IF([test "x$enable_lz4" != "xno"], [
    AC_DEFINE(HAVE_LZ4, 1, [enable lz4 compression support])
    AC_ARG_WITH(lz4,
      AS_HELP_STRING([--with-lz4=DIR], [lz4 base directory, or:]),
      [lz4="$withval"
       CPPFLAGS="$CPPFLAGS -I$withval/include"
       LDFLAGS="$LDFLAGS -L$withval/lib"]
    )

    AC_ARG_WITH(lz4-include,
      AS_HELP_STRING([--with-lz4-include=DIR], [lz4 headers directory]),
      [lz4_include="$withval"
       CPPFLAGS="$CPPFLAGS -I$withval"]
    )

    AC_ARG_WITH(lz4-lib,
      AS_HELP_STRING([--with-lz4-lib=DIR], [lz4 library directory]),
      [lz4_lib="$withval"
       LDFLAGS="$LDFLAGS -L$withval"]
    )

    AC_CHECK_LIB(lz4, LZ4_compress_default,
      [LIBS="$LIBS -llz4"],
      [AC_CHECK_LIB(lz4, LZ4_compress_default,
        [LIBS="$LIBS -llz4"],
        [AC_MSG_ERROR("lz4 libraries not found."); break]
      )]
    )

    AC_CHECK_HEADERS(lz4.h,
      [AC_DEFINE(LZ4_H, [<lz4.h>], [Location of lz4.h])],
      [AC_CHECK_HEADERS(lz4.h,
        [AC_DEFINE(LZ4_H, [<lz4.h>], [Location of lz4.h])],
        [AC_CHECK_HEADERS(lz4.h,
          [AC_DEFINE(LZ4_H, [<lz4.h>], [Location of lz4.h])],
          [AC_MSG_ERROR("lz4 header files not found."); break]
        )]
      )]
    )
  ])
])
