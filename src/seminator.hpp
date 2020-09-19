// Copyright (c) 2017-2020  The Seminator Authors
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

#include <set>
#include <spot/twaalgos/postproc.hh>
#include <spot/misc/optionmap.hh>

enum jobs_type_values { ViaTGBA = 1,
                        ViaTBA = 2,
                        ViaSBA = 4,
                        AllJobs = ViaTGBA | ViaTBA | ViaSBA};
typedef int jobs_type;

enum output_type : int {TGBA = 0, TBA = 1, BA = 2, SLIM = 3};

/**
* Transform the automaton aut into a semi-deterministic equivalent automaton.
* Produce a cut-deterministic automaton if cut_det is true.
*
* Fine-tuning options may be passed via opt and jobs.
*/
spot::twa_graph_ptr semi_determinize(spot::twa_graph_ptr aut,
                                     bool cut_det = false,
                                     jobs_type jobs = AllJobs,
                                     const spot::option_map* opt = nullptr);

namespace from_spot {
  /// \brief Complement a semideterministic TωA
  ///
  /// The automaton \a aut should be semideterministic.
  ///
  /// Uses the NCSB algorithm described by F. Blahoudek, M. Heizmann,
  /// S. Schewe, J. Strejček, and MH. Tsai (TACAS'16).
  /// Implements optimization suggested by YF. Chen, M. Heizmann,
  /// O. Lengál, Y. Li, MH. Tsai, A. Turrini, and L. Zhang (PLDI'18)
  spot::twa_graph_ptr
  complement_semidet(const spot::const_twa_graph_ptr &aut, bool show_names = false);
}

typedef std::set<unsigned> state_set;

/**
 * A function that checks whether a given automaton is cut-deterministic.
 *
 * @param[in] aut               The automata to check for cut-determinism.
 * @param[out] non_det_states   Vector of the states that block cut-determinism.
 * @return Whether the automaton is cut-deterministic or not.
 */
bool is_cut_deterministic(spot::const_twa_graph_ptr aut,
                          std::set<unsigned>* non_det_states = nullptr);

/**
* Colors components and cut-edges of the given semi-deterministic automaton
*
* @param[in] aut                Semi-deterministic automaton
* @param[in, optional] nondet   States of the 1st component.
*                               Will be computed if nullptr (default)
*/
void highlight_components(spot::twa_graph_ptr aut,
                          bool edges = true, state_set * nondet = nullptr);

/**
* Colors cut-edges of the given semi-deterministic automaton
*
* @param[in] aut                Semi-deterministic automaton
* @param[in, optional] nondet   States of the 1st component.
*                               Will be computed if nullptr (default)
*/
void highlight_cut(spot::twa_graph_ptr aut,
                   state_set * nondet = nullptr);
