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
#pragma once

#include <set>
#include <spot/twaalgos/postproc.hh>
#include <spot/misc/optionmap.hh>

enum { Onestep = 1,
       ViaTBA = 2,
       ViaSBA = 4,
       AllJobs = Onestep | ViaTBA | ViaSBA};
typedef int jobs_type;

enum output_type : int {TGBA = 0, TBA = 1, BA = 2};

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
