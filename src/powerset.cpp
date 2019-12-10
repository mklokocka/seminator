#include <powerset.hpp>

std::string powerset_name(state_set * ps)
{
  if (ps->size() == 0)
    return "∅";

  std::stringstream ss;
  ss << "{";
  for (auto state: *ps)
    ss << state << ',';
  //Remove the last comma
  ss.seekp(-1,ss.cur);
  ss << "}";

  return ss.str();
}

spot::bitvect_array *
powerset_builder::compute_bva(state_t s, unsigned mark) {
  //create bitvect_array of `nc` bitvectors with `ns` bits
  auto bv = spot::make_bitvect_array(ns_, nc_);

  bdd allap = src_->ap_vars();
  for (auto& t: src_->out(s))
  {
    if ((!t.acc.has(mark)) && mark < src_->num_sets())
      continue;
    bdd all = t.cond;
    while (all != bddfalse)
    {
      bdd one = bdd_satoneset(all, allap, bddfalse);
      all -= one;
      unsigned num = bdd2num_[one];
      bv->at(num).set(t.dst);
    }
  }
  return bv;
}
