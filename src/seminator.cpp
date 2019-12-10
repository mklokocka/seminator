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

#include <spot/parseaut/public.hh>
#include <spot/twaalgos/dot.hh>
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

int main(int argc, char* argv[])
{
    // Declaration for input options. The rest is in seminator.hpp
    // as they need to be included in other files.
    bool cd_check = false;
    bool high = false;
    std::string path_to_file;

    spot::option_map om;
    jobs_type jobs = 0;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        // Transformation types
        if (arg.compare("--via-sba") == 0)
            jobs |= ViaSBA;

        else if (arg.compare("--via-tba") == 0)
            jobs |= ViaTBA;

        else if (arg.compare("--via-tgba") == 0)
            jobs |= Onestep;

        else if (arg.compare("--is-cd") == 0)
            cd_check = true;

        // Cut edges
        else if (arg.compare("--cut-on-SCC-entry") == 0) {
            om.set("cut-on-SCC-entry", true);

        } else if (arg.compare("--cut-always") == 0)
            om.set("cut-always", true);

        else if (arg.compare("--powerset-on-cut") == 0)
            om.set("powerset-on-cut", true);

        // Optimizations
        else if (arg.compare("--powerset-for-weak") == 0)
            om.set("powerset-for-weak", true);
        else if (arg.compare("--bscc-avoid") == 0)
            om.set("bscc-avoid", true);
        else if (arg.compare("--reuse-good-SCC") == 0)
            om.set("reuse-SCC", true);
        else if (arg.compare("--skip-levels") == 0)
            om.set("skip-levels", true);

        else if (arg.compare("--scc0") == 0)
            om.set("scc-aware", false);
        else if (arg.compare("--no-scc-aware") == 0)
            om.set("scc-aware", false);
        else if (arg.compare("--scc-aware") == 0)
            om.set("scc-aware", true);

        else if (arg.compare("-s0") == 0)
            om.set("postprocess", false);

        else if (arg.compare("--no-reductions") == 0)
            om.set("postprocess", false);

        else if (arg.compare("--simplify-input") == 0)
            om.set("preprocess", true);

        // Prefered output
        else if (arg.compare("--cd") == 0)
            om.set("cut-deterministic", true);

        else if (arg.compare("--sd") == 0)
            om.set("cut-deterministic", false);

        else if (arg.compare("--ba") == 0)
            om.set("output", BA);

        else if (arg.compare("--tba") == 0)
            om.set("output", TBA);

        else if (arg.compare("--tgba") == 0)
            om.set("output", TGBA);

        else if (arg.compare("--highlight") == 0)
            high = true;

        else if (arg.compare("-f") == 0)
        {
            if (argc < i + 1)
            {
                std::cerr << "Seminator: Option requires an argument -- 'f'" << std::endl;
                return 1;
            }
            else
            {
                path_to_file = argv[i+1];
                i++;
            }
        }
        else if ((arg.compare("--help") == 0) | (arg.compare("-h") == 0))
        {
            print_help();
            return 1;
        }
        else if (arg.compare("--version") == 0)
        {
            std::cout << "Seminator (" << VERSION_TAG <<
            ") compiled with Spot " << SPOT_PACKAGE_VERSION <<  std::endl;

            std::cout << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>." << std::endl;
            std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
            std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

            std::cout << std::endl;

            return 0;
        }
        // removed
        else if (arg.compare("--cy") == 0)
        {
            std::cerr << "Invalid option --cy. Use --via-sba -s0 instead.\n";
            return 2;
        }
        // Detection of unsupported options
        else if (arg.compare(0, 1, "-") == 0)
        {
          std::cout << "Unsupported option " << arg << std::endl;
          return 2;
        }
        else if (path_to_file.empty())
            path_to_file = argv[i];
    }

    if (path_to_file.empty() && isatty(STDIN_FILENO))
    {
      std::cerr << "Seminator: No automaton to process? Run\n"
            "'seminator --help' for more help" << std::endl;
      print_usage(std::cerr);
      return 1;

    }

    if (jobs == 0)
      jobs = AllJobs;

    auto dict = spot::make_bdd_dict();

    spot::parsed_aut_ptr parsed_aut;

    if (path_to_file.empty())
        path_to_file = "-";
    parsed_aut = parse_aut(path_to_file, dict);

    if (parsed_aut->format_errors(std::cerr))
      return 1;

    aut_ptr aut = parsed_aut->aut;
    // Remove dead and unreachable states and prune accepting conditions in non-accepting SCCs.
    aut = spot::scc_filter(aut, true);

    // Check if input is TGBA
    if (!aut->acc().is_generalized_buchi())
    {
      std::cerr << "Seminator: The tool requires a TGBA on input" << std::endl;
      return 1;
    }

    if (cd_check)
    {
      std::cout << is_cut_deterministic(aut) << std::endl;
      return 0;
    }

    auto result = check_and_compute(aut, jobs, &om);
    if (result != aut)
    {
      auto old_n = aut->get_named_prop<std::string>("automaton-name");
      if (old_n)
      {
        std::stringstream ss;
        ss << (om.get("cut-deterministic",0) ? "cDBA for " : "sDBA for ") << *old_n;
        std::string * name = new std::string(ss.str());
        result->set_named_prop("automaton-name", name);
      }
    }
    if (high)
    {
      highlight(result);
      spot::print_hoa(std::cout, result, "1.1") << '\n';
    } else
      spot::print_hoa(std::cout, result) << '\n';
    return 0;
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

void seminator::prepare_inputs(jobs_type jobs)
{
  if (!jobs)
    jobs = jobs_;

  // TODO check what makes sense
  if (jobs & Onestep)
    inputs_.emplace(Onestep, input_);
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

void seminator::run_jobs(jobs_type jobs)
{
  if (!jobs)
    jobs = jobs_;
  for (auto job : unitjobs)
  {
    if (job & jobs)
    {
      // Prepare the desired input
      if (!inputs_[job])
        prepare_inputs(job);
      assert(inputs_[job]);

      state_set non_det_states;
      if (spot::is_deterministic(inputs_[job]) |
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
