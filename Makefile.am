ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall
AM_CFLAGS	=
AM_CXXFLAGS = -std=c++0x
AM_LDFLAGS	= -no-undefined

lib_LTLIBRARIES = libbitstreamxx.la
libbitstreamxx_la_SOURCES = ./src/bitstream.cpp

check_PROGRAMS = \
				 test1 \
				 test2 \
				 test3

test1_SOURCES	= ./tests/test1.cpp
test1_LDADD		= libbitstreamxx.la

test2_SOURCES	= ./tests/test2.cpp
test2_LDADD		= libbitstreamxx.la

test3_SOURCES	= ./tests/test3.cpp
test3_LDADD		= libbitstreamxx.la

TESTS = $(check_PROGRAMS)
