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
#include <spot/misc/bddlt.hh>

#include <spot/twaalgos/sccinfo.hh>
//#include <spot/twaalgos/sccfilter.hh>

static const std::string VERSION_TAG = "v1.2.0dev";

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
bool jump_condition(spot::const_twa_graph_ptr, spot::twa_graph::edge_storage_t);
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
bool jump_enter = false;
bool jump_always = false;
