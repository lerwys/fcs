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

noinst_LTLIBRARIES = libtcpserver.la

libtcpserver_la_SOURCES = \
	tcp_server.cpp \
	tcp_server.h

libtcpserver_la_LIBADD = \
	$(top_builddir)/src/fmc_config/libfmcconfig.la \
	-lbsmp
	@LTLIBOBJS@

AM_CPPFLAGS = \
	-I. \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/fmc_config \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/reg_map \
	-I$(top_srcdir)/src/reg_map/mod_regs \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/commlink
