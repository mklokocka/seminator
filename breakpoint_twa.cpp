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

#include <types.hpp>
#include <breakpoint_twa.hpp>

std::string bp_name(breakpoint_state bps) {
  state_set p = std::get<Bp::P>(bps);
  state_set q = std::get<Bp::Q>(bps);
  int level   = std::get<Bp::LEVEL>(bps);
  std::stringstream name;
  name << powerset_name(p) << " , " << powerset_name(q) << " , " << level;
  return name.str();
}

/// bp_twa
const_aut_ptr
bp_twa::src_aut() {
  return src_;
}

aut_ptr
bp_twa::res_aut() {
  return res_;
}

state_names
bp_twa::names() {
  return names_;
}

unsigned
bp_twa::add_cut_transition(unsigned from, unsigned to, bdd cond) {
  auto edge = res_->new_edge(from, to, cond);
  cut_trans_.emplace_back(edge);
  return edge;
}

unsigned
bp_twa::bp_state(breakpoint_state bps) {
  unsigned result;
  if (bp2num_->count(bps) == 0) {
    // create a new state
    assert(num2bp_->size() == res_->num_states());
    result = res_->new_state();
    num2bp_->emplace_back(bps);
    (*bp2num_)[bps] = result;
    //TODO add to bp2 states

    auto name = bp_name(bps);
    names_->emplace_back(name);
  }  else {
    // return the existing one
    result = bp2num_->at(bps);
  }
  return result;
}

void
bp_twa::create_all_cut_transitions() {
  for (auto& edge : src_->edges())
  {
    if (jump_condition_(src_, edge))
    {
      // create the target state
      state_set new_set{edge.dst};
      // (level, P=new_set, Q=∅)
      breakpoint_state dest(0, new_set, empty_set);
      int target_state = bp_state(dest);

      if (cut_det_) {
        // in cDBA, add cut-edge from each state that contains edge.src
        for (unsigned s = 0; s < ps2num_->size(); ++s) {
          state_set * current_states = &(num2ps_->at(s));
          if (current_states->count(edge.src)) {
            add_cut_transition(s, target_state, edge.cond);
          }
        }
      } else {// in sDBA add (s, cond, dest)
        add_cut_transition(edge.src, target_state, edge.cond);
      }
    }
  }
}

void
bp_twa::create_first_component()
{

  if (cut_det_) {
    // Set the initial state
    state_t num = res_->new_state();
    res_->set_init_state(num);
    state_t init_num = src_->get_init_state_number();
    state_set ps{init_num};
    (*ps2num_)[ps] = num;
    num2ps_->emplace_back(ps);
    names_->emplace_back(powerset_name(ps));

    // Creates a new state if needed
    auto get_state = [&](state_set ps) {
      if (ps2num_->count(ps) == 0) {
        // create a new state
        assert(num2ps_->size() == res_->num_states());
        num2ps_->emplace_back(ps);
        auto state = res_->new_state();
        (*ps2num_)[ps] = state;
        //TODO add to bp1 states

        names_->emplace_back(powerset_name(ps));
        return state;
      } else
        return ps2num_->at(ps);
    };

    // Build the transitions
    for (state_t src = 0; src < res_->num_states(); ++src)
    {
      auto ps = num2ps_->at(src);
      auto succs = psb_->get_succs(ps, false);
      for(size_t c = 0; c < psb_->nc_; ++c) {
        auto cond = psb_->num2bdd_[c];
        auto d_ps = succs->at(c);
        // Skip transitions to ∅
        if (d_ps == empty_set)
          continue;
        auto dst = get_state(d_ps);
        res_->new_edge(src, dst, cond);
      }
    }
    res_->merge_edges();
  } else { // Just copy the states and transitions
    res_->new_states(src_->num_states());
    res_->set_init_state(src_->get_init_state_number());

    // We remember the old numbers.
    for (unsigned i = 0; i < src_->num_states(); i++)
      names_->emplace_back(std::to_string(i));

    // Copy edges
    for (auto e : src_->edges()) {
      res_->new_edge(e.src, e.dst, e.cond);
    }
  }
  res_->set_named_prop("state-names", names_);
}

void
bp_twa::finish_second_component(state_t start) {
  for (state_t src = start; src < res_->num_states(); ++src)
  {
    auto bps = num2bp_->at(src);
    state_set p = std::get<Bp::P>(bps);
    state_set q = std::get<Bp::Q>(bps);
    int k       = std::get<Bp::LEVEL>(bps);

    //TODO true -> scc_optim
    auto p_succs   = psb_->get_succs(p, true);
    auto q_succs   = psb_->get_succs(q, true);
    auto p_k_succs = psb_->get_succs(p, true, k); // go to Q

    for(size_t c = 0; c < psb_->nc_; ++c) {
      auto p2   = p_succs->at(c);
      auto q2   = q_succs->at(c);
      auto p2_k = p_k_succs->at(c); // go to Q
      q2.insert(p2_k.begin(),p2_k.end());
      // Skip transitions to ∅
      if (p2 == empty_set)
        continue;

      auto k2 = k;
      // Check p == q
      auto acc = spot::acc_cond::mark_t();
      if (p2 == q2) {
        k2 = (k + 1) % src_->num_sets();
        acc = {0};
        // Take the k2-succs of p
        q2 = psb_->get_succs(p, true, k2)->at(c);
        if (p2 == q2)
          q2 = empty_set;
      }
      // Construct the breakpoint_state. We use get just to be error-prone
      breakpoint_state bpd;
      std::get<Bp::LEVEL>(bpd) = k2;
      std::get<Bp::P>    (bpd) = p2;
      std::get<Bp::Q>    (bpd) = q2;

      auto dst = bp_state(bpd);
      auto cond = psb_->num2bdd_[c];
      res_->new_edge(src, dst, cond, acc);
    }
  }
}
