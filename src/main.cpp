// Copyright (C) 2017, 2019, 2020, Fakulta Informatiky Masarykovy univerzity
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

#include "config.h"
#include <unistd.h>
#include "seminator.hpp"
#include "cutdet.hpp"
#include <spot/parseaut/public.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twaalgos/complement.hh>

void print_usage(std::ostream& os) {
  os << "Usage: seminator [OPTION...] [FILENAME...]\n";
}

void print_help() {
  print_usage(std::cout);
  std::cout <<
    R"(The tool transforms TGBA into equivalent semi- or cut-deterministic TBA.

By default, it reads a generalized Büchi automaton (GBA) from standard input
and converts it into semi-deterministic Büchi automaton (sDBA), runs
Spot's simplifications on it and outputs the result in the HOA format.
The main algorithms are based on breakpoint construction. If the automaton
is already of the requested shape, only the simplifications are run.

Input options:
    -f FILENAME reads the input from FILENAME instead of stdin

Output options:
    --cd        cut-deterministic automaton
    --sd        semi-deterministic automaton (default)
    --complement[=best|spot|pldi]
                build a semi-deterministic automaton to complement it using
                the NCSB implementation of Spot, or the PLDI'18 variant
                implemented in Seminator

    --ba        SBA output
    --tba       TBA output
    --tgba      TGBA output (default)

    --highlight color states of 1st component by violet, 2nd by green,
                cut-edges by red

    --is-cd     do not run transformation, check whether input is
                cut-deterministic. Outputs only the cut-deterministic inputs.
                (Spot's autfilt offers --is-semideterministic check)

Transformation type (T=transition-based, S=state-based):
    --via-tgba  one-step semi-determinization: TGBA -> sDBA
    --via-tba   two-steps: TGBA -> TBA -> sDBA
    --via-sba   two-steps: TGBA -> SBA -> sDBA

  Multiple translation types can be chosen, the one with smallest
  result will be outputted. If none is chosen, all three are run.

Cut-edges construction:
    --cut-always        cut-edges for each edge to an accepting SCC
                        (default, unless --pure)
    --cut-on-SCC-entry  cut-edges also for edges freshly entering an
                        accepting SCC
    --cut-highest-mark  cut-edges on highest marks only (default if --pure)

  Cut-edges are edges between the 1st and 2nd component of the result.
  They are based on edges of the input automaton.

Optimizations:
    --bscc-avoid[=0|1]          avoid deterministic bottom part of input in 1st
                                component and jump directly to 2nd component
    --jump-to-bottommost[=0|1]  remove useless trivial SCCs of 2nd component
    --powerset-for-weak[=0|1]   avoid breakpoint construction for
                                inherently weak accepting SCCs and use
                                powerset construction instead
    --powerset-on-cut[=0|1]     if s -a-> p needs a cut, create
                                s -a-> (δ(s),δ_0(s),0) instead of
                                s -a-> ({p},∅,0).
    --reuse-deterministic[=0|1] similar to --bscc-avoid, but uses the SCCs
                                unmodified with (potentialy) TGBA acceptance
    --skip-levels[=0|1]         allow multiple breakpoints on 1 edge; a trick
                                well known from degeneralization
    --scc-aware[=0|1]           scc-aware optimizations
    --scc0, --no-scc-aware      same as --scc-aware=0
    --pure                      disable all optimizations except --scc-aware,
                                also disable pre and post processings and
                                implies --cut-highest-mark by default

  Pass 1 (or nothing) to enable, or 0 to disable.  All optimizations
  are enabled by default, unless --pure is specified, in which case
  only --scc-aware is on.

Pre- and Post-processing:
    --preprocess[=0|1]       simplify the input automaton
    --postprocess[=0|1]      simplify the output of the semi-determinization
                             algorithm (default)
    --postprocess-comp[=0|1] simplify the output of the complementation
                             (default)
    -s0, --no-reductions     same as --postprocess=0 --preprocess=0
                             --postprocess-comp=0

Miscellaneous options:
  -h, --help    print this help
  --version     print program version
)";
}

void check_cout()
{
  std::cout.flush();
  if (!std::cout)
    {
      std::cerr << "seminator: error writing to standard output\n";
      exit(2);
    }
}

int main(int argc, char* argv[])
{
    // Declaration for input options. The rest is in seminator.hpp
    // as they need to be included in other files.
    bool cd_check = false;
    bool high = false;
    std::vector<std::string> path_to_files;

    spot::option_map om;
    bool cut_det = false;
    jobs_type jobs = 0;
    enum complement_t { NoComplement = 0, NCSBBest, NCSBSpot, NCSBPLDI };
    complement_t complement = NoComplement;
    output_type desired_output = TGBA;

    auto match_opt =
      [&](const std::string& arg, const std::string& opt)
      {
        if (arg.compare(0, opt.size(), opt) == 0)
          {
            if (const char* tmp = om.parse_options(arg.c_str() + 2))
              {
                std::cerr << "seminator: failed to process option --"
                          << tmp << '\n';
                exit(2);
              }
            return true;
          }
        return false;
      };

    for (int i = 1; i < argc; i++)
      {
        std::string arg = argv[i];

        // Transformation types
        if (arg == "--via-sba")
          jobs |= ViaSBA;
        else if (arg == "--via-tba")
          jobs |= ViaTBA;
        else if (arg == "--via-tgba")
          jobs |= ViaTGBA;
        //
        else if (arg == "--is-cd")
          cd_check = true;
        // Cut edges
        else if (arg == "--cut-on-SCC-entry") {
          om.set("cut-on-SCC-entry", true);
          om.set("cut-always", false);
        }
        else if (arg == "--cut-always")
          om.set("cut-always", true);
        else if (arg == "--cut-highest-mark")
          {
            om.set("cut-always", false);
            om.set("cut-on-SCC-entry", false);
          }
        // Optimizations
        else if (match_opt(arg, "--powerset-for-weak")
                 || match_opt(arg, "--jump-to-bottommost")
                 || match_opt(arg, "--bscc-avoid")
                 || match_opt(arg, "--reuse-deterministic")
                 || match_opt(arg, "--skip-levels")
                 || match_opt(arg, "--scc-aware")
                 || match_opt(arg, "--powerset-on-cut")
                 || match_opt(arg, "--preprocess")
                 || match_opt(arg, "--postprocess")
                 || match_opt(arg, "--postprocess-comp"))
          {
          }
        else if (arg == "--scc0")
          om.set("scc-aware", false);
        else if (arg == "--no-scc-aware")
          om.set("scc-aware", false);
        else if (arg == "--pure")
          {
            om.set("bscc-avoid", false);
            om.set("powerset-for-weak", false);
            om.set("reuse-deterministic", false);
            om.set("jump-to-bottommost", false);
            om.set("bscc-avoid", false);
            om.set("skip-levels", false);
            om.set("powerset-on-cut", false);
            om.set("postprocess", false);
            om.set("postprocess-comp", false);
            om.set("preprocess", false);
            om.set("cut-always", false);
            om.set("cut-on-SCC-entry", false);
          }
        else if (arg == "-s0" || arg == "--no-reductions")
          {
            om.set("postprocess", false);
            om.set("preprocess", false);
          }
        else if (arg == "--simplify-input")
          om.set("preprocess", true);

        // Prefered output
        else if (arg == "--cd")
          cut_det = true;
        else if (arg == "--sd")
          cut_det = false;
        else if (arg == "--complement" || arg == "--complement=best")
          complement = NCSBBest;
        else if (arg == "--complement=spot")
          complement = NCSBSpot;
        else if (arg == "--complement=pldi")
          complement = NCSBPLDI;

        else if (arg == "--ba")
          desired_output = BA;
        else if (arg == "--tba")
          desired_output = TBA;
        else if (arg == "--tgba")
          desired_output = TGBA;

        else if (arg == "--highlight")
          high = true;

        else if (arg == "-f")
          {
            if (argc < i + 1)
              {
                std::cerr << "seminator: Option -f requires an argument.\n";
                return 1;
              }
            else
              {
                path_to_files.emplace_back(argv[i+1]);
                i++;
              }
          }
        else if ((arg == "--help") || (arg == "-h"))
          {
            print_help();
            check_cout();
            return 0;
          }
        else if (arg == "--version")
          {
            std::cout <<
              ("Seminator (" PACKAGE_VERSION
               ") compiled with Spot " SPOT_PACKAGE_VERSION "\n"
               "License GPLv3+: GNU GPL version 3 or later"
               " <http://gnu.org/licenses/gpl.html>.\n"
               "This is free software: you are free to change "
               "and redistribute it.\n"
               "There is NO WARRANTY, to the extent permitted by law.\n")
                      << std::flush;
            return 0;
          }
        // removed
        else if (arg == "--cy")
          {
            std::cerr << ("seminator: "
                          "Invalid option --cy. Use --via-sba -s0 instead.\n");
            return 2;
          }
        // Detection of unsupported options
        else if (arg[0] == '-')
          {
            std::cerr << "seminator: Unsupported option " << arg << '\n';
            return 2;
          }
        else
          {
            path_to_files.emplace_back(argv[i]);
          }
      }

    if (path_to_files.empty())
    {
      if (isatty(STDIN_FILENO))
        {
          std::cerr << "seminator: No automaton to process? "
            "Run 'seminator --help' for more help.\n";
          print_usage(std::cerr);
          return 1;
        }
      else
        {
          // Process stdin by default.
          path_to_files.emplace_back("-");
        }
    }

    if (high && complement)
      {
        std::cerr
          << "seminator --highlight and --complement are incompatible\n";
        return 1;
      }

    if (jobs == 0)
      jobs = AllJobs;

    om.set("output", complement ? TBA : desired_output);

    auto dict = spot::make_bdd_dict();

    for (std::string& path_to_file: path_to_files)
      {
        spot::automaton_stream_parser parser(path_to_file);

        for (;;)
          {
            spot::parsed_aut_ptr parsed_aut = parser.parse(dict);

            if (parsed_aut->format_errors(std::cerr))
              return 1;

            spot::twa_graph_ptr aut = parsed_aut->aut;

            if (!aut)
              break;

            // Check if input is TGBA
            if (!aut->acc().is_generalized_buchi())
              {
                if (parsed_aut->filename != "-")
                  std::cerr << parsed_aut->filename << ':';
                std::cerr << parsed_aut->loc
                          << ": seminator requires a TGBA on input.\n";
                return 1;
              }

            if (cd_check)
              {
                if (!is_cut_deterministic(aut))
                  continue;
              }
            else
              {
                aut = semi_determinize(aut, cut_det, jobs, &om);
                if (auto old_n = parsed_aut->aut->get_named_prop<std::string>
                    ("automaton-name"))
                  {
                    auto name =
                      new std::string(((aut->num_sets() == 1)
                                       ? "sDBA for " : "sDGBA for ") + *old_n);
                    if (cut_det)
                      (*name)[0] = 'c';
                    aut->set_named_prop("automaton-name", name);
                  }

                if (complement)
                  {
                    spot::twa_graph_ptr comp = nullptr;
                    spot::postprocessor postprocessor;
                    // We don't deal with TBA: (1) complement_semidet() returns a
                    // TBA, and (2) in Spot 2.8 spot::postprocessor only knows
                    // about state-based BA and Transition-based GBA.  So TBA/TGBA
                    // are simply simplified as TGBA.
                    postprocessor.set_type(desired_output == BA
                                           ? spot::postprocessor::BA
                                           : spot::postprocessor::TGBA);
                    if (!om.get("postprocess-comp", 1))
                      {
                        // Disable simplifications except acceptance change.
                        postprocessor.set_level(spot::postprocessor::Low);
                        postprocessor.set_pref(spot::postprocessor::Any);
                      }

                    if (complement == NCSBSpot || complement == NCSBBest)
                      {
                        comp = spot::complement_semidet(aut);
                        comp = postprocessor.run(comp);
                      }
                    if (complement == NCSBPLDI || complement == NCSBBest)
                      {
                        spot::twa_graph_ptr comp2 =
                          from_spot::complement_semidet(aut);
                        comp2 = postprocessor.run(comp2);
                        if (!comp || comp->num_states() > comp2->num_states())
                          comp = comp2;
                      }
                    aut = comp;
                  }
              }
            const char* opts = nullptr;
            if (high)
              {
                highlight_components(aut);
                opts = "1.1";
              }
            spot::print_hoa(std::cout, aut, opts) << '\n';
          }
      }

    check_cout();
    return 0;
}


