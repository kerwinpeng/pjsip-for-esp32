#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS :=include include/pj include/pj/compat include/pj++ src/pj

COMPONENT_SRCDIRS := src/pj/compat src/pj

CFLAGS +=-D_POSIX_THREADS -D_POSIX_THREAD_PRIORITY_SCHEDULING \
        -DPJ_HAS_SETJMP_H -DPJ_HAS_STRING_H -DPJ_HAS_TIME_H