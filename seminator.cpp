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

#include <string>
#include <iostream>
#include <utility>
#include <tuple>
#include <vector>
#include <map>
#include <stack>
#include <set>
#include <stdexcept>
#include <unistd.h>
#include <bddx.h>
#include <spot/twa/twa.hh>
#include <spot/parseaut/public.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/bddprint.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/stripacc.hh>
#include <spot/misc/bddlt.hh>
#include <spot/twaalgos/powerset.hh>
#include <spot/twaalgos/degen.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/sccinfo.hh>
#include <spot/twaalgos/postproc.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/misc/optionmap.hh>

typedef std::set<unsigned> state_set;
typedef std::map<std::tuple<int, state_set, state_set>, unsigned> state_dictionary;
typedef std::tuple<int, state_set, state_set, unsigned> todo_state;
typedef std::vector<todo_state> todo_list;
typedef std::map<state_set, unsigned> powerset_state_dictionary;
typedef std::tuple<state_set, unsigned> powerset_todo_state;
typedef std::vector<powerset_todo_state> powerset_todo_list;
typedef std::map<unsigned,unsigned> state_map;

spot::twa_graph_ptr buchi_to_semi_deterministic_buchi(spot::twa_graph_ptr& aut, bool deterministic_first_component, bool optimization, unsigned output);
void copy_buchi(spot::twa_graph_ptr aut, spot::const_twa_graph_ptr to_copy, state_dictionary* sdict, todo_list* todo, std::vector<std::string>* names);
std::vector<bdd> edge_condition_to_minterms(bdd allap, bdd cond, std::map<bdd, std::vector<bdd>, spot::bdd_less_than>* minterms);
unsigned sets_to_state(spot::twa_graph_ptr aut, state_dictionary* sdict, todo_list* todo, std::vector<std::string>* names, int k, state_set left, state_set right);
unsigned powerset_set_to_state(spot::twa_graph_ptr aut, powerset_state_dictionary* sdict, powerset_todo_list* todo, std::vector<std::string>* names, state_set states);
bool is_cut_deterministic(const spot::twa_graph_ptr& aut, std::set<unsigned>* non_det_states = nullptr);
void determinize_first_component(spot::twa_graph_ptr result, spot::twa_graph_ptr aut, std::set<unsigned> to_determinize);

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


static const unsigned TGBA = 0;
static const unsigned TBA = 1;
static const unsigned BA = 2;

static const std::string VERSION_TAG = "v1.2.0";

int main(int argc, char* argv[])
{
    // Setting up the configurations flags for the user.

    std::string automata_from_cin;
    if (!isatty(fileno(stdin)))
    {
        // don't skip the whitespace while reading
        std::cin >> std::noskipws;

        // use stream iterators to copy the stream to a string
        std::istream_iterator<char> it(std::cin);
        std::istream_iterator<char> end;
        std::string result(it, end);
        automata_from_cin.append(result);
    }

    bool deterministic_first_component = false;
    bool optimize = true;
    bool cy_alg = false;
    bool cd_check = false;
    bool via_tgba = false;
    bool via_tba = false;
    bool via_sba = false;
    unsigned preferred_output = TGBA;
    std::string path_to_file;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.compare("--cd") == 0)
        {
            deterministic_first_component = true;
        }
        else if (arg.compare("--via-sba") == 0)
        {
            via_sba = true;
        }
        else if (arg.compare("--via-tba") == 0)
        {
            via_tba = true;
        }
        else if (arg.compare("--via-tgba") == 0)
        {
            via_tgba = true;
        }
        else if (arg.compare("--is-cd") == 0)
        {
            cd_check = true;
        }
        else if (arg.compare("-s0") == 0)
        {
            optimize = false;
        }
        else if (arg.compare("--cy") == 0)
        {
            cy_alg = true;
        }
        else if (arg.compare("--ba") == 0)
        {
            preferred_output = BA;
        }
        else if (arg.compare("--tba") == 0)
        {
            preferred_output = TBA;
        }
        else if (arg.compare("--tgba") == 0)
        {
            preferred_output = TGBA;
        }
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
        else if (arg.compare("--help") == 0)
        {
            std::cout << "Usage: seminator [OPTION...] [FILENAME]" << std::endl;
            std::cout << "The tool is used to transform a TGBA into an equivalent semi-(or cut-)deterministic TBA." << std::endl;

            std::cout << std::endl;

            std::cout << " Input:" << std::endl;
            std::cout << "   -f FILENAME\ttransform the automaton in FILENAME" << std::endl;

            std::cout << " Transformation options: " << std::endl;
            std::cout << "  --cd\t\tmake sure the first component of the result is deterministic" << std::endl;
            std::cout << "  --cy\t\tsimulate the CY[88] algorithm (preceeded by degeneralization to NBA)" << std::endl;
            std::cout << "  --via-tgba\tthe standard algorithm which proceeds by transforming" <<
                std::endl << "\t\tthe input automaton as is" << std::endl;
            std::cout << "  --via-tba\tthe input automaton is first degeneralized into a TBA" <<
                std::endl << "\t\tbefore being transformed" << std::endl;
            std::cout << "  --via-sba\tthe input automaton is first degeneralized into a BA" <<
                std::endl << "\t\tbefore being transformed" << std::endl;
            std::cout << "   -s0\t\tdisables spot automata reductions algorithms" << std::endl;

            std::cout << " Output options: " << std::endl;
            std::cout << "  --tgba\tprefer TGBA output" << std::endl;
            std::cout << "  --tba\t\tprefer TBA output" << std::endl;
            std::cout << "  --ba\t\tprefer BA output" << std::endl;
            std::cout << "  --is-cd\toutputs 1 if the input automaton is cut-deterministic," <<
                std::endl << "\t\t0 otherwise; does not transform the input automaton" << std::endl;

            std::cout << " Miscellaneous options: " << std::endl;
            std::cout << "  --help\tprint this help" << std::endl;
            std::cout << "  --version\tprint program version" << std::endl;

            std::cout << std::endl;

            return 1;
        }
        else if (arg.compare("--version") == 0)
        {
            std::cout << "Seminator (" << VERSION_TAG << ")" << std::endl;

            std::cout << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>." << std::endl;
            std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
            std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

            std::cout << std::endl;

            return 1;
        }
        else
        {
            if (path_to_file.empty())
            {
                path_to_file = argv[i];
            }
        }
    }

    if (automata_from_cin.empty() && path_to_file.empty())
    {
        std::cerr << "Seminator: No automaton to process?  Run 'seminator --help' for help." << std::endl;
        return 1;
    }

    spot::bdd_dict_ptr dict = spot::make_bdd_dict();

    spot::parsed_aut_ptr parsed_aut;

    if (!path_to_file.empty())
    {
        parsed_aut = parse_aut(path_to_file, dict);
    }
    else
    {
        const std::string filename = "Reading from std::cin pipe";
        spot::automaton_stream_parser parser(automata_from_cin.c_str(), filename);
        parsed_aut = parser.parse(dict);
    }

    if (!parsed_aut->errors.empty() || parsed_aut->aborted)
    {
        std::cerr << "Seminator: Failed to read automaton from " << path_to_file << std::endl;
        return 1;
    }


    spot::twa_graph_ptr aut = parsed_aut->aut;
    spot::twa_graph_ptr result;
    try
    {
        if (cd_check) {
            std::cout << is_cut_deterministic(aut) << std::endl;
            return 0;
        }
        if (cy_alg)
        {
            aut = spot::degeneralize(aut);
            result = buchi_to_semi_deterministic_buchi(aut, deterministic_first_component, optimize, preferred_output);
        }
        else if (via_sba) {
            aut = spot::degeneralize(aut);
            result = buchi_to_semi_deterministic_buchi(aut, deterministic_first_component, optimize, preferred_output);
        }
        else if (via_tba) {
            aut = spot::degeneralize_tba(aut);
            result = buchi_to_semi_deterministic_buchi(aut, deterministic_first_component, optimize, preferred_output);
        }
        else if (via_tgba) {
            result = buchi_to_semi_deterministic_buchi(aut, deterministic_first_component, optimize, preferred_output);
        }
        else
        {
            auto res1 = buchi_to_semi_deterministic_buchi(aut, deterministic_first_component, optimize, preferred_output);
            auto tba_aut = spot::degeneralize_tba(aut);
            auto res2 = buchi_to_semi_deterministic_buchi(tba_aut, deterministic_first_component, optimize, preferred_output);
            result = res1;
            if (res2->num_states() < res1->num_states())
            {
                result = res2;
            }
            auto nba_aut = spot::degeneralize(aut);
            auto res3 = buchi_to_semi_deterministic_buchi(nba_aut, deterministic_first_component, optimize, preferred_output);
            if (res3->num_states() < result->num_states())
            {
                result = res3;
            }

        }
        spot::print_hoa(std::cout, result) << '\n';
        return 0;
    }
    catch (const not_tgba_exception& e)
    {
        std::cerr << "Seminator: The tool requires a TGBA on input" << std::endl;
        return 1;
    }
    catch (const mismatched_bdd_dict_exception& e)
    {
        std::cerr << "Seminator: Mismatched BDD dict" << std::endl;
        return 1;
    }
    catch (const not_semi_deterministic_exception& e)
    {
        std::cerr << "Seminator: The tool could not properly semi-determinize the automata" << std::endl;
    }
    catch (const not_cut_deterministic_exception& e)
    {
        std::cerr << "Seminator: The tool could not properly cut-determinize the automata" << std::endl;
    }
}

/**
 * The semi-determinization algorithm as thought of by F. Blahoudek, J. Strejcek and M. Kretinsky.
 *
 * @param[in] aut TGBA to transform to a semi-deterministic TBA.
 */
spot::twa_graph_ptr buchi_to_semi_deterministic_buchi(spot::twa_graph_ptr& aut, bool deterministic_first_component, bool optimization, unsigned output)
{
    // Remove dead and unreachable states and prune accepting conditions in non-accepting SCCs.
    aut = spot::scc_filter_states(aut, true);

    // Get the BDD dictionary with atomic propositions used in our automata.
    const spot::bdd_dict_ptr& dict = aut->get_dict();

    // We make a copy of the original automata, keeping the acceptance condition the same, for now.
    spot::twa_graph_ptr result = spot::make_twa_graph(dict);

    // Names of the new states for printing purposes.
    std::vector<std::string>* names = new std::vector<std::string>();

    // Make sure we copy all the atomic propositions, so that when aut seize to exists, the resulting
    // automata will still be okay.
    result->copy_ap_of(aut);

    // Check if the given automata is state-based Buchi automata.
    if (!aut->acc().is_generalized_buchi())
    {
        throw not_tgba_exception();
    }
    // Check if automaton is deterministic already.
    else if (spot::is_deterministic(aut))
    {
        result = aut;
    }
    // Check if semi-deterministic already.
    else if (spot::is_semi_deterministic(aut))
    {
        if (deterministic_first_component)
        {
            std::set<unsigned> non_det_states;
            if (is_cut_deterministic(aut, &non_det_states))
            {
                result = aut;
            }
            else
            {
                // Determinize first component.
                determinize_first_component(result, aut, non_det_states);
            }
        }
        else
        {
            result = aut;
        }
    }
    else
    {
        // Keeps a dictionary giving us a relationship between pairs of sets of states and states in the result automata.
        state_dictionary sdict;
        // List of states we still need to go through.
        todo_list todo;

        int num_of_acc_sets = aut->acc().num_sets();
        bool all = aut->acc().is_all();

        // We will also need to get all the atomic propositions of the automata.
        bdd allap = bddtrue;
        {
            typedef std::set<bdd, spot::bdd_less_than> sup_map;
            sup_map sup;
            // Record occurrences of all guards
            for (auto& t: aut->edges())
            sup.emplace(t.cond);
            for (auto& i: sup)
            allap &= bdd_support(i);
        }

        // Creating a dictionary of minterms
        std::map<bdd, std::vector<bdd>, spot::bdd_less_than> minterms;

        // Now, we create the first component of the new automata.
        if (!deterministic_first_component)
        {
            // Make a copy of the given automata to the result.
            copy_buchi(result, aut, &sdict, &todo, names);
        }
        else
        {
            // We need to create the powerset construction for the first part of the automaton.

            // Keeps a dictionary giving us a relationship between pairs of sets of states and states in the result automata.
            powerset_state_dictionary powerset_sdict;
            // List of states we still need to go through.
            powerset_todo_list powerset_todo;

            state_set initial_state_set{aut->get_init_state_number()};

            int initial_state = powerset_set_to_state(result, &powerset_sdict, &powerset_todo, names, initial_state_set);

            result->set_init_state(initial_state);

            while (!powerset_todo.empty())
            {
                powerset_todo_state todo_now = powerset_todo.back();
                powerset_todo.pop_back();
                state_set states = std::get<0>(todo_now);
                int state_now = std::get<1>(todo_now);

                // First, we will use the state set to generate a map from formulae to state_sets to create new states.
                std::map<bdd, state_set, spot::bdd_less_than> states_cond_map;
                for (auto& state : states)
                {
                    for (auto& edge : aut->out(state))
                    {
                        std::vector<bdd> minterm_conds = edge_condition_to_minterms(allap, edge.cond, &minterms);

                        for(auto& minterm_cond : minterm_conds)
                        {
                            states_cond_map[minterm_cond].insert(edge.dst);
                        }

                        // If e is an accepting transition from set 0, we create an edge to the second component.
                        // In the case the automata accepts everything, we move from every state.
                        if (edge.acc.has(0) || all)
                        {
                            state_set new_set{edge.dst};
                            state_set empty_set;
                            int new_state = sets_to_state(result, &sdict, &todo, names, 0, empty_set, new_set);
                            result->new_edge(state_now, new_state, edge.cond);
                        }
                    }
                }

                for (auto& cond : states_cond_map)
                {
                    state_set new_set = states_cond_map[cond.first];
                    int new_state = powerset_set_to_state(result, &powerset_sdict, &powerset_todo, names, new_set);
                    result->new_edge(state_now, new_state, cond.first);
                }
            }
        }

        spot::scc_info si(aut);
        spot::acc_cond::mark_t accepting_set = result->set_buchi();
        result->prop_state_acc(false);

        // Now we have to go through the newly created states in todo to finish generating the deterministic part of the new automata.
        while (!todo.empty())
        {
            todo_state todo_now = todo.back();
            todo.pop_back();
            int k = std::get<0>(todo_now);
            state_set p = std::get<1>(todo_now);
            state_set q = std::get<2>(todo_now);
            int state_now = std::get<3>(todo_now);

            // First, we will use the q set to generate a map from formulae to state_sets to create new q's.
            std::map<bdd, state_set, spot::bdd_less_than> q_map;
            // Now, we do a similar thing for p, but we have to take care to differentiate an accepting state (p, p) and non-accepting states.
            std::map<bdd, state_set, spot::bdd_less_than> p_map;

            for (auto& state : q)
            {
                for (auto& edge : aut->out(state))
                {
                    if (si.scc_of(edge.dst) == si.scc_of(state))
                    {
                    std::vector<bdd> minterm_conds = edge_condition_to_minterms(allap, edge.cond, &minterms);

                    for(auto& minterm_cond : minterm_conds)
                    {
                        q_map[minterm_cond].insert(edge.dst);
                        // Check if q is in the intersection of Q and F
                        if (all || edge.acc.has(k))
                        {
                            p_map[minterm_cond].insert(edge.dst);
                        }
                    }
                }
            }
            }
            if (p != q)
            {
                for (auto& state : p)
                {
                    for (auto& edge : aut->out(state))
                    {
                        if (si.scc_of(edge.dst) == si.scc_of(state))
                        {
                        std::vector<bdd> minterm_conds = edge_condition_to_minterms(allap, edge.cond, &minterms);

                        for(auto& minterm_cond : minterm_conds)
                        {
                            p_map[minterm_cond].insert(edge.dst);
                            }
                        }
                    }
                }
            }

            std::set<bdd, spot::bdd_less_than> conditions;

            for (auto& cond : p_map)
            {
                conditions.insert(cond.first);
            }
            for (auto& cond : q_map)
            {
                conditions.insert(cond.first);
            }

            // Now we will iterate over all conds relevant to our state
            for (auto& cond : conditions)
            {
                state_set p_set = p_map[cond];
                state_set q_set = q_map[cond];

                // Now we have to generate the successor state. Here we have to be careful, if
                // p_set != q_set, we can generate as normal, if p_set == q_set, and the result
                // should be a transition based automata, we need to generate an accepting edge
                int new_state;
                if (p_set != q_set)
                {
                    new_state = sets_to_state(result, &sdict, &todo, names, k, p_set, q_set);
                    result->new_edge(state_now, new_state, cond);
                }
                else
                {
                    state_set empty_set;
                    new_state = sets_to_state(result, &sdict, &todo, names, !all ? (k + 1) % num_of_acc_sets : 0, empty_set, q_set);
                    result->new_edge(state_now, new_state, cond, accepting_set);
                }
            }
        }

        result->set_named_prop("state-names", names);

        result->merge_edges();
    }

    // Optimization
    // Purge dead and unreachable states.
    result->purge_dead_states();

    if (optimization)
    {

        spot::postprocessor postprocessor;
        if (deterministic_first_component) {
          static spot::option_map extra_options;
          extra_options.set("ba_simul",1);
          extra_options.set("simul",1);
          postprocessor = spot::postprocessor(&extra_options);
        }
        result = postprocessor.run(result);
    }

    if (output == TBA && result->acc().is_generalized_buchi())
    {
        result = spot::degeneralize_tba(result);
    }
    else if (output == BA)
    {
        result = spot::degeneralize(result);
    }

    if (!spot::is_semi_deterministic(result))
    {
        throw not_semi_deterministic_exception();
    }
    else if (deterministic_first_component && !is_cut_deterministic(result))
    {
        throw not_cut_deterministic_exception();
    }

    return result;
}

/**
 * Helper function to copy all the states and transitions of an automaton to another one.
 * Doesn't keep any flags besides state Buchi acceptance. Both automata need to have the same dictionary.
 *
 * @param[out] aut          Automata to copy to.
 * @param[in]  to_copy      Automata to copy from.
 * @param[in, out] sdict    Dictionary of states in second component to their number.
 * @param[in, out] todo     Todo for generating the second component.
 * @param[out] names        Names of the states for printing purposes.
 */
void copy_buchi(spot::twa_graph_ptr aut, spot::const_twa_graph_ptr to_copy, state_dictionary* sdict, todo_list* todo, std::vector<std::string>* names)
{
    if (aut->get_dict() != to_copy->get_dict())
    {
        throw mismatched_bdd_dict_exception();
    }

    bool all = to_copy->acc().is_all();

    aut->new_states(to_copy->num_states());

    // We remember the old names.
    for (unsigned i = 0; i < aut->num_states(); i++)
    {
        names->push_back(std::to_string(i));
    }

    std::vector<bool> seen(to_copy->num_states());
    std::stack<unsigned> copy_todo;
    auto push_state = [&](unsigned state)
    {
        copy_todo.push(state);
        seen[state] = true;
    };
    push_state(to_copy->get_init_state_number());
    while (!copy_todo.empty())
    {
        unsigned s = copy_todo.top();
        copy_todo.pop();
        for (auto& e: to_copy->out(s))
        {
            aut->new_edge(e.src, e.dst, e.cond);
            if (!seen[e.dst])
                push_state(e.dst);

            // If e is an accepting transition from set 0, we create an edge to the second component.
            // In the case the automata accepts everything, we move from every state.
            if (e.acc.has(to_copy->acc().num_sets()-1) || all)
            {
                state_set new_set{e.dst};
                state_set empty_set;
                int new_state = sets_to_state(aut, sdict, todo, names, 0, empty_set, new_set);
                aut->new_edge(e.src, new_state, e.cond);
            }
        }
    }

    aut->set_init_state(to_copy->get_init_state_number());
}

/**
 * Helper function to convert an edge condition to a vector of all the different minterms.
 *
 * @param[in] allap         All atomic propositions used through out the automata.
 * @param[in] cond          Edge condition.
 * @param[in, out] minterms A map of edge conditions to minterms that we have already found.
 * @return Vector of minterms.
 */
std::vector<bdd> edge_condition_to_minterms(bdd allap, bdd cond, std::map<bdd, std::vector<bdd>, spot::bdd_less_than>* minterms)
{
    if (!(*minterms)[cond].empty())
    {
        return (*minterms)[cond];
    }
    else
    {
        bdd all = cond;
        while (all != bddfalse)
        {
            bdd one = bdd_satoneset(all, allap, bddfalse);
            all -= one;
            (*minterms)[cond].emplace_back(one);
        }
        return (*minterms)[cond];
    }
}

/**
 * Helper function that pairs two sets of states an already existing (or new, in the case it does not exist) state in the new automata.
 *
 * @param[in,out] aut The new automata.
 * @param[in,out] sdict Dictionary keeping track of the relationship between pair of sets of states and new states in the new automata.
 * @param[out]    todo Vector of states we have to go through next.
 * @param[out]    names Vector of names that we keep to name the states of the new automata.
 * @param[in]     left Left set of states.
 * @param[in]     right Right set of states.
 * @return Returns a number of the state that either already existed or was created a new representing the pair of sets of states from the old automata.
 */
unsigned sets_to_state(spot::twa_graph_ptr aut, state_dictionary* sdict, todo_list* todo, std::vector<std::string>* names, int k, state_set left, state_set right)
{
    std::tuple<int, state_set, state_set> set_pair (k, left, right);

    try {
        return sdict->at(set_pair);
    } catch (std::out_of_range exc) {
        unsigned new_state_number = aut->new_state();
        (*sdict)[set_pair] = new_state_number;
        todo_state new_todo_state (k, left, right, new_state_number);
        todo->push_back(new_todo_state);

        // Create the name
        std::string left_part;
        for (const unsigned state : left)
        {
            left_part.append(std::to_string(state));
        }

        std::string right_part;
        for (const unsigned state : right)
        {
            right_part.append(std::to_string(state));
        }

        names->push_back("(" + std::to_string(k) + ", {" + left_part + "}, {" + right_part + "})");
        return new_state_number;
    }
}

/**
 * Helper function that pairs a set of states an already existing (or new, in the case it does not exist) state in the powerset construction of an automata.
 *
 * @param[in,out] aut The powerset automata.
 * @param[in,out] sdict Dictionary keeping track of the relationship between pair of sets of states and new states in the new automata.
 * @param[out]    todo Vector of states we have to go through next.
 * @param[out]    names Vector of names that we keep to name the states of the new automata.
 * @param[in]     states Left set of states.
 * @return Returns a number of the state that either already existed or was created a new representing the pair of sets of states from the old automata.
 */
unsigned powerset_set_to_state(spot::twa_graph_ptr aut, powerset_state_dictionary* sdict, powerset_todo_list* todo, std::vector<std::string>* names, state_set states)
{
    try {
        return sdict->at(states);
    } catch (std::out_of_range exc) {
        unsigned new_state_number = aut->new_state();
        (*sdict)[states] = new_state_number;
        powerset_todo_state new_todo_state (states, new_state_number);
        todo->push_back(new_todo_state);

        // Create the name
        std::string name;
        for (const unsigned state : states)
        {
            name.append(std::to_string(state));
        }

        names->push_back("{" + name + "}");
        return new_state_number;
    }
}

/**
 * A function that checks whether a given automaton is cut-deterministic.
 *
 * @param[in] aut               The automata to check for cut-determinism.
 * @param[out] non_det_states   Vector of the states that block cut-determinism.
 * @return Whether the automaton is cut-deterministic or not.
 */
bool is_cut_deterministic(const spot::twa_graph_ptr& aut, std::set<unsigned>* non_det_states)
{
    unsigned UNKNOWN = 0;
    unsigned IN_CUT = 1;
    unsigned NOT_IN_CUT = 2;

    // Take the basic semi-determinism check as implemented in SPOT.
    spot::scc_info si(aut);
    si.determine_unknown_acceptance();
    unsigned nscc = si.scc_count();
    assert(nscc);
    std::vector<bool> reachable_from_acc(nscc);

    std::vector<unsigned> cut(si.scc_count());

    bool cut_det = true;

    do // iterator of SCCs in reverse topological order
    {
        --nscc;
        if (si.is_accepting_scc(nscc) || reachable_from_acc[nscc])
        {
            cut[nscc] = IN_CUT;

            for (unsigned succ: si.succ(nscc))
                reachable_from_acc[succ] = true;

            for (unsigned src: si.states_of(nscc))
            {
                bdd available = bddtrue;
                for (auto& t: aut->out(src))
                {
                    if (!bdd_implies(t.cond, available))
                    {
                        // Not even semi-deterministic.
                        cut_det = false;
                    }
                    else
                    {
                        available -= t.cond;
                    }
                }
            }
        }
    }
    while (nscc);

    for (unsigned i = 0; i < si.scc_count(); i++)
    {
        if (!si.is_accepting_scc(i) && !reachable_from_acc[i])
        {
            for (unsigned succ: si.succ(i))
            {
                if (cut[succ] == NOT_IN_CUT)
                {
                    cut[i] = NOT_IN_CUT;
                }
            }

            std::set<unsigned> edge_states;

            // Check determinism of this component.
            for (unsigned src: si.states_of(i))
            {
                bdd available = bddtrue;
                for (auto& t: aut->out(src))
                {
                    // We check whether the state is at the edge of the SCC.
                    if (si.scc_of(t.dst) != i)
                    {
                        edge_states.insert(src);
                    }
                    else if (!bdd_implies(t.cond, available))
                    {
                        // Component is not deterministic, thus the automaton is not cut deterministic.
                        cut_det = false;
                    }
                    else
                    {
                        available -= t.cond;
                    }
                }
            }

            if (cut[i] == UNKNOWN)
            {
                bool is_in_cut = true;
                // The inner part of the component is deterministic, now we are interested in the outgoing edges.
                for (unsigned edge_state : edge_states)
                {
                    bdd available = bddtrue;
                    for (auto& t: aut->out(edge_state))
                    {
                        if (!bdd_implies(t.cond, available))
                        {
                            is_in_cut = false;
                        }
                        else
                        {
                            available -= t.cond;
                        }
                    }
                }

                cut[i] = is_in_cut ? IN_CUT : NOT_IN_CUT;
            }
            else if (cut_det)
            {
                // We know that the component cannot be in the cut, so we check if the transitions outside the cut
                // are deterministic.
                for (unsigned edge_state : edge_states)
                {
                    bdd available = bddtrue;
                    for (auto& t: aut->out(edge_state))
                    {
                        if (cut[si.scc_of(t.dst)] != IN_CUT)
                        {
                            if (!bdd_implies(t.cond, available))
                            {
                                cut_det = false;
                            }
                            else
                            {
                                available -= t.cond;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            cut[i] = IN_CUT;
        }
    }

    if (non_det_states != nullptr)
    {
        for (unsigned scc = 0; scc < cut.size(); scc++)
        {
            if (cut[scc] != IN_CUT)
            {
                for (unsigned state: si.states_of(scc))
                {
                    non_det_states->insert(state);
                }
            }
        }
    }

    return cut_det;
}

void determinize_first_component(spot::twa_graph_ptr result, spot::twa_graph_ptr aut, std::set<unsigned> to_determinize)
{
    // We will also need to get all the atomic propositions of the automata.
    bdd allap = bddtrue;
    {
        typedef std::set<bdd, spot::bdd_less_than> sup_map;
        sup_map sup;
        // Record occurrences of all guards
        for (auto& t: aut->edges())
        sup.emplace(t.cond);
        for (auto& i: sup)
        allap &= bdd_support(i);
    }

    // Creating a dictionary of minterms
    std::map<bdd, std::vector<bdd>, spot::bdd_less_than> minterms;

    // Names of the new states for printing purposes.
    std::vector<std::string>* names = new std::vector<std::string>();

    // We need to create the powerset construction for the first part of the automaton.

    // Keeps a dictionary giving us a relationship between pairs of sets of states and states in the result automata.
    powerset_state_dictionary powerset_sdict;
    // List of states we still need to go through.
    powerset_todo_list powerset_todo;

    state_set initial_state_set{aut->get_init_state_number()};

    int initial_state = powerset_set_to_state(result, &powerset_sdict, &powerset_todo, names, initial_state_set);

    result->set_init_state(initial_state);

    while (!powerset_todo.empty())
    {
        powerset_todo_state todo_now = powerset_todo.back();
        powerset_todo.pop_back();
        state_set states = std::get<0>(todo_now);
        int state_now = std::get<1>(todo_now);

        // First, we will use the state set to generate a map from formulae to state_sets to create new states.
        std::map<bdd, state_set, spot::bdd_less_than> states_cond_map;
        for (auto& state : states)
        {
            for (auto& edge : aut->out(state))
            {
                //std::cout << edge.dst << std::endl;
                if (to_determinize.find(edge.dst) == to_determinize.end()) {
                    continue;
                }
                std::vector<bdd> minterm_conds = edge_condition_to_minterms(allap, edge.cond, &minterms);

                for(auto& minterm_cond : minterm_conds)
                {
                    states_cond_map[minterm_cond].insert(edge.dst);
                }
            }
        }

        for (auto& cond : states_cond_map)
        {
            state_set new_set = states_cond_map[cond.first];
            int new_state = powerset_set_to_state(result, &powerset_sdict, &powerset_todo, names, new_set);
            result->new_edge(state_now, new_state, cond.first);
        }
    }

    // Now we need to connect the first with the second component
    auto names_2 = aut->get_named_prop<std::vector<std::string>>("state-names");
    // Copy the second component into result
    state_map old_new_map;
    result->copy_acceptance_of(aut);
    for (unsigned i = 0; i < aut->num_states(); i++)
    {
        if (to_determinize.find(i) != to_determinize.end()) {
            continue;
        }
        auto new_index = result->new_state();
        old_new_map.insert(std::pair<unsigned,unsigned>(i,new_index));
        std::string name;
        if (names_2 == nullptr)
        {
            name = std::to_string(i);
        }
        else
        {
            name=names_2->at(i);
        }
        names->push_back(name);
    }

    for (unsigned i = 0; i < aut->num_states(); i++)
        {
            if (to_determinize.find(i) != to_determinize.end()) {
                continue;
            }
        for (auto& edge : aut->out(i))
        {
            result->new_edge(old_new_map[i],old_new_map[edge.dst],edge.cond, edge.acc);
        }
    }

    //Iterate over the states in the first component and find their edges to the second component
    for (auto& res_state : powerset_sdict) {
        state_set states = std::get<0>(res_state);
        int state_now = std::get<1>(res_state);

        for (auto& state : states)
        {
            for (auto& edge : aut->out(state))
            {
                if (to_determinize.find(edge.dst) != to_determinize.end()) {
                    continue;
                }
                result->new_edge(state_now,old_new_map[edge.dst],edge.cond, edge.acc);
            }
        }
    }

    result->set_named_prop("state-names", names);
    result->merge_edges();
}
