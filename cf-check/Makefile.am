#
#  Copyright 2021 Northern.tech AS
#
#  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; version 3.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
#
# To the extent this program is licensed as part of the Enterprise
# versions of CFEngine, the applicable Commercial Open Source License
# (COSL) may apply to this file if you as a licensee so wish it. See
# included file COSL.txt.
#
noinst_LTLIBRARIES = libcf-check.la

AM_CPPFLAGS = -I$(srcdir)/../libntech/libutils \
	-I$(srcdir)/../libntech/libcompat \
	@CPPFLAGS@ \
	$(PCRE_CPPFLAGS) \
	$(LIBYAML_CPPFLAGS) \
	$(LMDB_CPPFLAGS) \
	$(OPENSSL_CPPFLAGS)

AM_CFLAGS = \
	@CFLAGS@ \
	$(LMDB_CFLAGS) \
	$(PCRE_CFLAGS) \
	$(LIBYAML_CFLAGS) \
	$(PTHREAD_CFLAGS)

AM_LDFLAGS = \
	@LDFLAGS@ \
	$(PCRE_LDFLAGS) \
	$(LIBYAML_LDFLAGS) \
	$(LMDB_LDFLAGS)

libcf_check_la_LIBADD = ../libntech/libutils/libutils.la \
	$(LMDB_LIBS) \
	$(PCRE_LIBS) \
	$(LIBYAML_LIBS) \
	$(PTHREAD_LIBS) \
	$(OPENSSL_LIBS)

libcf_check_la_SOURCES = \
	backup.c backup.h \
	cf-check.c \
	diagnose.c diagnose.h \
	lmdump.c lmdump.h \
	db_structs.h \
	dump.c dump.h \
	utilities.c utilities.h \
	repair.c repair.h \
	replicate_lmdb.c replicate_lmdb.h \
	validate.c validate.h \
	observables.c observables.h

if !BUILTIN_EXTENSIONS
bin_PROGRAMS = cf-check

# Workaround for automake madness (try removing it if you want to know why).
cf_check_CFLAGS = $(AM_CFLAGS)

# Build both a libcf-check.la library, and a cf-check executable
cf_check_LDADD = libcf-check.la
endif

CLEANFILES = *.gcno *.gcda

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig *.rej
