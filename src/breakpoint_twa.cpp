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

#include <string>

#include <types.hpp>
#include <breakpoint_twa.hpp>
#include <cutdet.hpp>

std::string bp_name(breakpoint_state bps) {
  state_set p = std::get<Bp::P>(bps);
  state_set q = std::get<Bp::Q>(bps);
  int level   = std::get<Bp::LEVEL>(bps);
  std::stringstream name;
  name << powerset_name(&p) << " , " << powerset_name(&q) << " , " << level;
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
  auto loc = bp2num_.lower_bound(bps);
  if (loc != bp2num_.end() && loc->first == bps)
    return loc->second;         // existing state

  // create a new state
  assert(num2bp_.size() == res_->num_states());
  unsigned result = res_->new_state();
  bp2num_.emplace_hint(loc, bps, result);

  // Update the state vectors to correct size
  num2bp_.emplace_back(bps);
  num2ps2_.resize(num2bp_.size());
  //TODO add to bp2 states

  names_->emplace_back(bp_name(bps));
  return result;
}

state_t
bp_twa::reuse_state(state_t old) {
  auto result_it = old2new2_.find(old);

  // state already exists, return it
  if (result_it != old2new2_.end())
    return result_it->second;

  // else create a new state
  unsigned result = res_->new_state();
  new2old2_[result] = old;
  old2new2_[old] = result;

  // Update the vector maps to be of the correct size
  num2ps2_.emplace_back(empty_set);
  num2bp_.emplace_back(breakpoint_state());
  //TODO add to bp2 states

  names_->emplace_back(std::to_string(old));
  return result;
}

// _s a new state if needed
state_t
bp_twa::ps_state(state_set ps, bool fc) {
  // fc = first component
  auto num2ps = fc ? &num2ps1_ : &num2ps2_;
  auto ps2num = fc ? &ps2num1_ : &ps2num2_;

  auto loc = ps2num->lower_bound(ps);
  if (loc != ps2num->end() && loc->first == ps)
    return loc->second;         // existing state

  // create a new state
  assert(num2ps->size() == res_->num_states());
  num2ps->emplace_back(ps);
  if (!fc) {
    num2bp_.resize(num2ps2_.size());
  }
  auto state = res_->new_state();
  ps2num->emplace_hint(loc, ps, state);
  //TODO add to bp1 states

  names_->emplace_back(powerset_name(&ps));
  return state;
};

void
bp_twa::create_all_cut_transitions() {
  for (auto& edge : src_->edges())
  {
    if (cut_condition(edge))
    {
      if (cut_det_) {
        // in cDBA, add cut-edge from each state that contains edge.src
        for (unsigned s = 0; s < ps2num1_.size(); ++s) {
          // Can we iterate over keys of ps2num1_?
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

// This is to be used for reused SCC during --reuse-deterministic
// Basicaly only copies the edges
template <> void
bp_twa::compute_successors<state_t>(state_t old, state_t src,
  state_vect * intersection,
  bool fc, bdd cond_constrain)
{
  assert(src == old2new2_[old]);
  assert(old == new2old2_[src]);

  for (auto& e : src_->out(old))
  {
    auto new_dst = reuse_state(e.dst);
    res_->new_edge(src, new_dst, e.cond, e.acc);
  }
}


// This is to be used for weak components or for 1st component of
// cut-deterministic automata (the fc switch)
// The edges in 2nd component are all accepting
// No edges are accepting in the first component
template <> void
bp_twa::compute_successors<state_set>(state_set ps, state_t src,
  state_vect * intersection,
  bool fc, bdd cond_constrain)
{
  assert(ps != empty_set);

  auto succs = psb_->get_succs<>(&ps, intersection->begin(), intersection->end());
  for(size_t c = 0; c < psb_->nc_; ++c) {
    auto cond = psb_->num2bdd_[c];
    if (!bdd_implies(cond, cond_constrain))
      continue;
    auto d_ps = succs->at(c);
    // Skip transitions to ∅
    if (d_ps == empty_set)
      continue;
    auto dst = ps_state(d_ps, fc);
    acc_mark mark = acc_mark();
    if (!fc)
      mark = acc_mark(acc_mark_);
    res_->new_edge(src, dst, cond, mark);
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
    names_->emplace_back(powerset_name(&ps));

    assert(!bscc_avoid_ || !bscc_avoid_->avoid_state(init_num));

    // Build the transitions
    // For the bscc-avoid we create intersection with states that are not avoided
    auto not_avoided = state_vect();
    for (unsigned scc = 0; scc < src_si_.scc_count(); ++scc)
    {
      if (bscc_avoid_ && bscc_avoid_->avoid_scc(scc))
        continue;
      not_avoided.insert(not_avoided.end(),
                         src_si_.states_of(scc).begin(),
                         src_si_.states_of(scc).end());
    }

    for (state_t src = 0; src < res_->num_states(); ++src)
    {
      auto ps = num2ps1_.at(src);
      compute_successors(ps, src, &not_avoided, true);
    }
    res_->merge_edges();
  } else { // Just copy the states and transitions
    res_->new_states(src_->num_states());
    res_->set_init_state(src_->get_init_state_number());

    // We remember the old numbers.
    for (unsigned i = 0; i < src_->num_states(); i++)
      names_->emplace_back(std::to_string(i));

    // Copy edges
    for (auto& e : src_->edges())
    {
      if (bscc_avoid_
          && (bscc_avoid_->avoid_state(e.dst)
              || (bscc_avoid_->avoid_state(e.src))))
        continue;
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
  //assert(!fc);

  succ_vect_ptr p_succs   (psb_->get_succs(&p,
                          intersection->begin(), intersection->end()));
  succ_vect_ptr q_succs   (psb_->get_succs(&q,
                          intersection->begin(), intersection->end()));
  succ_vect_ptr p_k_succs (psb_->get_succs(&p, k, // go to Q
                          intersection->begin(), intersection->end()));

  for(size_t c = 0; c < psb_->nc_; ++c)
  {
    bdd cond = psb_->num2bdd_[c];

    // don't build edges not satisfying cond_constraint
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

    do
    {
      if (!fc && p2 == q2) {
        k2 = (k2 + 1) % src_->num_sets();
        acc = acc_mark_;
        // Take the k2-succs of p
        succ_vect_ptr tmp (psb_->get_succs(&p, k2,
                          intersection->begin(), intersection->end()));
        q2 = tmp->at(c);
      } else
        break;
    } while ((k2 != k) && skip_levels_);

    // keep Q empty if all breakpoints were reached
    if (p2 == q2)
      q2 = empty_set;

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
  bool reuse = bscc_avoid_ && bscc_avoid_->avoid_scc(scc);

  state_vect scc_states;
  if (scc_aware_)
    scc_states = src_si_.states_of(scc);

  state_t target_state;

  if (reuse_SCC_ && reuse)
  {
    res_->new_edge(from, reuse_state(edge.dst), edge.cond);
    return;
  }

  if (!powerset_on_cut_)
  {
    // create the target state
    state_set new_set{edge.dst};
    if (powerset_for_weak_ && weak && !(reuse && bscc_avoid_))
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
    if (powerset_for_weak_ && weak && !(reuse && bscc_avoid_))
      compute_successors(start, from, &scc_states, false, edge.cond);
    else
    {
      breakpoint_state bps;
      std::get<Bp::LEVEL>(bps) = 0;
      std::get<Bp::P>    (bps) = start;
      std::get<Bp::Q>    (bps) = empty_set;
      compute_successors(bps, from, &scc_states, true, edge.cond);
    }
  }
}

state_vect
bp_twa::get_and_check_scc(state_set ps) {
  state_vect intersection;
  if (scc_aware_)
  { // create set of states from current SCC
    auto scc = src_si_.scc_of(*(ps.begin()));

    // For the bottom-SCC optimization we have to be carefull.
    // The components C that are in the cut (already satisfy the semi-det
    // property) we have to avoid the SCC optimization as they may have
    // some successor SCCs that would not be reachable if C is not in
    // the 1st component.
    if (bscc_avoid_ && bscc_avoid_->avoid_scc(scc))
      return intersection;

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
      if (new2old2_.find(src) != new2old2_.end())
        compute_successors<state_t>(new2old2_[src], src);
      else
      { // breakpoint
        auto bps = num2bp_.at(src);
        auto intersection = get_and_check_scc(std::get<Bp::P>(bps));
        compute_successors(bps, src, &intersection);
      }
    else
    { // powerset
      auto intersection = get_and_check_scc(ps);
      compute_successors(ps, src, &intersection);
    }
  }
}

// Returns whether a cut transition (jump to the deterministic component)
// for the current edge should be created.
bool bp_twa::cut_condition(const edge_t& e)
{
    unsigned u = src_si_.scc_of(e.src);
    unsigned v = src_si_.scc_of(e.dst);
    unsigned highest_mark = src_->acc().num_sets() - 1;

    // The states of u are not present in the 1st component
    // when it is deterministic BSCC and bscc_avoid is true
    // Maybe add avoid_scc(scc)?
    if (bscc_avoid_ && bscc_avoid_->avoid_scc(u))
      return false;
    // This is basically cut_on_SCC_entry for detBSCC as u != v
    if (bscc_avoid_ && bscc_avoid_->avoid_scc(v))
      return true;

    // Currently, 3 conditions trigger the jump:
    //  1. If the edge has the highest mark
    //  2. If we freshly enter accepting scc (--cut-on-SCC-entry option)
    //  3. If e leads to accepting SCC (--cut-always option)
    return (src_si_.is_accepting_scc(v) &&
            (cut_always_ || // 3
             e.acc.has(highest_mark) || //1
             (cut_on_SCC_entry_ && u != v))); // 2
}


void
bp_twa::print_res(std::string * name)
{
  if (name)
    res_->set_named_prop<std::string>("automaton-name", name);
  spot::print_hoa(std::cout, res_);
  std::cout << std::endl;
}

void
bp_twa::remove_useless_prefixes()
{
  auto empty_bp = breakpoint_state();
  auto res_ns = res_->num_states();

  // Compute bottommost
  std::map<state_set, state_t> bottommost_occurence;
  auto si_res = spot::scc_info(res_);
  unsigned res_scc_count = si_res.scc_count();

  {
    unsigned n = res_scc_count;
    do
      for (unsigned s: si_res.states_of(--n))
        {
          auto bps = num2bp_[s];
          if (bps == empty_bp) continue;
          state_set R = std::get<Bp::P>(bps);
          bottommost_occurence[R] = s;
        }
    while (n);
  }

  // Check what states we want to remove
  std::vector<unsigned> retarget(res_ns);
  for (unsigned n = 0; n < res_ns; ++n)
  {
    auto bps = num2bp_[n];
    retarget[n] = n;
    if (bps == empty_bp) continue;
    state_set R = std::get<Bp::P>(bps);
    unsigned other = bottommost_occurence[R];
    retarget[n] =
         (si_res.scc_of(n) != si_res.scc_of(other)) ? other : n;
  }

  // Retarget edges leading to to_remove states
  for (auto& e: res_->edges())
      e.dst = retarget[e.dst];
  res_->set_init_state(retarget[res_->get_init_state_number()]);
  res_->purge_unreachable_states();
}
