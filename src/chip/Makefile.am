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

noinst_LTLIBRARIES = libchip.la

libchip_la_SOURCES = \
	ad9510.cpp \
	ad9510.h \
	amc7823.cpp \
	amc7823.h \
	isla216p.cpp \
	isla216p.h \
	lm75a.cpp \
	lm75a.h \
	si570.cpp \
	si570.h \
	eeprom_24a64.cpp \
	eeprom_24a64.h

libchip_la_LIBADD = @LTLIBOBJS@

AM_CPPFLAGS = \
	-I. \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/commlink
