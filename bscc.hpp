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

/*
* Determine whether given SCC is bottom (has no successor SCC)
*
* @param[in] aut    automaton
* @param[in] scc    scc to check
* @param[in] ss     precomputed scc_info
* @return           true if scc is bottom scc, false otherwise
*/
bool is_bottom_scc(const_aut_ptr aut,
                    unsigned scc,
                    spot::scc_info* si);

bool is_bottom_scc(const_aut_ptr aut, unsigned scc);


/*
* Prints information about (non-)bottom SCC. For testing purposes only
*/
void print_scc_info(const_aut_ptr aut);
