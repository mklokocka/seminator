//
// Created by psimovec on 9/16/20.
//


#pragma once

#include <types.hpp>
#include <powerset.hpp>
#include <cutdet.hpp>
#include <bscc.hpp>
#include <breakpoint_twa.hpp>


class slim : bp_twa{
  public:
    explicit slim(const_aut_ptr src_aut,const_om_ptr om ) : bp_twa(src_aut, false, om, false) {
      state_t src;
      res_->set_named_prop("state-names", names_);
//      breakpoint_state dest(0, empty_set, empty_set);
      breakpoint_state bps;
      state_set start({0});
      std::get<Bp::LEVEL>(bps) = 0;
      std::get<Bp::P>    (bps) = start;
      std::get<Bp::Q>    (bps) = empty_set;
      src = bp_state(bps);
//      auto ps = {0};
//      auto intersection = get_and_check_scc(ps);
//      auto succs = psb_->get_succs<>(&ps, intersection.begin(), intersection.end());
//      src = ps_state(succs->at(0), false);

      res_->set_init_state(src);
//      const auto first_comp_size = res_->num_states();
//      // Resize the num2bp_ for new states to be at appropriete indices.
//      num2bp_.resize(first_comp_size);
//      num2ps2_.resize(first_comp_size);
//      assert(names_->size() == first_comp_size);

//      auto succs = psb_->get_succs<>(&ps, intersection->begin(), intersection->end());
//      state_set * current_states = &(num2ps1_.at(0));
//      compute_successors<state_t>(new2old2_[src], src);
//      finish_second_component(src);
      for (state_t sorc = src; sorc < res_->num_states(); ++sorc) {
        auto bops = num2bp_.at(sorc);
        auto intersection = get_and_check_scc(std::get<Bp::P>(bops));
        compute_successors(bops, sorc, &intersection);
      }
//      res_->merge_edges();
//      compute_successors(ps, src, &intersection);
      spot::print_hoa(std::cout, res_);
    }
};