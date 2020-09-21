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
      res_->set_named_prop("state-names", names_);
      state_t src;
      breakpoint_state bps;
      state_set start({0});
      std::get<Bp::LEVEL>(bps) = 0;
      std::get<Bp::P>    (bps) = start;
      std::get<Bp::Q>    (bps) = empty_set;
      src = bp_state(bps);
      res_->set_init_state(src);
      for (state_t sorc = src; sorc < res_->num_states(); ++sorc) {
        auto bops = num2bp_.at(sorc);
        auto intersection = get_and_check_scc(std::get<Bp::P>(bops));
        compute_successors(bops, sorc, &intersection);
      }
      spot::print_hoa(std::cout, res_);
    }
};