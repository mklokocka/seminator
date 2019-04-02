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
#include <powerset.hpp>

/*
* Gives the name for a breakpoint state of the form: level, P, Q
* Example:  0, {p1, p2}, {q1, 12}
*/
std::string bp_name(breakpoint_state);

class bp_twa {
  public:
    bp_twa(const_aut_ptr src_aut,
      bool cut_det, bool powerset_for_weak, bool powerset_on_cut, bool scc_aware,
      cut_condition_t cut_condition) :
    cut_det_(cut_det), powerset_for_weak_(powerset_for_weak),
    powerset_on_cut_(powerset_on_cut),
    scc_aware_(scc_aware),
    src_(src_aut), src_si_(spot::scc_info(src_aut)),
    cut_condition_(cut_condition),
    psb_(new powerset_builder(src_)) {
      res_ = spot::make_twa_graph(src_->get_dict());
      res_->copy_ap_of(src_);
      create_first_component();

      const auto first_comp_size = res_->num_states();
      // Resize the num2bp_ for new states to be at appropriete indices.
      num2bp_.resize(first_comp_size);
      num2ps2_.resize(first_comp_size);
      assert(names_->size() == first_comp_size);

      // spot::print_hoa(std::cout, src_);
      // std::cout << "\n\n" << std::endl;

      // print_res('After 1st component built');

      create_all_cut_transitions();

      // print_res('After cut');

      res_->set_buchi();
      finish_second_component(first_comp_size);

      res_->merge_edges();

      // spot::print_hoa(std::cout, src_);

      auto old_n = src_->get_named_prop<std::string>("automaton-name");
      if (old_n)
      {
      std::stringstream ss;
      ss << (cut_det_ ? "cDBA for " : "sDBA for ") << old_n;

      std::string * name = new std::string(ss.str());
      res_->set_named_prop("automaton-name", name);
      }
    }

    ~bp_twa()
    {
      delete psb_;
    }

    // Getters
    const_aut_ptr src_aut();
    aut_ptr res_aut();
    state_names names();

    /**
    * \brief Returns state for given value.
    *
    * In case such breakpoint_state does not exists, creates one.
    *
    * @param[in] bp_state (breakpoint_state = <level, state, state>)
    *                                           int , unsigned....
    * returns    state (unsigned)
    */
    state_t bp_state(breakpoint_state);

    /**
    * \brief Returns state for given value.
    *
    * In case such powerset_state does not exists, creates one.
    *
    * @param[in] ps_state (state_set)
    * @param[in] fc       (bool) do we built the 1st component?
    * returns    state (unsigned)
    */
    state_t ps_state(state_set, bool = false);

    /**
    * \brief Creates cut transitions after the first component was build.
    *
    * Iterates over the edges of the original automaton and if
    * `cut-condition()` is `true` it iterates over the states of the
    * sd-automaton (res_) and add a corresponding cut-transitions to those
    * that qualify:
    *   - for edge.src                      for sDBA
    *   - for states that include edge.src  for cDBA
    */
    void create_all_cut_transitions();

    /**
    * \brief Create cut transition from `from` built using `edge`
    *
    * @param[in] from (state_t) State in 1st component
    * @param[in] edge (edge_t)  Edge of the input automaton
    */
    void add_cut_transition(state_t, edge_t);

    // \brief print the res_ automaton on std::cout and set its name to `name`
    void print_res(std::string * name = nullptr);

  private:
    bool cut_det_; // true if cut-determinism is requested
    bool powerset_for_weak_;
    bool powerset_on_cut_; //start bp already on cut
    bool scc_aware_;

    // input and result automata
    const_aut_ptr src_;
    aut_ptr res_;

    // scc info of src (needed for scc-aware optimization)
    spot::scc_info src_si_;

    // fcnPointer to decide when jump from 1st to 2nd component (cut-transition)
    // The function should take 2 arguments:
    //
    //  1. const_aut_ptr src_
    //  2. edge_t edge
    //
    // The edge is an edge from src_
    cut_condition_t cut_condition_;

    // mapping between power_states and their indices
    //(1st comp. of res_ or 2nd comp. of res for weak components)
    power_map ps2num1_ = power_map();
    succ_vect num2ps1_ = succ_vect();
    power_map ps2num2_ = power_map();
    succ_vect num2ps2_ = succ_vect();

    // mapping between states of 2nd comp. of res_ and their content
    breakpoint_map bp2num_ = breakpoint_map();        // bp_state -> state_t
    std::vector<breakpoint_state>
          num2bp_ = std::vector<breakpoint_state>();  // state_t  -> bp_state

    // names of res automata states
    state_names names_ = new std::vector<std::string>;

    // Builder of powerset successors
    powerset_builder* psb_;

    // Creates res_ and its 1st component
    //
    // * for semi-deterministic automata only copy states and edges of src_ to
    // 1st component of res_ and remove marks from all transitions.
    //
    // * for cut-deterministic automata apply powerset construction.
    void create_first_component();

    // Performs the main part of the construction (breakpoint with levels)
    //
    // @param[in] state_t s: the lowest index of 2nd component's state
    //
    // Cycles through all states, and computes successors for them. Suitable
    // to use powerset construction for inherently_weak SCC.
    //
    void finish_second_component(state_t);

    // For a state_set S from src_ checks that all states in S are from the same
    // SCC and returns the vector of all states of this SCC.
    state_vect get_and_check_scc(state_set);

    // Create successors (and edges to them) for a given state
    //
    // The possible states right now are:
    //  * breakpoint_state
    //  * powerset (state_set)
    //
    // @param[in] T state     : the given state (may nat exist in any automaton)
    // @param[in] state_t from: state-index (in res_) of state to which we add
    //                          the computed edges (can be also used to add
    //                          behaviour of the given state to state `from`)
    // @param[in] state_vect intersection:
    //                        : all successors will be interesected with the
    //                          states given here (can be states of SCC)
    // @param[in] bool fc     : indicates whether the constructed states should
    //                          belong to the 1st component (for state_set only)
    // @param[in] bdd cond    : build only edges with label described by `cond`
    template <class T>
    void compute_successors (T, state_t, state_vect * intersection,
      bool first_comp = false, bdd cond_constrain = bddtrue);

    template <class T>
    void compute_successors (T from, state_t src,
      bool first_comp = false, bdd cond_constrain = bddtrue) {
        compute_successors<T>(from, src, new state_vect(), first_comp, cond_constrain);
      }
};
