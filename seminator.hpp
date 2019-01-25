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

#include <utility>

#include <stdexcept>
#include <unistd.h>

#include <powerset.hpp>
#include <types.hpp>

#include <spot/misc/bddlt.hh>


static const std::string VERSION_TAG = "v1.2.0dev";

static const unsigned TGBA = 0;
static const unsigned TBA = 1;
static const unsigned BA = 2;

bool jump_enter = false;
bool jump_always = false;

// TO REMOVE!!!
typedef std::tuple<int, state_set, state_set, unsigned> todo_state;
typedef std::vector<todo_state> todo_list;
typedef std::map<state_set, unsigned> powerset_state_dictionary;
typedef std::tuple<state_set, unsigned> powerset_todo_state;
typedef std::vector<powerset_todo_state> powerset_todo_list;
typedef std::map<unsigned,unsigned> state_map;
// TO REMOVE
#include <stack>




/**
 * The semi-determinization algorithm as thought of by F. Blahoudek, J. Strejcek and M. Kretinsky.
 *
 * @param[in] aut TGBA to transform to a semi-deterministic TBA.
 */
spot::twa_graph_ptr buchi_to_semi_deterministic_buchi(spot::twa_graph_ptr& aut, bool deterministic_first_component, bool optimization, unsigned output);

/**
 * Helper function to copy all the states and transitions of an automaton to another one.
 * Doesn't keep any flags besides state Buchi acceptance. Both automata need to have the same dictionary.
 *
 * @param[out] aut          Automata to copy to.
 * @param[in]  to_copy      Automata to copy from.
 * @param[in, out] sdict    Dictionary of states in second component to their number.
 * @param[in, out] todo     Todo for generating the second component.
 * @param[out] names        Names of the states for printing purposes.
 */
void copy_buchi(spot::twa_graph_ptr aut, spot::const_twa_graph_ptr to_copy, breakpoint_map* sdict, todo_list* todo, std::vector<std::string>* names);

/**
 * Helper function to convert an edge condition to a vector of all the different minterms.
 *
 * @param[in] allap         All atomic propositions used through out the automata.
 * @param[in] cond          Edge condition.
 * @param[in, out] minterms A map of edge conditions to minterms that we have already found.
 * @return Vector of minterms.
 */
std::vector<bdd> edge_condition_to_minterms(bdd allap, bdd cond, std::map<bdd, std::vector<bdd>, spot::bdd_less_than>* minterms);

/**
 * Helper function that pairs two sets of states an already existing (or new, in the case it does not exist) state in the new automata.
 *
 * @param[in,out] aut The new automata.
 * @param[in,out] sdict Dictionary keeping track of the relationship between pair of sets of states and new states in the new automata.
 * @param[out]    todo Vector of states we have to go through next.
 * @param[out]    names Vector of names that we keep to name the states of the new automata.
 * @param[in]     left Left set of states.
 * @param[in]     right Right set of states.
 * @return Returns a number of the state that either already existed or was created a new representing the pair of sets of states from the old automata.
 */
unsigned sets_to_state(spot::twa_graph_ptr aut, breakpoint_map* sdict, todo_list* todo, std::vector<std::string>* names, int k, state_set left, state_set right);

/**
 * Helper function that pairs a set of states an already existing (or new, in the case it does not exist) state in the powerset construction of an automata.
 *
 * @param[in,out] aut The powerset automata.
 * @param[in,out] sdict Dictionary keeping track of the relationship between pair of sets of states and new states in the new automata.
 * @param[out]    todo Vector of states we have to go through next.
 * @param[out]    names Vector of names that we keep to name the states of the new automata.
 * @param[in]     states Left set of states.
 * @return Returns a number of the state that either already existed or was created a new representing the pair of sets of states from the old automata.
 */
unsigned powerset_set_to_state(spot::twa_graph_ptr aut, powerset_state_dictionary* sdict, powerset_todo_list* todo, std::vector<std::string>* names, state_set states);


void determinize_first_component(spot::twa_graph_ptr result, spot::twa_graph_ptr aut, std::set<unsigned> to_determinize);


/**
 * A function that checks whether a given automaton is cut-deterministic.
 *
 * @param[in] aut               The automata to check for cut-determinism.
 * @param[out] non_det_states   Vector of the states that block cut-determinism.
 * @return Whether the automaton is cut-deterministic or not.
 */
bool is_cut_deterministic(const spot::twa_graph_ptr& aut, std::set<unsigned>* non_det_states = nullptr);

/**
* Returns whether a cut transition (jump to the deterministic component) for the
* current edge should be created.
*
* @param[in] aut                The input automaton_stream_parser
* @param[in] e                  The edge beeing processed
* @return True if some jump condition is satisfied
*
* Currently, 4 conditions trigger the jump:
*  1. If the input automaton is safety (accepts all)
*  2. If the edge has the highest mark
*  3. If we freshly enter accepting scc (--jump-enter only)
*  4. If e leads to accepting SCC (--jump-always only)
*/
bool jump_condition(const_aut_ptr, spot_edge);

/*
* Gives the name for a breakpoint state of the form: level, P, Q
* Example:  0, {p1, p2}, {q1, 12}
*/
std::string bp_name(breakpoint_state);

// Simple and PowerSet in 1st component,
// BreakPoint and PowerSet in 2nd component
enum class State_type {SIMPLE1,PS1,BP2,PS2};

class bp_twa {
  public:
    bp_twa(const_aut_ptr src_aut, bool cut_det) :
    src_(src_aut), cut_det_(cut_det),
    psb_(new powerset_builder(src_)) {
      create_first_component();
      res_->set_buchi();

      const auto first_comp_size = res_->num_states();
      // Resize the num2bp_ for new states to be at appropriete indices.
      num2bp_->resize(first_comp_size);
      assert(names_->size() == first_comp_size);

      spot::print_hoa(std::cout, src_);
      std::cout << "\n\n" << std::endl;

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


      finish_second_component(first_comp_size);

      res_->merge_edges();
      res_->set_named_prop<std::string>("automaton-name", new std::string("My result"));
      spot::print_hoa(std::cout, res_);
      std::cout << "\n\n" << std::endl;


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

    // needed for determinization in case of cut-det is requester
    spot::power_map pm_ = spot::power_map();

    // Builder of powerset successors
    powerset_builder* psb_;

    /**
    * Sets the names of the automaton `aut` build by the `tgba_powerset`
    *
    * @param aut The result of `spot::tgba_powerset`
    * @param pm  Power_map filled by `spot::tgba_powerset`
    */
    void set_powerset_names();

    // Creates res_ and its 1st component
    void create_first_component();

    void finish_second_component(state_t);
};


/**
 * Class representing an exception thrown when the algorithm is not run on a proper SBwA automata.
 */
class not_tgba_exception: public std::runtime_error {
public:
    not_tgba_exception()
        : std::runtime_error("algorithm excepts a TGBA")
        {}
};

/**
 * Class representing an exception thrown when trying to copy to an automata with a different BDD dictionary.
 */
class mismatched_bdd_dict_exception: public std::runtime_error {
public:
    mismatched_bdd_dict_exception()
        : std::runtime_error("the source and destination automata require to have the same BDD dictionary")
        {}
};

/**
 * Class representing an exception thrown when the resulting automata is not semi-deterministic (this, of course, should not happen).
 */
class not_semi_deterministic_exception: public std::runtime_error {
public:
    not_semi_deterministic_exception()
        : std::runtime_error("the resulting automata is not semi-deterministic")
        {}
};

/**
 * Class representing an exception thrown when the resulting automata is not cut-deterministic (this, of course, should not happen).
 */
class not_cut_deterministic_exception: public std::runtime_error {
public:
    not_cut_deterministic_exception()
        : std::runtime_error("the resulting automata is not cut-deterministic")
        {}
};
