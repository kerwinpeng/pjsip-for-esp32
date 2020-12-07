#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS :=include include/pjmedia include/pjmedia-audiodev include/pjmedia-codec \
        include/pjmedia-videodev src/pjmedia src/pjmedia-audiodev src/pjmedia-codec/g722

COMPONENT_SRCDIRS := src/pjmedia src/pjmedia-audiodev src/pjmedia-codec src/pjmedia-codec/g722 src/pjmedia-videodev

CFLAGS +=-D_POSIX_THREAD_PRIORITY_SCHEDULING