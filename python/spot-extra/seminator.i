// -*- coding: utf-8 -*-
// Copyright (C) 2019 Laboratoire de Recherche et Développement de
// l'Epita (LRDE).
//
// This file is part of Spot, a model checking library.
//
// Spot is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// Spot is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.
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
                     powerset_for_weak=False,
                     powerset_on_cut=False,
                     remove_prefixes=False,
                     skip_levels=False,
                     reuse_good_scc=False,
                     cut_on_scc_entry=False,
                     cut_always=False,
                     bscc_avoid=False,
                     preprocess=False,
                     postprocess=True):
  if type(input) is str:
    input = spot.automaton(input)
  if type(input) is spot.formula:
    input = input.translate('deterministic', 'tgba')
  om = spot.option_map()
  om.set("scc-aware", int(scc_aware))
  om.set("powerset-for-weak", int(powerset_for_weak))
  om.set("powerset-on-cut", int(powerset_on_cut))
  om.set("remove-prefixes", int(remove_prefixes))
  om.set("skip-levels", int(skip_levels))
  om.set("reuse-good-SCC", int(reuse_good_scc))
  om.set("cut-on-SCC-entry", int(cut_on_scc_entry))
  om.set("cut-always", int(cut_always))
  om.set("bscc-avoid", int(bscc_avoid))
  om.set("preprocess", int(preprocess))
  om.set("postprocess", int(postprocess))
  return semi_determinize_cpp(input, cut_det, jobs, om)


def seminator(input, pure=False, highlight=False, **semi_determinize_args):
  if pure:
    res = semi_determinize(input, **semi_determinize_args)
  else:
    # We should get better defaults than pure.
    res = semi_determinize(input, **semi_determinize_args)
  if highlight:
    highlight_components(res);
  return res

%}
