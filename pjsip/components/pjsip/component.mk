#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS :=include include/pjsip include/pjsip-simple include/pjsip-ua include/pjsua-lib

COMPONENT_SRCDIRS := src/pjsip src/pjsip-simple src/pjsip-ua src/pjsua-lib

CFLAGS +=-D_POSIX_THREADS -D_POSIX_THREAD_PRIORITY_SCHEDULING

CPPFLAGS +=-fexceptions