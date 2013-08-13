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

# the program to build (the names of the final binaries)
bin_PROGRAMS = \
	fmc_config_130m_4ch \
	fmc_config_130m_4ch_crystek \
	fmc_config_130m_4ch_passive \
	fmc_config_250m_4ch \
	fmc_config_250m_4ch_passive

# list of sources
fmc_config_130m_4ch_SOURCES = \
	fmc_config_130m_4ch.cpp

fmc_config_130m_4ch_crystek_SOURCES = \
	fmc_config_130m_4ch_crystek.cpp

fmc_config_130m_4ch_passive_SOURCES = \
	fmc_config_130m_4ch_passive.cpp

fmc_config_250m_4ch_SOURCES = \
	fmc_config_250m_4ch.cpp

fmc_config_250m_4ch_passive_SOURCES = \
	fmc_config_250m_4ch_passive.cpp

LDADD = \
	$(top_builddir)/src/chip/libchip.la \
	$(top_builddir)/src/commlink/libcommlink.la \
	$(top_builddir)/src/interface/libinterface.la \
	$(top_builddir)/src/wishbone/libwishbone.la \
	$(top_builddir)/src/common/libcommon.la

AM_CPPFLAGS = \
	-I. \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/reg_map \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/commlink

# install these headers
nobase_include_HEADERS = \
	reg_map/fmc_config_130m_4ch.h \
	reg_map/fmc_config_250m_4ch.h

include_HEADERS = \
	include/platform/fmc130m_plat.h \
	include/platform/fmc250m_plat.h

# don't install these headers
noinst_HEADERS = \
	include/data.h \
	include/wbint_drv.h \
	include/wbmaster_unit.h
