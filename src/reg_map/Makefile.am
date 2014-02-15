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

noinst_LTLIBRARIES = libregmap.la

libregmap_la_SOURCES = \
	fmc_config_130m_4ch.cpp

libregmap_la_LIBADD = @LTLIBOBJS@

AM_CPPFLAGS = \
	-I. \
	-I$(top_srcdir)/src/reg_map \
	-I$(top_srcdir)/src/reg_map/mod_regs
