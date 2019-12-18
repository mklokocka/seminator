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

#include <seminator.hpp>

#include <cutdet.hpp>
#include <bscc.hpp>
#include <breakpoint_twa.hpp>

#include <spot/twaalgos/stripacc.hh>
#include <spot/twaalgos/degen.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/sccinfo.hh>
#include <spot/misc/optionmap.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twa/bddprint.hh>

static constexpr jobs_type unitjobs[3] = {ViaTGBA, ViaTBA, ViaSBA};

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
    : input_(input), opt_(opt), cut_det_(cut_det)
  {
    if (!opt)
      opt_ = new const spot::option_map;

    preproc_  = opt_->get("preprocess",0);
    postproc_ = opt_->get("postprocess", 1);

    // The deterministic attempt was done in semi_determinize
    if (preproc_)
      preprocessor_.set_pref(spot::postprocessor::Small);

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
    prepare_inputs(jobs);
    return results_[best_from(jobs)];
  }

  /**
  * Run requested jobs
  *
  * @param[in] jobs specifies the jobs to run
  */
  void run_jobs(jobs_type jobs)
  {
    for (auto job : unitjobs)
      {
        if (job & jobs)
          {
            // Prepare the desired input
            if (!inputs_[job])
              prepare_inputs(job);
            assert(inputs_[job]);

            state_set non_det_states;
            if (spot::is_deterministic(inputs_[job]) ||
                is_cut_deterministic(inputs_[job], &non_det_states))
              results_[job] = inputs_[job];
            else if (spot::is_semi_deterministic(inputs_[job]))
              {
                if (!cut_det_)
                  results_[job] = inputs_[job];
                else
                  results_[job] = determinize_first_component(inputs_[job],
                                                              &non_det_states);
              }
            else
              {
                // Run the breakpoint algorithm
                bp_twa resbp(inputs_[job], cut_det_, opt_);
                results_[job] = resbp.res_aut();
                results_[job]->purge_dead_states();
              }

            // Check the result
            assert(spot::is_semi_deterministic(results_[job]));
            assert(!cut_det_ || is_cut_deterministic(results_[job]));
            assert(results_[job]->acc().is_generalized_buchi());

            if (postproc_)
              results_[job] = postprocessor_.run(results_[job]);

            switch (output_)
              {
              case TBA:
                results_[job] = spot::degeneralize_tba(results_[job]);
                if (postproc_)
                  results_[job] = postprocessor_.run(results_[job]);
                break;
              case BA:
                results_[job] = spot::degeneralize(results_[job]);
                if (postproc_)
                  results_[job] = postprocessor_.run(results_[job]);
                break;
              default:
                break;
              }
          }
      }
  }

  /**
  * Choose best from given jobs. Run the algo if not done before.
  *
  * @param[in] jobs specifies the jobs to tests
  */
    jobs_type best_from(jobs_type jobs)
    {
      jobs_type res = 0;
      for (auto job : unitjobs)
        {
          if (job & jobs)
            {
              if (!results_[job])
                run_jobs(job);
              if (!res ||
                  (results_[res]->num_states() > results_[job]->num_states()))
                res = job;
            }
        }
      return res;
    }


private:
  /**
  * Read options from opt into variables
  */
  void parse_options();

  /**
  * Prepare input for requested jobs.
  *
  * @param[in] jobs specifies the jobs to run
  */
  void prepare_inputs(jobs_type jobs)
  {
    // TODO check what makes sense
    if (jobs & ViaTGBA)
      inputs_.emplace(ViaTGBA, input_);
    if (jobs & ViaTBA)
      {
        inputs_.emplace(ViaTBA, spot::degeneralize_tba(input_));
        if (preproc_)
          inputs_[ViaTBA] = preprocessor_.run(inputs_[ViaTBA]);
      }
    if (jobs & ViaSBA)
      {
        inputs_.emplace(ViaSBA, spot::degeneralize(input_));
        if (preproc_)
          {
            preprocessor_.set_type(spot::postprocessor::BA);
            inputs_[ViaSBA] = preprocessor_.run(inputs_[ViaSBA]);
          }
      }
  }


  spot::twa_graph_ptr input_;
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

aut_ptr semi_determinize(aut_ptr aut,
                         bool cut_det,
                         jobs_type jobs,
                         const_om_ptr opt)
{
  if (spot::is_deterministic(aut))
    return aut;

  bool preproc = false;
  if (opt)
    preproc = opt->get("preprocess",0);

  if (preproc)
  {
    spot::postprocessor preprocessor;
    preprocessor.set_pref(spot::postprocessor::Deterministic);
    preprocessor.run(aut);
  }

  if (spot::is_semi_deterministic(aut))
  {
    if (!cut_det)
      return aut;

    auto non_det_states = new state_set;
    aut_ptr result;

    if (is_cut_deterministic(aut, non_det_states))
      result = aut;
    else
      result = determinize_first_component(aut, non_det_states);
    delete non_det_states;
    return result;
  }

  // Safety automata can be determinized using powerset construction
  if (aut->acc().is_all())
  {
    auto result = spot::tba_determinize(aut);
    result->set_acceptance(0, spot::acc_cond::acc_code::t());
    return result;
  }

  seminator sem(aut, cut_det, opt);
  return sem.run(jobs);
}



