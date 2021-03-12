## Copyright (C) 2019-2020  The Seminator Authors
##
## This file is a part of Seminator, a tool for semi-determinization
## of omega automata.
##
## Seminator is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Seminator is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS = seminator
lib_LTLIBRARIES = src/libseminator.la

AM_CPPFLAGS = -I$(srcdir)/src
seminator_LDADD = -L$(SPOTPREFIX)/lib src/libseminator.la -lspot -lbddx
src_libseminator_la_LIBADD = -L$(SPOTPREFIX)/lib -lspot -lbddx

src_libseminator_la_SOURCES =			\
  src/breakpoint_twa.cpp			\
  src/breakpoint_twa.hpp			\
  src/bscc.cpp					\
  src/bscc.hpp					\
  src/complement.cpp            \
  src/cutdet.cpp				\
  src/cutdet.hpp				\
  src/powerset.cpp				\
  src/powerset.hpp				\
  src/seminator.cpp				\
  src/seminator.hpp				\
  src/types.hpp

seminator_SOURCES = src/main.cpp

if USE_PYTHON
sempyexecdir = $(pyexecdir)/spot-extra
sempyexec_PYTHON = python/spot-extra/seminator.py
sempyexec_LTLIBRARIES = python/spot-extra/_seminator.la

# Debian has an inconsistency where it requires packages to be
# installed in .../dist-packages, but its sysconfig return
# sys-packages.  See
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=911038.  To work
# around this, we link dist-packages/spot-extra/ to
# site-package/spot-extra/ when installing in the latter and the
# former exists.
pyexecdir2 = $(pyexecdir:site-packages=dist-packages)
install-exec-local:
	-case "$(pyexecdir)" in *site-packages)						\
           test -d "$(pyexecdir2)" && ln -s $(sempyexecdir) $(pyexecdir2)/spot-extra;;	\
        esac
endif

MAINTAINERCLEANFILES = python/spot/seminator.cxx

## Automake 1.16 fails to remove the __pycache__/* files.
uninstall-local:
	rm -rf $(DESTDIR)$(pyexecdir)/spot-extra/__pycache__

python_spot_extra__seminator_la_CPPFLAGS = \
   -I$(PYTHONINC) -I$(top_srcdir)/src -DSWIG_TYPE_TABLE=spot
python_spot_extra__seminator_la_SOURCES = python/spot-extra/seminator_wrap.cxx
python_spot_extra__seminator_la_LDFLAGS = -avoid-version -module $(SYMBOLIC_LDFLAGS)
python_spot_extra__seminator_la_LIBADD = \
   src/libseminator.la -L$(SPOTPREFIX)/lib -lspot -lbddx

SWIGFLAGS = -c++ -python -py3 -O -I$(SPOTPREFIX)/include -I$(top_srcdir)/src

${srcdir}/python/spot-extra/seminator_wrap.cxx: python/spot-extra/seminator.i src/seminator.hpp
	$(SWIG) $(SWIGFLAGS) $(srcdir)/python/spot-extra/seminator.i
## work around https://github.com/swig/swig/issues/1553
	$(PERL) -pi -e 's/^#define SWIG_Python_CallFunctor.*/#define SWIG_Python_CallFunctor(functor, obj) PyObject_CallFunctionObjArgs(functor, obj, NULL);/' $@


AM_TESTS_ENVIRONMENT =						\
  PATH="$(abs_top_builddir):$$PATH"; export PATH;		\
  abs_top_srcdir="$(abs_top_srcdir)"; export abs_top_srcdir;

TEST_EXTENSIONS = .test .py .ipynb
PY_LOG_COMPILER = tests/pyrun
IPYNB_LOG_COMPILER = tests/pyrun

shell_TESTS =					\
  tests/batch.test				\
  tests/bscc-avoid.test				\
  tests/cut-on-scc-entry.test			\
  tests/complement.test				\
  tests/jump-to-bottommost.test			\
  tests/no-preprocess.test                      \
  tests/output.test				\
  tests/powerset-on-cut.test			\
  tests/reuse-deterministic.test		\
  tests/run_ltlcross.test			\
  tests/skip-levels.test			\
  tests/via.test

python_TESTS =					\
  tests/bindings.py

ipynb_TESTS =					\
  notebooks/bSCC.ipynb				\
  notebooks/Complement.ipynb			\
  notebooks/Cut-edges.ipynb			\
  notebooks/Example.ipynb			\
  notebooks/LevelSkipping.ipynb

TESTS = $(shell_TESTS)
if USE_PYTHON
  TESTS += $(python_TESTS) $(ipynb_TESTS)
endif

EXTRA_DIST =					\
  README.md					\
  workflow.svg					\
  ChangeLog.md					\
  formulae/random_sd.ltl			\
  formulae/random_nd.ltl			\
  python/spot-extra/seminator.i			\
  tests/ipnbdoctest.py				\
  $(shell_TESTS)				\
  $(ipynb_TESTS)				\
  $(python_TESTS)

