#include <powerset.hpp>

static state_set bv_to_ps(const spot::bitvect* in)
{
  state_set ss;
  unsigned ns = in->size();
  for (unsigned pos = 0; pos < ns; ++pos)
    if (in->get(pos))
      ss.insert(pos);
  return ss;
}

/**
* Converts a set of states into bitvector
*/
static void ps_to_bv(state_set * ps, spot::bitvect * bv)
{
  for (auto s : *ps)
    bv->set(s);
}

std::string powerset_name(state_set state)
{
  if (state.size() == 0)
    return "∅";

  std::stringstream ss;
  ss << "{";
  for (auto state: state)
    ss << state << ',';
  //Remove the last comma
  ss.seekp(-1,ss.cur);
  ss << "}";

  return ss.str();
}

succ_vect *
powerset_builder::get_succs(state_set ss, unsigned mark, state_set * intersect) {
  if (ss == empty_set)
    return new succ_vect(nc_, empty_set);

  auto sm = pw_storage.at(mark);

  auto i_bv = spot::make_bitvect(ns_);
  if (intersect)
    ps_to_bv(intersect, i_bv);
  else
    i_bv->set_all();

  // outgoing map
  auto om = std::unique_ptr<bitvect_array>(spot::make_bitvect_array(ns_, nc_));
  auto result = new succ_vect;

  for (auto s : ss)
  {
    if (sm->count(s) == 0)
    { // Compute the bitvector_array with powerset transitions
      auto bva = compute_bva(s, mark);
      (*sm)[s] = bva;
    }
    // Add the successors into outgoing bitvector
    for (unsigned c = 0; c < nc_; ++c)
      (om->at(c) |= sm->at(s)->at(c)) &= *i_bv;
  }
  // Convert bitvector for each condition into a set
  for (unsigned c = 0; c < nc_; ++c)
  {
    auto ps = bv_to_ps(&om->at(c));
    result->emplace_back(std::move(ps));
  }
  delete i_bv;

  return result;
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
