// Copyright (C) 2017-2020  The Seminator Authors
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

#pragma once

#include <types.hpp>
#include <powerset.hpp>

#include <spot/twaalgos/isdet.hh>

/**
 * Determinizes the first part of input. The first part is given by to_determinize
 * that can be obtained by `is_cut_deterministic`. Returns a new automaton.
 */
aut_ptr determinize_first_component(const_aut_ptr, state_set * to_determinize);


/**
* Decide whether the given scc is is_deterministic
*
* @param inside_only    Checks only edges inside the scc
*/
bool
is_deterministic_scc(unsigned scc, spot::scc_info& si,
                     bool inside_only=true);

/*
* whether all successors of given SCC are deterministic
*/
std::vector<bool> get_semidet_sccs(spot::scc_info& si);
