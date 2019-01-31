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

state_t
bp_twa::bp_state(breakpoint_state bps) {
  unsigned result;
  if (bp2num_.count(bps) == 0) {
    // create a new state
    assert(num2bp_.size() == res_->num_states());
    result = res_->new_state();
    num2bp_.emplace_back(bps);
    num2ps2_.resize(num2bp_.size());
    bp2num_[bps] = result;
    //TODO add to bp2 states

    auto name = bp_name(bps);
    names_->emplace_back(name);
  }  else {
    // return the existing one
    result = bp2num_.at(bps);
  }
  return result;
}

// _s a new state if needed
state_t
bp_twa::ps_state(state_set ps, bool fc) {
  auto num2ps = fc ? &num2ps1_ : &num2ps2_;
  auto ps2num = fc ? &ps2num1_ : &ps2num2_;

  if (ps2num->count(ps) == 0) {
    // create a new state
    assert(num2ps->size() == res_->num_states());
    num2ps->emplace_back(ps);
    if (!fc)
      num2bp_.resize(num2ps2_.size());
    auto state = res_->new_state();
    (*ps2num)[ps] = state;
    //TODO add to bp1 states

    names_->emplace_back(powerset_name(ps));
    return state;
  } else
    return ps2num->at(ps);
};

void
bp_twa::create_all_cut_transitions() {
  for (auto& edge : src_->edges())
  {
    if (jump_condition_(src_, edge))
    {
      if (cut_det_) {
        // in cDBA, add cut-edge from each state that contains edge.src
        for (unsigned s = 0; s < ps2num1_.size(); ++s) {
          state_set * current_states = &(num2ps1_.at(s));
          if (current_states->count(edge.src))
            add_cut_transition(s, edge);
        }
      } else {// in sDBA add (s, cond, dest)
        add_cut_transition(edge.src, edge);
      }
    }
  }
}


// This is to be used for weak components or for 1st component of
// cut-deterministic automata (the fc switch)
// The edges in 2nd component are all is_accepting
// No edges are accepting in the first component
template <> void
bp_twa::compute_successors<state_set>(state_set ps, state_t src,
  state_vect * intersection,
  bool fc, bdd cond_constrain)
{
  assert(ps != empty_set);

  typedef spot::acc_cond::mark_t acc_mark;

  auto succs = psb_->get_succs<>(ps, intersection->begin(), intersection->end());
  for(size_t c = 0; c < psb_->nc_; ++c) {
    auto cond = psb_->num2bdd_[c];
    if (!bdd_implies(cond, cond_constrain))
      continue;
    auto d_ps = succs->at(c);
    // Skip transitions to ∅
    if (d_ps == empty_set)
      continue;
    auto dst = ps_state(d_ps, fc);
    res_->new_edge(src, dst, cond, fc ?
      acc_mark() :
      acc_mark({0}));
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
    ps2num1_[ps] = num;
    num2ps1_.emplace_back(ps);
    names_->emplace_back(powerset_name(ps));

    // Build the transitions
    for (state_t src = 0; src < res_->num_states(); ++src)
    {
      auto ps = num2ps1_.at(src);
      compute_successors(ps, src, true);
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

template <> void
bp_twa::compute_successors<breakpoint_state>(breakpoint_state bps, state_t src,
  state_vect * intersection,
  bool fc, bdd cond_constrain)
{
  state_set p = std::get<Bp::P>(bps);
  state_set q = std::get<Bp::Q>(bps);
  int k       = std::get<Bp::LEVEL>(bps);

  assert(p != empty_set);
  assert(!fc);

  succ_vect_ptr p_succs   (psb_->get_succs(p,
                          intersection->begin(), intersection->end()));
  succ_vect_ptr q_succs   (psb_->get_succs(q,
                          intersection->begin(), intersection->end()));
  succ_vect_ptr p_k_succs (psb_->get_succs(p, k, // go to Q
                          intersection->begin(), intersection->end()));

  for(size_t c = 0; c < psb_->nc_; ++c) {
    bdd cond = psb_->num2bdd_[c];
    if (!bdd_implies(cond, cond_constrain))
      continue;
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
      succ_vect_ptr tmp (psb_->get_succs(p, k2,
                        intersection->begin(), intersection->end()));
      q2 = tmp->at(c);
      if (p2 == q2)
        q2 = empty_set;
    }
    // Construct the breakpoint_state. We use get just to be error-prone
    breakpoint_state bpd;
    std::get<Bp::LEVEL>(bpd) = k2;
    std::get<Bp::P>    (bpd) = p2;
    std::get<Bp::Q>    (bpd) = q2;

    auto dst = bp_state(bpd);
    res_->new_edge(src, dst, cond, acc);
  }
}

void
bp_twa::add_cut_transition(state_t from, edge_t edge) {

  auto scc = src_si_.scc_of(edge.dst);
  bool weak = src_si_.weak_sccs()[scc];

  state_vect scc_states;
  if (true) // TODO true -> scc_optim
    scc_states = src_si_.states_of(scc);


  state_t target_state;

  if (!breakpoint_jump_)
  {
    // create the target state
    state_set new_set{edge.dst};
    if (weak_powerset_ && weak)
      target_state = ps_state(new_set, false);
    else
    {
      // (level, P=new_set, Q=∅)
      breakpoint_state dest(0, new_set, empty_set);
      target_state = bp_state(dest);
    }
    res_->new_edge(from, target_state, edge.cond);
  } else {
    state_set start({edge.src});
    if (weak_powerset_ && weak)
      compute_successors(start, from, &scc_states, false, edge.cond);
    else
    {
      breakpoint_state bps;
      std::get<Bp::LEVEL>(bps) = 0;
      std::get<Bp::P>    (bps) = start;
      std::get<Bp::Q>    (bps) = empty_set;
      compute_successors(bps, from, &scc_states, false, edge.cond);
    }
  }
}

state_vect
bp_twa::get_and_check_scc(state_set ps) {
  state_vect intersection;
  //TODO true -> scc_optim
  if (true)
  { // create set of states from current SCC
    auto scc = src_si_.scc_of(*(ps.begin()));
    for (auto s : ps)
      assert(src_si_.scc_of(s) == scc);
    intersection = src_si_.states_of(scc);
  }
  return intersection;
}

void
bp_twa::finish_second_component(state_t start) {
  for (state_t src = start; src < res_->num_states(); ++src)
  {
    // Resolve the type of state and run compute_successors
    auto ps = num2ps2_.at(src);
    if (ps == empty_set)
    {
      auto bps = num2bp_.at(src);
      auto intersection = get_and_check_scc(std::get<Bp::P>(bps));
      compute_successors(bps, src, &intersection);
    }
    else
    {
      auto intersection = get_and_check_scc(ps);
      compute_successors(ps, src, &intersection);
    }
  }
}

void
bp_twa::print_res(std::string * name)
{
  if (name)
    res_->set_named_prop<std::string>("automaton-name", name);
  spot::print_hoa(std::cout, res_);
  std::cout << std::endl;
}
