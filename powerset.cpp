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

static state_set bv_to_ps(const spot::bitvect* in, spot::scc_info * si, unsigned scc)
{
  state_set ss;
  unsigned ns = in->size();
  for (unsigned pos = 0; pos < ns; ++pos)
    if (in->get(pos)) {
      if (si->scc_of(pos) != scc)
        continue;
      ss.insert(pos);
    }
  return ss;
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

std::vector<state_set> *
powerset_builder::get_succs(state_set ss, bool single_scc, unsigned mark) {
  if (ss == empty_set)
    return new std::vector<state_set>(nc_, empty_set);

  unsigned scc;

  if (single_scc)
  {
    scc = si_.scc_of(*ss.begin());
    for (auto s : ss)
      if (si_.scc_of(s) != scc)
        throw std::runtime_error("States of input set are from different SCCs");
  }

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
    state_set ps;
    if (single_scc) {
      ps = bv_to_ps(&om->at(c), &si_, scc);
    } else {
      ps = bv_to_ps(&om->at(c));
    }
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
