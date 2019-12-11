# -*- coding: utf-8 -*-
# Copyright (C) 2019, FIXME
#
# This file is part of Seminator, a tool for semi-determinization of
# omega automata.
#
# TCLTL is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# TCLTL is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([m4_default([$5], [AC_LANG_PROGRAM()])],
    [AS_VAR_SET(CACHEVAR,[yes])],
    [AS_VAR_SET(CACHEVAR,[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_IF([test x"AS_VAR_GET(CACHEVAR)" = xyes], [$2], [$3])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

AC_PREREQ([2.63])
AC_INIT([seminator], [1.99])
AC_CONFIG_AUX_DIR([tools])
AM_INIT_AUTOMAKE([1.11 gnu tar-ustar color-tests parallel-tests
                  subdir-objects foreign])
AC_CONFIG_HEADERS([config.h])dnl Private config, not to be used in .hh files.

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
# Decrease verbosity when passing argument V=0
AM_SILENT_RULES([no])
AC_LANG(C++)

# Turn on C++14 support
m4_define([_AX_CXX_COMPILE_STDCXX_14_testbody],
[AC_LANG_SOURCE([#include <memory>
#include <string>
#include <chrono>  // used to fail in C++11 with some installation of clang
#include <map>

    template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    typedef check<check<bool>> right_angle_brackets;

    auto f = std::make_unique<std::string>("uniq_ptr");

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);

    auto d = a;
])])

for f in -std=c++14 '-std=c++14 -stdlib=libc++'
do
  AX_CHECK_COMPILE_FLAG([$f], [CXXFLAGS="$CXXFLAGS $f" stdpass=true], [], [],
                        [_AX_CXX_COMPILE_STDCXX_14_testbody])
  ${stdpass-false} && break
done
if ! "${stdpass-false}"; then
  AC_ERROR([unable to turn on C++14 mode with this compiler])
fi

AC_ARG_WITH([spot],
  [AS_HELP_STRING([--with-spot=PREFIXDIR],
    [assume Spot has been installed in PREFIXDIR @<:@default to --prefix@:>@])],
  [], [with_spot='${prefix}'])
AC_SUBST([SPOTPREFIX], [$with_spot])

LT_INIT

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
