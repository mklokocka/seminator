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

#include "config.h"
#include <unistd.h>
#include "seminator.hpp"
#include "cutdet.hpp"
#include <spot/parseaut/public.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/sccfilter.hh>

static const char* VERSION_TAG = PACKAGE_VERSION;


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
  "    --cd       \tcut-deterministic automaton\n"
  "    --sd       \tsemi-deterministic automaton (default)\n\n"
  "    --ba       \tSBA output\n"
  "    --tba      \tTBA output\n"
  "    --tgba     \tTGBA output (default)\n\n"

  "    --highlight\tcolor states of 1st component by violet, 2nd by green,\n"
  "               \tcut-edges by red\n\n"

  "    --is-cd    \tdo not run transformation, check whether input is \n"
  "               \tcut-deterministic. Outputs 1 if it is, 0 otherwise.\n"
  "               \t(Spot's autfilt offers --is-semideterministic check)\n\n";

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
  "    --bscc-avoid           \tavoid deterministic bottom part of input in 1st\n"
  "                           \tcomponent and jump directly to 2nd component\n"
  "    --powerset-for-weak    \tavoid breakpoint construction for\n"
  "                           \tinherently weak accepting SCCs and use\n"
  "                           \tpowerset construction instead\n"
  "    --remove-prefixes      \tremove useless prefixes of second component\n"
  "    --reuse-good-SCC       \tsimilar as --bscc-avoid, but uses the SCCs\n"
  "                           \tunmodified with (potentialy) TGBA acceptance\n"
  "    --skip-levels          \tallow multiple breakpoints on 1 edge; a trick\n"
  "                           \twell known from degeneralization\n"
  "    --scc-aware            \tenable scc-aware optimization (default)\n"
  "    --scc0, --no-scc-aware \tdisable scc-aware optimization\n\n";

  std::cout << "Pre- and Post-processing:\n"
  "    -s0,    --no-reductions\tdisable Spot automata post-reductions\n"
  "    --simplify-input       \tenable simplification of input automaton\n\n";

  std::cout << "Miscellaneous options: \n"
  "  -h, --help \tprint this help\n"
  "  --version  \tprint program version" << std::endl;
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

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        // Transformation types
        if (arg.compare("--via-sba") == 0)
            jobs |= ViaSBA;

        else if (arg.compare("--via-tba") == 0)
            jobs |= ViaTBA;

        else if (arg.compare("--via-tgba") == 0)
            jobs |= ViaTGBA;

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
        else if (arg.compare("--remove-prefixes") == 0)
            om.set("remove-prefixes", true);
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
            cut_det = true;

        else if (arg.compare("--sd") == 0)
            cut_det = false;

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
        else if ((arg.compare("--help") == 0) || (arg.compare("-h") == 0))
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

    spot::twa_graph_ptr aut = parsed_aut->aut;
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
      highlight(result);
      spot::print_hoa(std::cout, result, "1.1") << '\n';
    } else
      spot::print_hoa(std::cout, result) << '\n';
    return 0;
}


