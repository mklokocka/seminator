// Copyright (C) 2017, 2019, Fakulta Informatiky Masarykovy univerzity
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

#include <set>
#include <spot/twaalgos/postproc.hh>
#include <spot/misc/optionmap.hh>

enum { Onestep = 1,
       ViaTBA = 2,
       ViaSBA = 4,
       AllJobs = Onestep | ViaTBA | ViaSBA};
typedef int jobs_type;

enum output_type : int {TGBA = 0, TBA = 1, BA = 2};

/**
* Checks the input automaton if it is of requested type and returns it back.
* If not, checks for easy cases first and only after that runs seminator.
*/
spot::twa_graph_ptr check_and_compute(spot::twa_graph_ptr aut, jobs_type jobs,
                                      const spot::option_map* opt);

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
  seminator(spot::twa_graph_ptr input,
            jobs_type jobs, const spot::option_map* opt = nullptr) :
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

  seminator(spot::twa_graph_ptr input, const spot::option_map* opt = nullptr) :
    seminator(input, AllJobs, opt) {};

  /**
  * Run the algorithm for all jobs and returns the smallest automaton
  *
  * @param[in] jobs may specify more jobs, 0 (default) means jobs_
  */
  spot::twa_graph_ptr run(jobs_type jobs = 0);

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

  spot::twa_graph_ptr input_;
  jobs_type jobs_;
  const spot::option_map* opt_;

  // Intermediate results
  typedef std::map<jobs_type, spot::twa_graph_ptr> aut_ptr_dict;
  aut_ptr_dict inputs_ = aut_ptr_dict();
  aut_ptr_dict results_ = aut_ptr_dict();
  spot::twa_graph_ptr best_ = nullptr;

  // Simplifications options (from opt, see parse_options for defaults)
  bool postproc_;
  bool preproc_;
  bool cut_det_;

  // Prefered output types
  output_type output_;

  // Spot's postprocesssor
  spot::postprocessor postprocessor_;
  spot::postprocessor preprocessor_;
};
