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

#include <cutdet.hpp>

static unsigned NONDET_C = 3;
static unsigned DET_C = 4;
static unsigned CUT_C = 5;

bool is_cut_deterministic(const_aut_ptr aut, std::set<unsigned>* non_det_states)
{
    unsigned UNKNOWN = 0;
    unsigned IN_CUT = 1;
    unsigned NOT_IN_CUT = 2;

    // Take the basic semi-determinism check as implemented in SPOT.
    spot::scc_info si(aut);
    si.determine_unknown_acceptance();
    unsigned nscc = si.scc_count();
    assert(nscc);
    std::vector<bool> reachable_from_acc(nscc);

    std::vector<unsigned> cut(si.scc_count());

    bool cut_det = true;

    do // iterator of SCCs in reverse topological order
    {
        --nscc;
        if (si.is_accepting_scc(nscc) || reachable_from_acc[nscc])
        {
            cut[nscc] = IN_CUT;

            for (unsigned succ: si.succ(nscc))
                reachable_from_acc[succ] = true;

            for (unsigned src: si.states_of(nscc))
            {
                bdd available = bddtrue;
                for (auto& t: aut->out(src))
                    if (!bdd_implies(t.cond, available)) // Not even semi-deterministic.
                      cut_det = false;
                    else
                      available -= t.cond;
            }
        }
    }
    while (nscc);

    for (unsigned i = 0; i < si.scc_count(); i++)
    {
        if (!si.is_accepting_scc(i) && !reachable_from_acc[i])
        {
            for (unsigned succ: si.succ(i))
              if (cut[succ] == NOT_IN_CUT)
                  cut[i] = NOT_IN_CUT;

            std::set<unsigned> edge_states;

            // Check determinism of this component.
            for (unsigned src: si.states_of(i))
            {
                bdd available = bddtrue;
                for (auto& t: aut->out(src))
                {
                    // We check whether the state is at the edge of the SCC.
                    if (si.scc_of(t.dst) != i)
                      edge_states.insert(src);
                    else if (!bdd_implies(t.cond, available))
                      cut_det = false;// SCC not det. => automaton not cut-det.
                    else
                      available -= t.cond;
                }
            }

            if (cut[i] == UNKNOWN)
            {
                bool is_in_cut = true;
                // Inner part of SCC deterministic, what about the outgoing edges?
                for (unsigned edge_state : edge_states)
                {
                    bdd available = bddtrue;
                    for (auto& t: aut->out(edge_state))
                      if (!bdd_implies(t.cond, available))
                        is_in_cut = false;
                      else
                        available -= t.cond;
                }

                cut[i] = is_in_cut ? IN_CUT : NOT_IN_CUT;
            }
            else if (cut_det)
            {
                // SCC cannot be in the cut, check if transitions outside cut
                // are deterministic.
                for (unsigned edge_state : edge_states)
                {
                    bdd available = bddtrue;
                    for (auto& t: aut->out(edge_state))
                        if (cut[si.scc_of(t.dst)] != IN_CUT) {
                            if (!bdd_implies(t.cond, available))
                                cut_det = false;
                            else
                                available -= t.cond;
                        }
                }
            }
        }
        else
            cut[i] = IN_CUT;
    }

    if (non_det_states != nullptr)
        for (unsigned scc = 0; scc < cut.size(); scc++)
            if (cut[scc] != IN_CUT)
                for (unsigned state: si.states_of(scc))
                    non_det_states->insert(state);

    if (cut_det)
      // Spot has no property for cut-deterministic, but at least
      // any cut-deterministic automaton is semi-deterministic.
      std::const_pointer_cast<spot::twa_graph>(aut)
        ->prop_semi_deterministic(true);
    return cut_det;
}

aut_ptr determinize_first_component(const_aut_ptr src, state_set * to_determinize)
{
  auto res = spot::make_twa_graph(src->get_dict());
  res->copy_ap_of(src);
  res->set_acceptance(src->get_acceptance());
  auto names = new std::vector<std::string>;

  // Setup the powerset construction
  auto ps2num = std::unique_ptr<power_map>(new power_map);
  auto num2ps = std::unique_ptr<succ_vect>(new succ_vect);
  auto psb = std::unique_ptr<powerset_builder>(new powerset_builder(src));

  // returns the state`s index, creates a new state if needed
  auto get_state = [&](state_set ps) {
    if (ps2num->count(ps) == 0)
    {
      // create a new state
      assert(num2ps->size() == res->num_states());
      num2ps->emplace_back(ps);
      auto state = res->new_state();
      (*ps2num)[ps] = state;
      //TODO add to bp1 states

      names->emplace_back(powerset_name(&ps));
      return state;
    } else
      return ps2num->at(ps);

  };

  // Set the initial state
  state_t init_num = src->get_init_state_number();
  state_set ps{init_num};
  res->set_init_state(get_state(ps));

  // Compute powerset with respect to to_determinize
  for (state_t s = 0; s < res->num_states(); ++s)
  {
    auto ps = num2ps->at(s);
    auto succs = std::unique_ptr<succ_vect>(psb->get_succs(&ps,
                                            to_determinize->begin(),
                                            to_determinize->end()));
    for(size_t c = 0; c < psb->nc_; ++c)
    {
      auto cond = psb->num2bdd_[c];
      auto d_ps = succs->at(c);
      // Skip transitions to ∅
      if (d_ps == empty_set)
        continue;
      auto dst = get_state(d_ps);
      res->new_edge(s, dst, cond);
    }
  }

  // remeber for later stop iteration when adding cut transitions
  auto lsize = res->num_states();

  // Copy the second part
  typedef std::map<state_t, state_t> state_map;
  state_map old2new;
  state_map new2old;
  for (state_t s = 0; s < src->num_states(); s++)
  {
    if (to_determinize->count(s) > 0) // skip states from the 1st component
      continue;
    auto ns = res->new_state();
    old2new[s] = ns;
    new2old[ns] = s;
    names->emplace_back(std::to_string(s));
  }
  // Now copy the transitions
  for (state_t s = 0; s < src->num_states(); s++)
  {
    if (to_determinize->count(s) > 0) // skip states from the 1st component
      continue;
    for (auto e : src->out(s))
      res->new_edge(old2new[e.src],old2new[e.dst],e.cond,e.acc);
  }

  for (state_t ns = 0; ns < lsize; ns++)
  {
    auto ps = num2ps->at(ns);
    auto succs = std::unique_ptr<succ_vect>(psb->get_succs(&ps,
                                            to_determinize->begin(),
                                            to_determinize->end(),
                                            true
                                           ));
    for(size_t c = 0; c < psb->nc_; ++c)
    {
      auto cond = psb->num2bdd_[c];
      auto d_ps = succs->at(c);
      // Skip transitions to ∅
      if (d_ps == empty_set)
        continue;
      for (auto s : d_ps)
        res->new_edge(ns, old2new[s], cond);
    }
  }


  res->merge_edges();
  res->set_named_prop("state-names", names);
  return res;
}

void highlight_components(aut_ptr aut, bool edges, state_set * nondet)
{
  assert(spot::is_semi_deterministic(aut));
  bool del = false;
  if (nondet == nullptr)
  {
    nondet = new state_set();
    is_cut_deterministic(aut,  nondet);
    del = true;
  }
  assert(nondet);

  auto* highlight = aut->get_or_set_named_prop<std::map<unsigned, unsigned>>
      ("highlight-states");
  state_t ns = aut->num_states();
  for (state_t src = 0; src < ns; ++src)
  {
    if (nondet->count(src))
      (*highlight)[src] = NONDET_C;
    else
      (*highlight)[src] = DET_C;
  }

  if (edges)
    highlight_cut(aut, nondet);

  if (del)
    delete(nondet);
}

void highlight_cut(aut_ptr aut, state_set * nondet)
{
  assert(spot::is_semi_deterministic(aut));
  bool del = false;
  if (nondet == nullptr)
  {
    nondet = new state_set();
    is_cut_deterministic(aut,  nondet);
    del = true;
  }
  assert(nondet);

  auto* highlight = aut->get_or_set_named_prop<std::map<unsigned, unsigned>>
      ("highlight-edges");
  for (auto& e : aut->edges())
  {
    // Cut is between nondet and det
    if (!nondet->count(e.src))
      continue;
    if (nondet->count(e.dst))
      continue;
    (*highlight)[aut->edge_number(e)] = CUT_C;
  }

  if (del)
    delete(nondet);
}

bool
is_deterministic_scc(unsigned scc, spot::scc_info& si,
                     bool inside_only)
{
  for (unsigned src: si.states_of(scc))
  {
    bdd available = bddtrue;
    for (auto& t: si.get_aut()->out(src))
    {
      if (inside_only && (si.scc_of(t.dst) != scc))
        continue;
      if (!bdd_implies(t.cond, available))
        return false;
      else
        available -= t.cond;
    }
  }
  return true;
}

