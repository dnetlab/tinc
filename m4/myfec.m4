dnl Check to find the curses headers/libraries

AC_DEFUN([tinc_MYFEC],
[
    AC_ARG_WITH(myfec,
      AS_HELP_STRING([--with-myfec=DIR], [myfec base directory, or:]),
      [myfec="$withval"
       CPPFLAGS="$CPPFLAGS -I$withval/include"
       LDFLAGS="$LDFLAGS -L$withval/lib"]
    )

    AC_ARG_WITH(myfec-include,
      AS_HELP_STRING([--with-myfec-include=DIR], [myfec headers directory]),
      [myfec_include="$withval"
       CPPFLAGS="$CPPFLAGS -I$withval"]
    )

    AC_ARG_WITH(myfec-lib,
      AS_HELP_STRING([--with-myfec-lib=DIR], [myfec library directory]),
      [myfec_lib="$withval"
       LDFLAGS="$LDFLAGS -L$withval"]
    )

    AC_CHECK_HEADERS(myfec.h,
      [],
      [AC_MSG_ERROR("myfec header files not found."); break]
    )

    AC_CHECK_LIB(myfec, myfec_init,
      [LIBS="-lmyfec $LIBS"],
      [AC_MSG_ERROR("myfec libraries not found.")]
    )

  AC_SUBST(MYFEC_LIBS)
])
