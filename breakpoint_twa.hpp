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
    bp_twa(const_aut_ptr src_aut, bool cut_det, jump_condition_t jump_condition) :
    src_(src_aut), src_si_(spot::scc_info(src_aut)),
    cut_det_(cut_det), jump_condition_(jump_condition),
    psb_(new powerset_builder(src_)) {
      res_ = spot::make_twa_graph(src_->get_dict());
      res_->copy_ap_of(src_);
      create_first_component();

      const auto first_comp_size = res_->num_states();
      // Resize the num2bp_ for new states to be at appropriete indices.
      num2bp_->resize(first_comp_size);
      assert(names_->size() == first_comp_size);

      // spot::print_hoa(std::cout, src_);
      // std::cout << "\n\n" << std::endl;

      // TO REMOVE
      // res_->set_named_prop<std::string>("automaton-name", new std::string("After powerset"));
      // spot::print_hoa(std::cout, res_);
      // std::cout << std::endl;
      // std::cout << std::endl;

      create_all_cut_transitions();

      // TO REMOVE
      // res_->set_named_prop<std::string>("automaton-name", new std::string("After cut"));
      // spot::print_hoa(std::cout, res_);
      // std::cout << std::endl;
      // std::cout << std::endl;

      res_->set_buchi();
      finish_second_component(first_comp_size);

      res_->merge_edges();

      // res_->set_named_prop<std::string>("automaton-name", new std::string("My result"));
      // spot::print_hoa(std::cout, res_);
      // std::cout << "\n\n" << std::endl;


      // TO REMOVE
      // breakpoint_state bs = num2bp_->at(4);
      // auto succs = psb_->get_succs(std::get<1>(bs));
      // // For each condition: print succ
      // for(size_t c = 0; c < psb_->nc_; ++c) {
      //   auto cond = psb_->num2bdd_[c];
      //   std::cout << "\nFor ";
      //   spot::bdd_print_formula(std::cout, res_->get_dict(), cond);
      //   auto name = powerset_name(succs->at(c));
      //   std::cout << " go to " << name;
      // }
      // std::cout.flush();
      // psb_->get_succs(std::get<1>(bs), 0);

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
    unsigned bp_state(breakpoint_state);

    /**
    * \brief Creates cut transitions after the first component was build.
    *
    * Iterates over the edges of the original automaton and if
    * `jump-condition()` is `true` it iterates over the states of the
    * sd-automaton (res_) and add a corresponding cut-transitions to those
    * that qualify:
    *   - for edge.src                      for sDBA
    *   - for states that include edge.src  for cDBA
    */
    void create_all_cut_transitions();

    /**
    * \brief Create new cut transition and store it in `cut_trans`
    *
    * @param[in] from (unsigned[state]) State in 1st component
    * @param[in] to (unsigned[state])   State in 2nd component
    * @param[in] cond (bdd)             Label of the cut-transition
    *
    * @return    (unsigned) the index of newly created edge in res_aut
    */
    unsigned add_cut_transition(state_t, state_t, bdd);

  private:
    bool cut_det_; // true if cut-determinism is requested

    // input and result automata
    const_aut_ptr src_;
    aut_ptr res_;

    // scc info of src (needed for scc-aware optimization)
    spot::scc_info src_si_;

    // fcnPointer to condition where to jump from 1st to 2nd component (cut-transition)
    // edge should be edge from src_
    jump_condition_t jump_condition_;

    // mapping between states of 1st comp. of res_ and their content for cut-det
    power_map * ps2num_ = new power_map;
    std::vector<state_set> * num2ps_ = new std::vector<state_set>;

    // mapping between states of 2nd comp. of res_ and their content
    breakpoint_map * bp2num_ = new breakpoint_map;               // bp_state -> state_t
    std::vector<breakpoint_state> * num2bp_ = new std::vector<breakpoint_state>;  // state_t  -> bp_state

    // names of res automata states
    state_names names_ = new std::vector<std::string>;

    // pointers to cut-edges (can be changed after merge_edges() is called)
    std::vector<unsigned> cut_trans_ = std::vector<unsigned>();

    // Builder of powerset successors
    powerset_builder* psb_;

    // Creates res_ and its 1st component
    void create_first_component();

    // Performs the main part of the construction (breakpoint with levels)
    void finish_second_component(state_t);
};
