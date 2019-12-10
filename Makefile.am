

bin_PROGRAMS = seminator

seminator_LDADD = -L$(SPOTPREFIX)/lib -lspot -lbddx

seminator_SOURCES =				\
  breakpoint_twa.cpp				\
  breakpoint_twa.hpp				\
  bscc.cpp					\
  bscc.hpp					\
  cutdet.cpp					\
  cutdet.hpp					\
  powerset.cpp					\
  powerset.hpp					\
  seminator.cpp					\
  seminator.hpp					\
  types.hpp

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

