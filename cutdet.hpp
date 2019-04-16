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

#include <types.hpp>
#include <breakpoint_twa.hpp>

#include <spot/twaalgos/isdet.hh>

/**
 * Determinizes the first part of input. The first part is given by to_determinize
 * that can be obtained by `is_cut_deterministic`. Returns a new automaton.
 */
aut_ptr determinize_first_component(const_aut_ptr, state_set * to_determinize);


/**
 * A function that checks whether a given automaton is cut-deterministic.
 *
 * @param[in] aut               The automata to check for cut-determinism.
 * @param[out] non_det_states   Vector of the states that block cut-determinism.
 * @return Whether the automaton is cut-deterministic or not.
 */
bool is_cut_deterministic(const_aut_ptr aut, std::set<unsigned>* non_det_states = nullptr);

/**
* Colors components and cut-edges of the given semi-deterministic automaton
*
* @param[in] aut                Semi-deterministic automaton
* @param[in, optional] nondet   States of the 1st component.
*                               Will be computed if nullptr (default)
*/
void highlight(aut_ptr, bool edges = true, state_set * nondet = nullptr);

/**
* Colors cut-edges of the given semi-deterministic automaton
*
* @param[in] aut                Semi-deterministic automaton
* @param[in, optional] nondet   States of the 1st component.
*                               Will be computed if nullptr (default)
*/
void highlight_cut(aut_ptr, state_set * nondet = nullptr);
