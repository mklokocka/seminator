// Copyright (C) 2017, Fakulta Informatiky Masarykovy univerzity
//
// This file is a part of Seminator, a tool for semi-determinization of omega automata.
//
// Seminator is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Seminator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#pragma once

//#include <utility>
#include <stdexcept>
#include <unistd.h>
#include <limits>

#include <types.hpp>
#include <breakpoint_twa.hpp>

#include <spot/misc/bddlt.hh>


static const std::string VERSION_TAG = "v1.2.0dev";

static const unsigned TGBA = 0;
static const unsigned TBA = 1;
static const unsigned BA = 2;

bool cut_on_SCC_entry = false;
bool cut_always = false;
bool powerset_for_weak = false;
bool powerset_on_cut = false;
bool scc_aware = true;

/**
 * The semi-determinization algorithm as thought of by F. Blahoudek, J. Strejcek and M. Kretinsky.
 *
 * @param[in] aut TGBA to transform to a semi-deterministic TBA.
 */
aut_ptr buchi_to_semi_deterministic_buchi(aut_ptr aut, bool deterministic_first_component, bool optimization, unsigned output);

/**
 * Determinizes the first part of input. The first part is given by to_determinize
 * that can be obtained by `is_cut_deterministic`. Returns a new automaton.
 */
aut_ptr determinize_first_component(const_aut_ptr, state_set * to_determinize);


/**
 * A function that checks whether a given automaton is cut-deterministic.
 *
 * @param[in] aut               The automata to check for cut-determinism.
 * @param[out] non_det_states   Vector of the states that block cut-determinism.
 * @return Whether the automaton is cut-deterministic or not.
 */
bool is_cut_deterministic(const_aut_ptr aut, std::set<unsigned>* non_det_states = nullptr);

/**
* Returns whether a cut transition (jump to the deterministic component)
* for the current edge should be created.
*
* @param[in] aut                The input automaton_stream_parser
* @param[in] e                  The edge beeing processed
* @return True if some jump condition is satisfied
*
* Currently, 4 conditions trigger the jump:
*  1. If the edge has the highest mark
*  2. If we freshly enter accepting scc (--cut-on-SCC-entry option)
*  3. If e leads to accepting SCC (--cut-always option)
*/
bool cut_condition(const_aut_ptr aut, edge_t e) {
  spot::scc_info si(aut);
  unsigned u = si.scc_of(e.src);
  unsigned v = si.scc_of(e.dst);
  unsigned highest_mark(aut->acc().num_sets() - 1);

  return
    si.is_accepting_scc(v) && (
      e.acc.has(highest_mark) || //1
      (cut_on_SCC_entry && u != v) || // 2
      cut_always // 3
  );
}

void print_usage(std::ostream& os) {
  os << "Usage: seminator [OPTION...] [FILENAME]" << std::endl;
}

void print_help() {
  print_usage(std::cout);
  std::cout <<
"The tool transforms TGBA into equivalent semi- or cut-deterministic TBA.\n\n";

  std::cout <<
  "By default, it reads a generalized Büchi automaton (GBA) from standard input\n"
  "and converts it into semi-deterministic Büchi automaton (sDBA), runs\n"
  "Spot's simplifications on it and outputs the result in the HOA format.\n"
  "The main algorithms are based on breakpoint construction. If the automaton\n"
  "is already of the requested shape, only the simplifications are run.\n\n";

  std::cout << "Input options:\n";
  std::cout <<
  "    -f FILENAME\treads the input from FILENAME instead of stdin\n\n";

  std::cout << "Output options: \n"
  "    --cs   \tcut-deterministic automaton\n"
  "    --tgba \tTGBA output (default)\n"
  "    --tba\t\tTBA output\n"
  "    --ba \t\tSBA output\n"
  "    --is-cd\tdo not run the translation, check whether the input is \n"
  "           \tcut-deterministic. Outputs 1 if it is, 0 otherwise.\n"
  "           \t(Spot's autfilt offers --is-semideterministic check)\n\n";

  std::cout <<
  "Transformation type (T=transition-based, S=state-based): \n"
  "    --via-tgba\tone-step semi-determinization: TGBA -> sDBA\n"
  "    --via-tba\ttwo-steps: TGBA -> TBA -> sDBA\n"
  "    --via-sba\ttwo-steps: TGBA -> SBA -> sDBA\n\n"
  "  Multiple translation types can be chosen, the one with smallest\n"
  "  result will be outputed. If none is chosen, all three are run.\n\n";

  std::cout << "Cut-edges construction:\n"
  "    --cut-always      \tcut-edges for each edge to an accepting SCC\n"
  "    --cut-on-SCC-entry\tcut-edges also for edges freshly entering an\n"
  "                      \taccepting SCC\n"
  "    --powerset-on-cut \tcreate s -a-> (δ(s),δ_0(s),0) for s -a-> p\n\n"
  "  Cut-edges are edges between the 1st and 2nd component of the result.\n"
  "  They are based on edges of the input automaton. By default,\n"
  "  create cut-edges for edges with the highest mark, for edge\n"
  "  s -a-> p create cut-edge s -a-> ({p},∅,0).\n\n";

  std::cout << "Optimizations:\n"
  "  --powerset-for-weak\tavoid breakpoint construction for inherently weak\n" "                     \taccepting SCCs and use powerset construction instead\n"
  "  -s0                \tdisables Spot's automata reductions algorithms\n"
  "  --scc0             \tdisables scc-aware optimization\n\n";

  std::cout << "Miscellaneous options: \n"
  "  --help\tprint this help\n"
  "  --version\tprint program version" << std::endl;
}

/**
 * Class representing an exception thrown when the algorithm is not run on a proper SBwA automata.
 */
class not_tgba_exception: public std::runtime_error {
public:
    not_tgba_exception()
        : std::runtime_error("algorithm excepts a TGBA")
        {}
};

/**
 * Class representing an exception thrown when trying to copy to an automata with a different BDD dictionary.
 */
class mismatched_bdd_dict_exception: public std::runtime_error {
public:
    mismatched_bdd_dict_exception()
        : std::runtime_error("the source and destination automata require to have the same BDD dictionary")
        {}
};

/**
 * Class representing an exception thrown when the resulting automata is not semi-deterministic (this, of course, should not happen).
 */
class not_semi_deterministic_exception: public std::runtime_error {
public:
    not_semi_deterministic_exception()
        : std::runtime_error("the resulting automata is not semi-deterministic")
        {}
};

/**
 * Class representing an exception thrown when the resulting automata is not cut-deterministic (this, of course, should not happen).
 */
class not_cut_deterministic_exception: public std::runtime_error {
public:
    not_cut_deterministic_exception()
        : std::runtime_error("the resulting automata is not cut-deterministic")
        {}
};
