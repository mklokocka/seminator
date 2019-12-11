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

/**
* Checks the input automaton if it is of requested type and returns it back.
* If not, checks for easy cases first and only after that runs seminator.
*/
aut_ptr check_and_compute(aut_ptr aut, jobs_type jobs, const_om_ptr opt)
{
  if (spot::is_deterministic(aut))
    return aut;

  bool cut_det = opt->get("cut-deterministic",0);
  bool preproc = opt->get("preprocess",0);

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

  seminator sem(aut, jobs, opt);
  return sem.run();
}

void seminator::prepare_inputs(jobs_type jobs)
{
  if (!jobs)
    jobs = jobs_;

  // TODO check what makes sense
  if (jobs && Onestep)
    inputs_.emplace(Onestep, input_);
  if (jobs && ViaTBA)
  {
    inputs_.emplace(ViaTBA, spot::degeneralize_tba(input_));
    if (preproc_)
      inputs_[ViaTBA] = preprocessor_.run(inputs_[ViaTBA]);
  }
  if (jobs && ViaSBA)
  {
    inputs_.emplace(ViaSBA, spot::degeneralize(input_));
    if (preproc_)
    {
      preprocessor_.set_type(spot::postprocessor::BA);
      inputs_[ViaSBA] = preprocessor_.run(inputs_[ViaSBA]);
    }
  }
}

void seminator::run_jobs(jobs_type jobs)
{
  if (!jobs)
    jobs = jobs_;
  for (auto job : unitjobs)
  {
    if (job && jobs)
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
          results_[job] = determinize_first_component(inputs_[job], &non_det_states);
      } else
      {
        // Run the breakpoint algorithm
        bp_twa resbp(inputs_[job], opt_);
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

jobs_type seminator::best_from(jobs_type jobs)
{
  if (!jobs)
    jobs = jobs_;
  jobs_type res = 0;
  for (auto job : unitjobs)
  {
    if (job && jobs)
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

aut_ptr seminator::get(jobs_type job)
{
  assert (unitjobs.count(job));
  return results_[job];
}

aut_ptr seminator::run(jobs_type jobs)
{
  prepare_inputs(jobs);
  return results_[best_from(jobs)];
}

void seminator::parse_options()
{
  cut_det_  = opt_->get("cut-deterministic", 0);
  preproc_  = opt_->get("preprocess",0);
  postproc_ = opt_->get("postprocess", 1);

  // The deterministic attempt was done in check_and_compute
  if (preproc_)
    preprocessor_.set_pref(spot::postprocessor::Small);

  output_  = static_cast<output_type>(opt_->get("output", TGBA));
}
