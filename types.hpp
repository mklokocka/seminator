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

#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <iostream>
#include <bddx.h>
#include <spot/misc/bddlt.hh>
#include <spot/twa/twa.hh>
#include <spot/twaalgos/powerset.hh>
#include <spot/misc/bitvect.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/bddprint.hh>



typedef unsigned state_t;
typedef std::set<state_t> state_set;
typedef std::vector<state_t> state_vect;

typedef std::tuple<int, state_set, state_set> breakpoint_state;
typedef std::map<breakpoint_state, state_t> breakpoint_map;
typedef std::map<state_set, state_t> power_map;

typedef spot::const_twa_graph_ptr const_aut_ptr;
typedef spot::twa_graph_ptr aut_ptr;
typedef std::vector<std::string>* state_names;
typedef spot::twa_graph::edge_storage_t spot_edge;