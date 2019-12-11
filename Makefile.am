

bin_PROGRAMS = seminator
lib_LTLIBRARIES = libseminator.la

AM_CPPFLAGS = -I$(srcdir)/src
seminator_LDADD = -L$(SPOTPREFIX)/lib libseminator.la -lspot -lbddx
libseminator_la_LIBADD = -L$(SPOTPREFIX)/lib -lspot -lbddx

libseminator_la_SOURCES =			\
  src/breakpoint_twa.cpp			\
  src/breakpoint_twa.hpp			\
  src/bscc.cpp					\
  src/bscc.hpp					\
  src/cutdet.cpp				\
  src/cutdet.hpp				\
  src/powerset.cpp				\
  src/powerset.hpp				\
  src/seminator.cpp				\
  src/seminator.hpp				\
  src/types.hpp

seminator_SOURCES = src/main.cpp

AM_TESTS_ENVIRONMENT =						\
  PATH="$(abs_top_builddir):$$PATH"; export PATH;		\
  abs_top_srcdir="$(abs_top_srcdir)"; export abs_top_srcdir;

TESTS =						\
  tests/bscc-avoid.test				\
  tests/cut-on-scc-entry.test			\
  tests/powerset-on-cut.test			\
  tests/reuse-good-SCC.test			\
  tests/skip-levels.test			\
  tests/run_ltlcross.test

EXTRA_DIST =					\
  README.md					\
  ChangeLog.md					\
  formulae/random_sd.ltl			\
  formulae/random_nd.ltl			\
  $(TESTS)

