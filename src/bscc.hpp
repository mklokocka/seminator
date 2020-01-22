// Copyright (C) 2017-2020  The Seminator Authors
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

#include <spot/twaalgos/isdet.hh>

#include <types.hpp>

/*
* Determine whether given SCC is bottom (has no successor SCC)
*
* @param[in] scc    scc to check
* @param[in] ss     precomputed scc_info
* @return           true if scc is bottom scc, false otherwise
*/
bool is_bottom_scc(unsigned scc, spot::scc_info& si);


class bscc_avoid
{
  std::vector<char> semidetsccs_;
  spot::scc_info& si_;
public:
  bscc_avoid(spot::scc_info& si);

  /*
   * decide whether the given SCC should be avoided in the 1st component
   */
  bool avoid_scc(unsigned scc)
  {
    //return is_deterministic_scc(scc, si, false) && is_bottom_scc(scc, si);
    return semidetsccs_[scc];
  }

  // Decides whether state should be avoided during bscc-avoid optimization
  bool avoid_state(state_t s)
  {
    return avoid_scc(si_.scc_of(s));
  }
};

/*
* Prints information about (non-)bottom SCC. For testing purposes only
*/
void print_scc_info(const_aut_ptr aut);
