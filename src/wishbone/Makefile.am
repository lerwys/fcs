## Makefile.am -- Process this file with automake to produce Makefile.in
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

noinst_LTLIBRARIES = libwishbone.la

libwishbone_la_SOURCES = \
	rs232_syscon.cpp \
	rs232_syscon.h   \
	pcie_link.cpp \
	pcie_link.h

libwishbone_la_LIBADD = -lmxml -lserial -lpcidriver -lpthread @LTLIBOBJS@
#libwishbone_la_LIBADD = @LTLIBOBJS@

AM_CPPFLAGS = \
	-I. \
	-I$(top_srcdir)/src/common/ \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/wishbone/pcie_headers
