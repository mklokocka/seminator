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
#pragma once

#include <types.hpp>
#include <powerset.hpp>
#include <cutdet.hpp>
#include <bscc.hpp>

/*
* Gives the name for a breakpoint state of the form: P, Q, level
* Example:  {q1, q2, q3}, {q1, q3}, 0
*/
std::string bp_name(breakpoint_state);

class bp_twa {
  public:
    bp_twa(const_aut_ptr src_aut, bool cut_det, const_om_ptr om)
      : cut_det_(cut_det),
        src_(src_aut),
        src_si_(spot::scc_info(src_aut)),
        om_(om),
        psb_(new powerset_builder(src_)) {
      if (om) {
        scc_aware_ = om->get("scc-aware",1);
        powerset_for_weak_ = om->get("powerset-for-weak",1);
        powerset_on_cut_ = om->get("powerset-on-cut",1);
        jump_to_bottommost_ = om->get("jump-to-bottommost",1);
        skip_levels_ = om->get("skip-levels",1);
        reuse_SCC_ = om->get("reuse-deterministic",1);
        cut_always_ = om->get("cut-always",1);
        cut_on_SCC_entry_ = om->get("cut-on-SCC-entry",0);
        bscc_avoid_ = (om->get("bscc-avoid", 1) || reuse_SCC_) ?
          std::make_unique<bscc_avoid>(src_si_) : nullptr;
      }

      res_ = spot::make_twa_graph(src_->get_dict());
      res_->copy_ap_of(src_);

      // Set the acceptance conditions
      // Büchi, unless reuse_SCC_ option declared, which reuses the input acc
      if(reuse_SCC_)
      {
        acc_mark_ = src_->get_acceptance().used_sets();
        res_->set_acceptance(src_->get_acceptance());
      } else
        res_->set_buchi();

      create_first_component();

      const auto first_comp_size = res_->num_states();
      // Resize the num2bp_ for new states to be at appropriete indices.
      num2bp_.resize(first_comp_size);
      num2ps2_.resize(first_comp_size);
      assert(names_->size() == first_comp_size);

      // spot::print_hoa(std::cout, src_);
      // std::cout << "\n\n" << std::endl;

      // print_res(new std::string("After 1st component built"));

      create_all_cut_transitions();

      // print_res('After cut');

      finish_second_component(first_comp_size);

      res_->merge_edges();

      if(jump_to_bottommost_) remove_useless_prefixes();


      // spot::print_hoa(std::cout, src_);
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
    * \brief Returns state for given state from src_.
    *
    * In case we have not seen this state before, establishes the mapping.
    *
    * @param[in] state_t
    * returns    state_t (unsigned)
    */
    state_t reuse_state(state_t);

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

    /**
     * \brief Removes states that have equivalent states in other SCCs.
     *
     * States of the form s=(R,B,l) are equivalent to states s'=(R',B',l')
     * if R=R'. If we have two such states in different SCCs C and C', we
     * can keep only the one in SCC C' iff we know that C is not reachable
     * from C'. This assumption is guaranteed if C > C' in the reverse
     * topological order used by Spot.
     */
    void remove_useless_prefixes();

    // \brief print the res_ automaton on std::cout and set its name to `name`
    void print_res(std::string * name = nullptr);

  private:
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


    /**
     * Returns whether a cut transition (jump to the deterministic component)
     * for the current edge should be created.
     */
    bool cut_condition(const edge_t& e);

    // Construction modifiers; Change the default also in constructor
    std::unique_ptr<bscc_avoid> bscc_avoid_ = nullptr;
    bool cut_det_ = false; // true if cut-determinism is requested
    bool powerset_for_weak_ = false;
    bool powerset_on_cut_ = false; //start bp already on cut
    bool jump_to_bottommost_ = false;
    bool reuse_SCC_ = false;
    bool scc_aware_ = true;
    bool skip_levels_ = false;
    bool cut_always_ = false;
    bool cut_on_SCC_entry_ = false;

    // input and result automata
    const_aut_ptr src_;
    aut_ptr res_;

    // scc info of src (needed for scc-aware optimization)
    spot::scc_info src_si_;
    acc_mark acc_mark_ = acc_mark({0});

    // Transformation options
    const_om_ptr om_;

    // mapping between power_states and their indices
    //(1st comp. of res_ or 2nd comp. of res for weak components)
    power_map  ps2num1_  = power_map();
    succ_vect  num2ps1_  = succ_vect();
    power_map  ps2num2_  = power_map();
    succ_vect  num2ps2_  = succ_vect();

    // mapping between states of 2nd comp. of res_ and their content
    breakpoint_map bp2num_ = breakpoint_map();        // bp_state -> state_t
    std::vector<breakpoint_state>
          num2bp_ = std::vector<breakpoint_state>();  // state_t  -> bp_state

    // mapping between reused semi-det states.
    state_map old2new2_ = state_map();
    state_map new2old2_ = state_map();

    // names of res automata states
    state_names names_ = new std::vector<std::string>;

    // Builder of powerset successors
    powerset_builder* psb_;
};
