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

#include <powerset.hpp>

std::string powerset_name(state_set * ps)
{
  if (ps->size() == 0)
    return "∅";

  std::stringstream ss;
  ss << "{";
  for (auto state: *ps)
    ss << state << ',';
  //Remove the last comma
  ss.seekp(-1,ss.cur);
  ss << "}";

  return ss.str();
}

spot::bitvect_array *
powerset_builder::compute_bva(state_t s, unsigned mark) {
  //create bitvect_array of `nc` bitvectors with `ns` bits
  auto bv = spot::make_bitvect_array(ns_, nc_);

  bdd allap = src_->ap_vars();
  for (auto& t: src_->out(s))
  {
    if ((!t.acc.has(mark)) && mark < src_->num_sets())
      continue;
    bdd all = t.cond;
    while (all != bddfalse)
    {
      bdd one = bdd_satoneset(all, allap, bddfalse);
      all -= one;
      unsigned num = bdd2num_[one];
      bv->at(num).set(t.dst);
    }
  }
  return bv;
}
