// Copyright (c) 2017-2020  The Seminator Authors
//
// This file is a part of Seminator, a tool for semi-determinization
// of omega automata.
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

#include <seminator.hpp>

#include <cutdet.hpp>
#include <bscc.hpp>
#include <breakpoint_twa.hpp>

#include <spot/twaalgos/degen.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/sccinfo.hh>
#include <spot/twaalgos/minimize.hh>
#include <spot/misc/optionmap.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twa/bddprint.hh>

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
  seminator(spot::twa_graph_ptr input, bool cut_det,
            const spot::option_map* opt = nullptr)
    : input_(spot::scc_filter(input, true)), opt_(opt), cut_det_(cut_det)
  {
    if (!opt)
      opt_ = new const spot::option_map;

    preproc_  = opt_->get("preprocess",0);
    postproc_ = opt_->get("postprocess", 1);

    if (preproc_)
      preprocessor_.set_pref(spot::postprocessor::Deterministic);
    else if (input_->acc().is_all())
      // automata with "t" acceptance can be determinized and minimized
      // as a DFA.  (The preprocessor will do that if we use it.)
      input_ = spot::minimize_monitor(input_);

    output_  = static_cast<output_type>(opt_->get("output", TGBA));

    // Set postprocess options that preserve cut-determinism
    if (cut_det)
    {
      static spot::option_map extra_options;
      extra_options.set("ba_simul",1);
      extra_options.set("simul",1);
      postprocessor_ = spot::postprocessor(&extra_options);
    }
  }

  /**
  * Run the algorithm for all jobs and returns the smallest automaton
  *
  * @param[in] jobs may specify more jobs, 0 (default) means AllJobs.
  */
  spot::twa_graph_ptr run(jobs_type jobs)
  {
    if (jobs == 0)
      jobs = AllJobs;

    // If the input is already Buchi, there there is no need
    // for a TGBA job.
    if (input_->acc().is_buchi() && (jobs & (ViaTBA|ViaSBA)))
      {
        jobs &= ~ViaTGBA;
        // And if the input is already state-based Buchi, there there
        // is no need for a TBA job.
        if (input_->prop_state_acc() && (jobs & ViaSBA))
          jobs &= ~ViaTBA;
      }

    spot::twa_graph_ptr best = nullptr;
    for (auto job : {ViaTGBA, ViaTBA, ViaSBA})
      if (job & jobs)
        {
          auto result = postprocess_job(process_job(prepare_input(job)));
          if (!best || (best->num_states() > result->num_states()))
            best = result;
        }
    return best;
  }

private:

  spot::twa_graph_ptr prepare_input(jobs_type job)
  {
    switch (job)
      {
      case ViaTGBA:
        if (!preproc_)
          {
            return input_;
          }
        preprocessor_.set_type(spot::postprocessor::TGBA);
        return preprocessor_.run(input_);
      case ViaTBA:
        {
          auto out = spot::degeneralize_tba(input_);
          if (preproc_)
            {
              preprocessor_.set_type(spot::postprocessor::TGBA);
              out = preprocessor_.run(out);
            }
          return out;
        }
      case ViaSBA:
        if (!preproc_)
          {
            return spot::degeneralize(input_);
          }
        else
          {
            preprocessor_.set_type(spot::postprocessor::BA);
            return preprocessor_.run(input_);
          }
      default:
        assert(!"should not be reached");
      }
  }

  spot::twa_graph_ptr process_job(spot::twa_graph_ptr input)
  {
    spot::twa_graph_ptr result;
    state_set non_det_states;

    if (spot::is_deterministic(input) ||
        is_cut_deterministic(input, &non_det_states))
      {
        result = input;
      }
    else if (spot::is_semi_deterministic(input))
      {
        if (!cut_det_)
          result = input;
        else
          result = determinize_first_component(input, &non_det_states);
      }
    else
      {
        // Run the breakpoint algorithm
        bp_twa resbp(input, cut_det_, opt_);
        result = resbp.res_aut();
        result->purge_dead_states();
      }

    // Check the result
    if (!cut_det_)
      assert(spot::is_semi_deterministic(result));
    else
      assert(is_cut_deterministic(result));
    assert(result->acc().is_generalized_buchi());

    result->prop_semi_deterministic(true);
    return result;
  }

  spot::twa_graph_ptr postprocess_job(spot::twa_graph_ptr aut)
  {
    if (postproc_)
      {
        postprocessor_.set_type(spot::postprocessor::TGBA);
        aut = postprocessor_.run(aut);
      }

    switch (output_)
      {
      case TBA:
        aut = spot::degeneralize_tba(aut);
        if (postproc_)
          aut = postprocessor_.run(aut);
        break;
      case BA:
        if (postproc_)
          {
            postprocessor_.set_type(spot::postprocessor::BA);
            aut = postprocessor_.run(aut);
          }
        else
          {
            aut = spot::degeneralize(aut);
          }
        break;
      default:
        break;
      }
    return aut;
  }

  spot::twa_graph_ptr input_;
  const spot::option_map* opt_;

  // Simplifications options
  bool postproc_;
  bool preproc_;
  bool cut_det_;

  // Prefered output types
  output_type output_;

  // Spot's postprocesssor
  spot::postprocessor postprocessor_;
  spot::postprocessor preprocessor_;
};

aut_ptr semi_determinize(aut_ptr aut,
                         bool cut_det,
                         jobs_type jobs,
                         const_om_ptr opt)
{
  seminator sem(aut, cut_det, opt);
  return sem.run(jobs);
}
