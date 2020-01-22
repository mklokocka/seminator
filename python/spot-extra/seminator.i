// -*- coding: utf-8 -*-
// Copyright (C) 2019, 2020  The Seminator Authors
//
// This file is part of Seminator.
//
// Seminator is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// Seminator is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

%module(package="spot", director="1") seminator

%include "std_string.i"
%include "exception.i"
%import(module="spot.impl") "std_set.i"
%include "std_shared_ptr.i"

%shared_ptr(spot::bdd_dict)
%shared_ptr(spot::twa)
%shared_ptr(spot::twa_graph)


%{
#include <seminator.hpp>
%}

%import(module="spot.impl") <spot/misc/common.hh>
%import(module="spot.impl") <spot/twa/bdddict.hh>
%import(module="spot.impl") <spot/twa/fwd.hh>
%import(module="spot.impl") <spot/twa/twa.hh>
%import(module="spot.impl") <spot/twa/twagraph.hh>
%import(module="spot.impl") <spot/misc/optionmap.hh>

%exception {
  try {
    $action
  }
  catch (const std::runtime_error& e)
  {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

%rename(semi_determinize_cpp) semi_determinize;
%include <seminator.hpp>


%pythoncode %{
import spot

def semi_determinize(input,
                     cut_det=False,
                     jobs=AllJobs,
                     scc_aware=True,
                     powerset_for_weak=True,
                     powerset_on_cut=True,
                     jump_to_bottommost=True,
                     skip_levels=True,
                     reuse_deterministic=True,
                     cut_on_scc_entry=False,
                     cut_always=True,
                     bscc_avoid=True,
                     preprocess=False,
                     postprocess=True,
                     output=TGBA):
  if type(input) is str:
    input = spot.automaton(input)
  if type(input) is spot.formula:
    input = input.translate('deterministic', 'tgba')
  om = spot.option_map()
  om.set("scc-aware", int(scc_aware))
  om.set("powerset-for-weak", int(powerset_for_weak))
  om.set("powerset-on-cut", int(powerset_on_cut))
  om.set("jump-to-bottommost", int(jump_to_bottommost))
  om.set("skip-levels", int(skip_levels))
  om.set("reuse-deterministic", int(reuse_deterministic))
  om.set("cut-on-SCC-entry", int(cut_on_scc_entry))
  om.set("cut-always", int(cut_always))
  om.set("bscc-avoid", int(bscc_avoid))
  om.set("preprocess", int(preprocess))
  om.set("postprocess", int(postprocess))
  om.set("output", int(output))
  return semi_determinize_cpp(input, cut_det, jobs, om)


def seminator(input, pure=False, highlight=False,
              complement=False, postprocess_comp=None,
              output=TGBA, **semi_determinize_args):
  if highlight and complement:
    raise RuntimeError("highlight and complement cannot be used together");
  if pure:
    kwargs = { 'powerset_for_weak': False,
               'powerset_on_cut': False,
               'jump_to_bottommost': False,
               'skip_levels': False,
               'reuse_deterministic': False,
               'bscc_avoid': False,
               'preprocess': False,
               'postprocess': False,
               'cut_always': False,
               'cut_on_scc_entry': False
             }
    kwargs.update(semi_determinize_args);
    if postprocess_comp is None:
      postprocess_comp = False
  else:
    kwargs = semi_determinize_args
    if postprocess_comp is None:
      postprocess_comp = True
  kwargs['output'] = TBA if complement else output
  res = semi_determinize(input, **kwargs)
  if highlight:
    highlight_components(res);
  if complement:
    postopts = ["BA" if kwargs['output'] == BA else "TGBA",
                "high" if postprocess_comp else "low",
                "small" if postprocess_comp else "any"]
    comp = None
    if complement != "pldi":
       comp = spot.complement_semidet(res).postprocess(*postopts)
    if complement != "spot":
       comp2 = complement_semidet(res).postprocess(*postopts)
       if comp is None or comp2.num_states() < comp.num_states():
          comp = comp2
    res = comp
  return res

%}
