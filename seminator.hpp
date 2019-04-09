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
#include <cutdet.hpp>
#include <breakpoint_twa.hpp>

#include <spot/misc/bddlt.hh>
#include <spot/twaalgos/postproc.hh>


static const std::string VERSION_TAG = "v1.2.0dev";

/**
* Returns whether a cut transition (jump to the deterministic component)
* for the current edge should be created.
*
* @param[in] aut                The input automaton_stream_parser
* @param[in] e                  The edge beeing processed
* @param[in] om                 Options map
* @return True if some jump condition is satisfied
*
* Currently, 4 conditions trigger the jump:
*  1. If the edge has the highest mark
*  2. If we freshly enter accepting scc (--cut-on-SCC-entry option)
*  3. If e leads to accepting SCC (--cut-always option)
*/
bool cut_condition(const_aut_ptr aut, edge_t e, const_om_ptr om = nullptr) {
  spot::scc_info si(aut);
  bool cut_on_SCC_entry = false;
  bool cut_always = false;
  if (om)
  {
    cut_on_SCC_entry = om->get("cut-on-SCC-entry", 0);
    cut_always = om->get("cut-always", 0);
  }
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
  "    --cd   \tcut-deterministic automaton\n"
  "    --sd   \tsemi-deterministic automaton (default)\n\n"
  "    --ba   \tSBA output\n"
  "    --tba  \tTBA output\n"
  "    --tgba \tTGBA output (default)\n\n"

  "    --is-cd\tdo not run transformation, check whether input is \n"
  "           \tcut-deterministic. Outputs 1 if it is, 0 otherwise.\n"
  "           \t(Spot's autfilt offers --is-semideterministic check)\n\n";

  std::cout <<
  "Transformation type (T=transition-based, S=state-based): \n"
  "    --via-tgba\tone-step semi-determinization: TGBA -> sDBA\n"
  "    --via-tba\ttwo-steps: TGBA -> TBA -> sDBA\n"
  "    --via-sba\ttwo-steps: TGBA -> SBA -> sDBA\n\n"
  "  Multiple translation types can be chosen, the one with smallest\n"
  "  result will be outputted. If none is chosen, all three are run.\n\n";

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
  "    --powerset-for-weak    \tavoid breakpoint construction for\n"
  "                           \tinherently weak accepting SCCs and use\n"
  "                           \tpowerset construction instead\n"
  "    -s0,    --no-reductions\tdisable Spot automata post-reductions\n"
  "    --scc-aware            \tenable scc-aware optimization (default on)\n"
  "    --scc0, --no-scc-aware \tdisable scc-aware optimization\n\n";

  std::cout << "Miscellaneous options: \n"
  "  --help\tprint this help\n"
  "  --version\tprint program version" << std::endl;
}

/**
 * Class running possible multiple types of the transformation
 * and returns the best result. It also handles pre- and post-
 * processing of the automata.
 */
class seminator {
public:
  /**
  * Constructor for seminator.
  *
  * @param[in] input the input automaton
  * @param[in] jobs the jobs to be performed
  * @param[in] opt (optinial, nullptr) options that tweak transformations
  */
  seminator(aut_ptr input,
            jobs_type jobs, const_om_ptr opt = nullptr) :
            input_(input), jobs_(jobs), opt_(opt)
  {
    if (!opt)
      opt_ = new const spot::option_map;
    parse_options();

    // Set postprocess options that preserve cut-determinism
    if (cut_det_)
    {
      static spot::option_map extra_options;
      extra_options.set("ba_simul",1);
      extra_options.set("simul",1);
      postprocessor_ = spot::postprocessor(&extra_options);
    }
  }

  seminator(aut_ptr input, const_om_ptr opt = nullptr) :
    seminator(input, AllJobs, opt) {};

  /**
  * Run the algorithm for all jobs and returns the smallest automaton
  *
  * @param[in] jobs may specify more jobs, 0 (default) means jobs_
  */
  aut_ptr run(jobs_type jobs = 0);

  /**
  * Run requested jobs
  *
  * @param[in] jobs may specify more jobs, 0 (default) means jobs_
  */
  void run_jobs(jobs_type jobs = 0);

  /**
  * Choose best from given jobs. Run the algo if not done before.
  *
  * @param[in] jobs may specify more jobs, 0 (default) means jobs_
  * Returns the job with lowest number of states
  */
  jobs_type best_from(jobs_type jobs = 0);


  /**
  * Return the automaton of the job.
  *
  * @param[in] jobs must be an unique job index
  */
  aut_ptr get(jobs_type job);

private:
  /**
  * Read options from opt into variables
  */
  void parse_options();

  /**
  * Prepare input for requested jobs.
  *
  * @param[in] jobs may specify more jobs, 0 (default) means jobs_
  */
  void prepare_inputs(jobs_type jobs = 0);

  /**
  * Decide what jobs are reasonable
  */
  void choose_jobs();

  aut_ptr input_;
  jobs_type jobs_;
  const_om_ptr opt_;

  // Intermediate results
  typedef std::map<jobs_type, aut_ptr> aut_ptr_dict;
  aut_ptr_dict inputs_ = aut_ptr_dict();
  aut_ptr_dict results_ = aut_ptr_dict();
  aut_ptr best_ = nullptr;

  // Simplifications options (from opt, see parse_options for defaults)
  bool postproc_;
  bool cut_det_;

  // Prefered output types
  output_type output_;

  // Spot's postprocesssor
  spot::postprocessor postprocessor_;
};
