#
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS :=include crypto/include

COMPONENT_SRCDIRS :=crypto/cipher crypto/hash crypto/kernel crypto/math crypto/replay pjlib srtp

CFLAGS +=-D_POSIX_THREAD_PRIORITY_SCHEDULING