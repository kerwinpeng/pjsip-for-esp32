#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := voipdemo

include $(ADF_PATH)/project.mk

CFLAGS += -DESP_PLATFORM -D__XTENSA__ -DPJ_SOCK_HAS_INET_NTOP -D_POSIX_THREADS -D_POSIX_THREAD_PRIORITY_SCHEDULING  \
        -DPJ_HAS_ASSERT_H -DPJ_HAS_SETJMP_H -DPJ_HAS_STRING_H -DPJ_HAS_CTYPE_H \
        -DPJ_HAS_SYS_TIME_H -DPJ_HAS_ERRNO_H -DPJ_HAS_SYS_TIMEB_H \
        -DPJ_HAS_LIMITS_H -DPJ_HAS_SEMAPHORE_H -DPJ_HAS_FLOATING_POINT -DPJ_HAS_UNISTD_H
