// Copyright (C) 2017, 2019, Fakulta Informatiky Masarykovy univerzity
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


void print_usage(std::ostream& os) {
  os << "Usage: seminator [OPTION...] [FILENAME]\n";
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

    --ba        SBA output
    --tba       TBA output
    --tgba      TGBA output (default)

    --highlight color states of 1st component by violet, 2nd by green,
                cut-edges by red

    --is-cd     do not run transformation, check whether input is
                cut-deterministic. Outputs 1 if it is, 0 otherwise.
                (Spot's autfilt offers --is-semideterministic check)

Transformation type (T=transition-based, S=state-based):
    --via-tgba  one-step semi-determinization: TGBA -> sDBA
    --via-tba   two-steps: TGBA -> TBA -> sDBA
    --via-sba   two-steps: TGBA -> SBA -> sDBA

  Multiple translation types can be chosen, the one with smallest
  result will be outputted. If none is chosen, all three are run.

Cut-edges construction:
    --cut-always        cut-edges for each edge to an accepting SCC
    --cut-on-SCC-entry  cut-edges also for edges freshly entering an
                        accepting SCC
    --cut-highest-mark  cut-edges on highest marks only (default)

  Cut-edges are edges between the 1st and 2nd component of the result.
  They are based on edges of the input automaton.

Optimizations:
    --bscc-avoid[=0|1]          avoid deterministic bottom part of input in 1st
                                component and jump directly to 2nd component
    --powerset-for-weak[=0|1]   avoid breakpoint construction for
                                inherently weak accepting SCCs and use
                                powerset construction instead
    --powerset-on-cut[=0|1]     if s -a-> p needs a cut, create
                                s -a-> (δ(s),δ_0(s),0) instead of
                                s -a-> ({p},∅,0).
    --jump-to-bottommost[=0|1]     remove useless prefixes of second component
    --reuse-good-SCC[=0|1]      similar as --bscc-avoid, but uses the SCCs
                                unmodified with (potentialy) TGBA acceptance
    --skip-levels[=0|1]         allow multiple breakpoints on 1 edge; a trick
                                well known from degeneralization
    --scc-aware[=0|1]           scc-aware optimizations
    --scc0, --no-scc-aware      same as --scc-aware=0
    --pure                      disable all optimizations except --scc-aware,
                                also disable pre and post processings

  Pass 1 (or nothing) to enable, or 0 to disable.  All optimizations
  are enabled by default, unless --pure is specified, in which case
  only --scc-aware is on.

Pre- and Post-processing:
    --preprocess[=0|1]       simplifications of input automaton
    --postprocess[=0|1]      simplifications of the output automaton (default)
    -s0, --no-reductions     same as --postprocess=0 --preprocess=0

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
    std::string path_to_file;

    spot::option_map om;
    bool cut_det = false;
    jobs_type jobs = 0;

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
        else if (arg == "--cut-on-SCC-entry")
          om.set("cut-on-SCC-entry", true);
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
                 || match_opt(arg, "--reuse-good-SCC")
                 || match_opt(arg, "--skip-levels")
                 || match_opt(arg, "--scc-aware")
                 || match_opt(arg, "--powerset-on-cut")
                 || match_opt(arg, "--preprocess")
                 || match_opt(arg, "--postprocess"))
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
            om.set("reuse-good-SCC", false);
            om.set("jump-to-bottommost", false);
            om.set("bscc-avoid", false);
            om.set("skip-levels", false);
            om.set("powerset-on-cut", false);
            om.set("postprocess", false);
            om.set("preprocess", false);
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

        else if (arg == "--ba")
          om.set("output", BA);

        else if (arg == "--tba")
          om.set("output", TBA);

        else if (arg == "--tgba")
          om.set("output", TGBA);

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
                path_to_file = argv[i+1];
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
        else if (path_to_file.empty())
          path_to_file = argv[i];
        else
          {
            std::cerr << "seminator: Multiple inputs specified.\n";
            return 2;
          }
      }

    if (path_to_file.empty() && isatty(STDIN_FILENO))
    {
      std::cerr << "seminator: No automaton to process? "
        "Run 'seminator --help' for more help.\n";
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

    spot::twa_graph_ptr aut = parsed_aut->aut;
    // Remove dead and unreachable states and prune accepting
    // conditions in non-accepting SCCs.
    aut = spot::scc_filter(aut, true);

    // Check if input is TGBA
    if (!aut->acc().is_generalized_buchi())
    {
      std::cerr << "seminator: The tool requires a TGBA on input.\n";
      return 1;
    }

    if (cd_check)
    {
      std::cout << is_cut_deterministic(aut) << std::endl;
      check_cout();
      return 0;
    }

    auto result = semi_determinize(aut, cut_det, jobs, &om);
    if (result != parsed_aut->aut)
      if (auto old_n =
          parsed_aut->aut->get_named_prop<std::string>("automaton-name"))
        {
          auto name = new std::string((om.get("cut-deterministic", 0) ?
                                       "cDBA for " : "sDBA for ") + *old_n);
          result->set_named_prop("automaton-name", name);
        }

    if (high)
    {
      highlight_components(result);
      spot::print_hoa(std::cout, result, "1.1") << '\n';
    } else
      spot::print_hoa(std::cout, result) << '\n';

    check_cout();
    return 0;
}


