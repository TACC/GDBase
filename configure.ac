AC_INIT(Makefile.in)
AC_PROG_CC
AC_CONFIG_HEADERS
AC_PATH_PROG(GDB, gdb)
AC_ARG_WITH(tcl,
[--with-tcl=version - tcl version],,
with_tcl=8.5)
AC_SUBST([TCL_VERSION],[$with_tcl])
AC_CHECK_FUNC(getline,,AC_SUBST(GETLINE_OBJ,[getline.o]))
AC_OUTPUT(makefile:Makefile.in)

