// Copyright (C) 2017, 2019, UT at Austin
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

#include <deque>
#include <map>

#include <spot/misc/hashfunc.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/sccinfo.hh>

#include <types.hpp>

namespace from_spot
{
    namespace
    {
        enum ncsb
        {
            ncsb_n = 0,       // non deterministic
            ncsb_c = 2 ,       // needs check
            ncsb_cb = 3,      // needs check AND in breakpoint
            ncsb_s = 4,       // safe
            ncsb_m = 1,       // missing
        };

        typedef std::vector<ncsb> mstate;
        typedef std::vector<std::pair<unsigned, ncsb>> small_mstate;

        struct small_mstate_hash
        {
            size_t
            operator()(small_mstate s) const noexcept
            {
              size_t hash = 0;
              for (const auto& p: s)
              {
                hash = spot::wang32_hash(hash ^ ((p.first<<2) | p.second));
              }
              return hash;
            }
        };

        class ncsb_complementation
        {
        private:
            // The source automaton.
            const spot::const_twa_graph_ptr aut_;

            // SCCs information of the source automaton.
            spot::scc_info si_;

            // Number of states in the input automaton.
            unsigned nb_states_;

            // The complement being built.
            spot::twa_graph_ptr res_;

            // Association between NCSB states and state numbers of the
            // complement.
            std::unordered_map<small_mstate, unsigned, small_mstate_hash> ncsb2n_;

            // States to process.
            std::deque<std::pair<mstate, unsigned>> todo_;

            // Support for each state of the source automaton.
            std::vector<bdd> support_;

            // Propositions compatible with all transitions of a state.
            std::vector<bdd> compat_;

            // Whether a SCC is deterministic or not
            std::vector<bool> is_deter_;

            // Whether a state only has accepting transitions
            std::vector<bool> is_accepting_;

            // State names for graphviz display
            std::vector<std::string>* names_;

            // Show NCSB states in state name to help debug
            bool show_names_;

            std::string
            get_name(const small_mstate& ms)
            {
              std::string res = "{";

              bool first_state = true;
              for (const auto& p: ms)
                if (p.second == ncsb_n)
                {
                  if (!first_state)
                    res += ",";
                  first_state = false;
                  res += std::to_string(p.first);
                }

              res += "},{";

              first_state = true;
              for (const auto& p: ms)
                if (p.second & ncsb_c)
                {
                  if (!first_state)
                    res += ",";
                  first_state = false;
                  res += std::to_string(p.first);
                }

              res += "},{";

              first_state = true;
              for (const auto& p: ms)
                if (p.second == ncsb_s)
                {
                  if (!first_state)
                    res += ",";
                  first_state = false;
                  res += std::to_string(p.first);
                }

              res += "},{";

              first_state = true;
              for (const auto& p: ms)
                if (p.second == ncsb_cb)
                {
                  if (!first_state)
                    res += ",";
                  first_state = false;
                  res += std::to_string(p.first);
                }

              return res + "}";
            }

            small_mstate
            to_small_mstate(const mstate& ms)
            {
              unsigned count = 0;
              for (unsigned i = 0; i < nb_states_; ++i)
                count+= (ms[i] != ncsb_m);
              small_mstate small;
              small.reserve(count);
              for (unsigned i = 0; i < nb_states_; ++i)
                if (ms[i] != ncsb_m)
                  small.emplace_back(i, ms[i]);
              return small;
            }

            // From a NCSB state, looks for a duplicate in the map before
            // creating a new state if needed.
            unsigned
            new_state(mstate&& s)
            {
              auto p = ncsb2n_.emplace(to_small_mstate(s), 0);
              if (p.second) // This is a new state
              {
                p.first->second = res_->new_state();
                if (show_names_)
                  names_->push_back(get_name(p.first->first));
                todo_.emplace_back(std::move(s), p.first->second);
              }
              return p.first->second;
            }

            void
            ncsb_successors(mstate&& ms, unsigned origin, bdd letter)
            {
              std::vector <mstate> succs;
              succs.emplace_back(nb_states_, ncsb_m);

              // PLDI: accepting transitions when B' would be empty
              std::vector <bool> acc_succs;
              acc_succs.push_back(false);

              // Handle S states.
              //
              // Treated first because we can escape early if the letter
              // leads to an accepting transition for a Safe state.
              for (unsigned i = 0; i < nb_states_; ++i)
              {
                if (ms[i] != ncsb_s)
                  continue;

                for (const auto &t: aut_->out(i))
                {
                  if (!bdd_implies(letter, t.cond))
                    continue;
                  if (t.acc || is_accepting_[t.dst])
                    // Exit early; transition is forbidden for safe
                    // state.
                    return;

                  succs[0][t.dst] = ncsb_s;

                  // No need to look for other compatible transitions
                  // for this state; it's in the deterministic part of
                  // the automaton
                  break;
                }
              }

              // Handle C states.
              for (unsigned i = 0; i < nb_states_; ++i)
              {
                if (!(ms[i] & ncsb_c))
                  continue;

                for (const auto &t: aut_->out(i))
                {
                  if (!bdd_implies(letter, t.cond))
                    continue;

                  // PLDI optimization:
                  // Compute C' and remove states that are already in S'
                  // We have still only unique successor
                  if (succs[0][t.dst] == ncsb_m)
                    succs[0][t.dst] = ncsb_c;

                  // No need to look for other compatible transitions
                  // for this state; it's in the deterministic part of
                  // the automaton
                  break;
                }
              }

              // Handle N states.
              for (unsigned i = 0; i < nb_states_; ++i)
              {
                if (ms[i] != ncsb_n)
                  continue;
                for (const auto &t: aut_->out(i))
                {
                  if (!bdd_implies(letter, t.cond))
                    continue;

                  // PLDI: All states from 2nd component go to C only.
                  // PLDI: We have still only a unique successor
                  if (is_deter_[si_.scc_of(t.dst)])
                  {
                    if (succs[0][t.dst] == ncsb_m)
                      succs[0][t.dst] = ncsb_c;
                  } else
                    for (auto &succ: succs)
                      succ[t.dst] = ncsb_n;
                }
              }

              // PLDI: Handle B states. We need to know what remained in C'.
              // PLDI: We first move all successors to B', and then pereform
              // branching in next pass
              for (unsigned i = 0; i < nb_states_; ++i)
              {
                if (ms[i] != ncsb_cb)
                  continue;

                bool has_succ = false;
                for (const auto &t: aut_->out(i))
                {
                  if (!bdd_implies(letter, t.cond))
                    continue;

                  has_succ = true;
                  if (succs[0][t.dst] == ncsb_c)
                      succs[0][t.dst] = ncsb_cb;

                  // PLDI: If t is not accepting and t.dst in S, stop
                  // because t.src should have been i S already.
                  if (!t.acc && (succs[0][t.dst] == ncsb_s))
                    return;

                  // No need to look for other compatible transitions
                  // for this state; it's in the deterministic part of
                  // the automaton
                  break;
                }
                if (!has_succ && !is_accepting_[i])
                  return;
              }

              // Allow to move accepting dst to S'
              for (unsigned i = 0; i < nb_states_; ++i)
              {
                if (ms[i] != ncsb_cb)
                  continue;

                for (const auto &t: aut_->out(i))
                {
                  if (!bdd_implies(letter, t.cond))
                    continue;

                  if (t.acc)
                  {
                    // double all the current possible states
                    unsigned length = succs.size();
                    for (unsigned j = 0; j < length; ++j)
                    {
                      if ((succs[j][t.dst] == ncsb_cb) & (!is_accepting_[t.dst]))
                      {
                        succs.push_back(succs[j]);
                        succs.back()[t.dst] = ncsb_s;
                        acc_succs.push_back(false);
                      }
                    }
                  }
                }
              }

              // PLDI: For each possible successor check if B' might be empty
              // If yes, double the successors for each state in C', make edges
              // to all clones accepting.
              {
                unsigned length = succs.size();
                for (unsigned j = 0; j < length; ++j)
                {
                  // Check for empty B'
                  bool b_empty = true;
                  for (unsigned i = 0; i < nb_states_; ++i)
                  {
                    if (succs[j][i] == ncsb_cb) {
                      b_empty = false;
                      break;
                    }
                  }

                  if (b_empty)
                  {
                    //PLDI: for each state s in C', move it to B'
                    // if s is not accepting make a clone
                    // of all succs in new_succs where s is in S'
                    for (unsigned i = 0; i < nb_states_; ++i) {
                      if (succs[j][i] != ncsb_c) {
                        continue;
                      }
                      succs[j][i] = ncsb_cb;
                    }

                    // Set edge as accepting
                    acc_succs[j] = true;
                    std::vector <mstate> new_succs; // Store clones of current succ
                    new_succs.push_back(succs[j]);

                    //PLDI: for each state s in C'
                    // if s is not accepting make a clone
                    // of all succs in new_succs where s is in S'
                    for (unsigned i = 0; i < nb_states_; ++i) {
                      if (succs[j][i] != ncsb_cb) {
                        continue;
                      }

                      {
                        unsigned k_length = new_succs.size();
                        for (unsigned k = 0; k < k_length; ++k) {
                          //PLDI: skip accepting states
                          if (is_accepting_[i])
                            continue;

                          // Make copy of k with i moved from C to S
                          new_succs.push_back(new_succs[k]);
                          new_succs.back()[i] = ncsb_s;
                        }
                      }
                      // new_succs[0] is succ[j] with C -> CB
                      succs[j] = new_succs[0];
                      // Move the rest to the end of succ
                      unsigned k_length = new_succs.size();
                      for (unsigned k = 1; k < k_length; ++k) {
                        succs.push_back(new_succs[k]);
                        acc_succs.push_back(true);
                      }
                    }
                  }
                }
              }

              // Create the automaton states
              unsigned length = succs.size();
              for (unsigned j = 0; j < length; ++j)
              {
                if (acc_succs[j])
                {
                  unsigned dst = new_state(std::move(succs[j]));
                  res_->new_edge(origin, dst, letter, {0});
                } else {
                  unsigned dst = new_state(std::move(succs[j]));
                  res_->new_edge(origin, dst, letter);
                }
              }
            }

        public:
            ncsb_complementation(const spot::const_twa_graph_ptr& aut, bool show_names)
                    : aut_(aut),
                      si_(aut),
                      nb_states_(aut->num_states()),
                      support_(nb_states_),
                      compat_(nb_states_),
                      is_accepting_(nb_states_),
                      show_names_(show_names)
            {
              res_ = spot::make_twa_graph(aut->get_dict());
              res_->copy_ap_of(aut);
              res_->set_buchi();

              // Generate bdd supports and compatible options for each state.
              // Also check if all its transitions are accepting.
              for (unsigned i = 0; i < nb_states_; ++i)
              {
                bdd res_support = bddtrue;
                bdd res_compat = bddfalse;
                bool accepting = true;
                bool has_transitions = false;
                for (const auto& out: aut->out(i))
                {
                  has_transitions = true;
                  res_support &= bdd_support(out.cond);
                  res_compat |= out.cond;
                  if (!out.acc)
                    accepting = false;
                }
                support_[i] = res_support;
                compat_[i] = res_compat;
                is_accepting_[i] = accepting && has_transitions;
              }



              // Compute which SCCs are part of the deterministic set.
              is_deter_ = spot::semidet_sccs(si_);

              if (show_names_)
              {
                names_ = new std::vector<std::string>();
                res_->set_named_prop("state-names", names_);
              }

              // Because we only handle one initial state, we assume it
              // belongs to the N set. (otherwise the automaton would be
              // deterministic)
              unsigned init_state = aut->get_init_state_number();
              mstate new_init_state(nb_states_, ncsb_m);
              new_init_state[init_state] = ncsb_n;
              res_->set_init_state(new_state(std::move(new_init_state)));
            }

            spot::twa_graph_ptr
            run()
            {
              // Main stuff happens here

              while (!todo_.empty())
              {
                auto top = todo_.front();
                todo_.pop_front();

                mstate ms = top.first;

                // Compute support of all available states.
                bdd msupport = bddtrue;
                bdd n_s_compat = bddfalse;
                bdd c_compat = bddtrue;
                bool c_empty = true;
                for (unsigned i = 0; i < nb_states_; ++i)
                  if (ms[i] != ncsb_m)
                  {
                    msupport &= support_[i];
                    // PLDI: add ms[i] == ncsb_c as those states could be also virtually in S
                    if (ms[i] == ncsb_n || ms[i] == ncsb_s || ms[i] == ncsb_c || is_accepting_[i])
                      n_s_compat |= compat_[i];
                    else
                    {
                      c_empty = false;
                      c_compat &= compat_[i];
                    }
                  }

                bdd all;
                if (!c_empty)
                  all = c_compat;
                else
                {
                  all = n_s_compat;
                  if (all != bddtrue)
                  {
                    mstate empty_state(nb_states_, ncsb_m);
                    res_->new_edge(top.second,
                                   new_state(std::move(empty_state)),
                                   !all,
                                   {0});
                  }
                }
                while (all != bddfalse)
                {
                  bdd one = bdd_satoneset(all, msupport, bddfalse);
                  all -= one;

                  // Compute all new states available from the generated
                  // letter.
                  ncsb_successors(std::move(ms), top.second, one);
                }
              }

              res_->merge_edges();
              return res_;
            }
        };

    }

    spot::twa_graph_ptr
    complement_semidet(const spot::const_twa_graph_ptr& aut, bool show_names)
    {
      if (!is_semi_deterministic(aut))
        throw std::runtime_error
                ("complement_semidet() requires a semi-deterministic input");

      auto ncsb = ncsb_complementation(aut, show_names);
      return ncsb.run();
    }
}



