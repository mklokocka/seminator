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

std::string powerset_name(power_state state)
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

std::vector<state_set> *
powerset_builder::get_succs(state_set ss, unsigned mark) {
  auto sm = pw_storage->at(mark);

  // outgoing map
  auto om = std::unique_ptr<bitvect_array>(spot::make_bitvect_array(ns_, nc_));
  auto result = new std::vector<state_set>;

  for (auto s : ss)
  {
    if (sm->count(s) == 0)
    { // Compute the bitvector_array with powerset transitions
      auto bva = compute_bva(s, mark);
      (*sm)[s] = bva;
    }
    // Add the successors into outgoing bitvector
    for (unsigned c = 0; c < nc_; ++c)
      om->at(c) |= sm->at(s)->at(c);
  }
  // Convert bitvector for each condition into a set
  for (unsigned c = 0; c < nc_; ++c)
  {
    auto ps = bv_to_ps(&om->at(c));
    result->emplace_back(std::move(ps));
  }
  return result;
}

spot::bitvect_array *
powerset_builder::compute_bva(state_t s, unsigned mark) {
  //create bitvect_array of `nc` bitvectors with `ns` bits
  auto bv = spot::make_bitvect_array(ns_, nc_);

  // Fill the bitvector: TODO
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
