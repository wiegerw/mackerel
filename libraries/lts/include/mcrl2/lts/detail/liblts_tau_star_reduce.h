// Author(s): Muck van Weerdenburg, Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file lts/detail/liblts_tau_star_reduce.h

#ifndef _LIBLTS_TAUSTARREDUCE_H
#define _LIBLTS_TAUSTARREDUCE_H

#include "mcrl2/utilities/logger.h"
#include "mcrl2/lts/lts_utilities.h"
#include "mcrl2/lts/probabilistic_lts.h"

namespace mcrl2
{
namespace lts
{
namespace detail
{

//Replace sequences tau* a tau* by a single action a.


enum t_reach { unknown, reached, explored };

/// \brief This procedure calculates the transitive tau
///        closure as a separate vector of transitions, for
///        a given transition system.
/// \parameter l A labelled transition system
/// \parameter forward A boolean that indicates whether the resulting closure
//             points forward, to the next state, or backward, to the previous state.
/// \return A map from states to sets of states indicating for each state those
//          states that can be reached by one or more tau_steps.

template < class STATE_LABEL_T, class ACTION_LABEL_T, class LTS_BASE_CLASS >
std::map < std::size_t,
           std::set <typename lts<STATE_LABEL_T,ACTION_LABEL_T, LTS_BASE_CLASS>::states_size_type > > 
            calculate_non_reflexive_transitive_tau_closure(lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>& l,
            const bool forward)
{
  typedef typename lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>::states_size_type state_t;
  using namespace std;

  typedef map < state_t,set < state_t > > map_from_states_to_states;
  map_from_states_to_states resulting_tau_transitions;

  // Copy the internal transitions into the result.
  for(std::vector < mcrl2::lts::transition>::const_iterator i=l.get_transitions().begin(); i!=l.get_transitions().end(); ++i)
  {
    if (l.is_tau(l.apply_hidden_label_map(i->label())))
    {
      if (forward) 
      { 
        resulting_tau_transitions[i->from()].insert(i->to());
      }
      else
      {
        resulting_tau_transitions[i->to()].insert(i->from());
      }
    }
  }

  bool new_state_added=true;
  while (new_state_added)
  {
    new_state_added=false;
    for(typename map_from_states_to_states::iterator i=resulting_tau_transitions.begin(); 
                       i!=resulting_tau_transitions.end(); ++i)
    {
      const set<std::size_t>& outgoing_states= i->second;
      set<std::size_t> new_outgoing_states=outgoing_states;
      for(set<std::size_t>::const_iterator j=outgoing_states.begin(); j!=outgoing_states.end(); j++)
      {
        new_outgoing_states.insert(resulting_tau_transitions[*j].begin(),
                                   resulting_tau_transitions[*j].end());
      }
      if (i->second.size()<new_outgoing_states.size())
      { 
        i->second=new_outgoing_states;
        new_state_added=true;
      }
    }
  }

  return resulting_tau_transitions;
}


template < class STATE_LABEL_T, class ACTION_LABEL_T, class LTS_BASE_CLASS >
void reflexive_transitive_tau_closure(lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>& l)
// This method assumes there are no tau loops!
{
  using namespace std;
  typedef typename lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>::states_size_type state_t;
  const vector < transition >& original_transitions=l.get_transitions();
  set < transition> new_transitions;

  // Add for every tau*.a tau* transitions sequence a single transition a;
  map <state_t, set <state_t> > backward_tau_closure=calculate_non_reflexive_transitive_tau_closure(l,false);
  map <state_t, set <state_t> > forward_tau_closure=calculate_non_reflexive_transitive_tau_closure(l,true);
  for(const transition& t: original_transitions)
  {
    new_transitions.insert(t);
    set<state_t>& new_from_states=backward_tau_closure[t.from()];
    set<state_t>& new_to_states=forward_tau_closure[t.to()];
    for(typename set<state_t>::const_iterator j_from=new_from_states.begin(); j_from!=new_from_states.end(); ++j_from)
    {
      new_transitions.insert(transition(*j_from,t.label(),t.to()));
      for(typename set<state_t>::const_iterator j_to=new_to_states.begin(); j_to!=new_to_states.end(); ++j_to)
      {
        new_transitions.insert(transition(*j_from,t.label(),*j_to));
      }
    }
    for(typename set<state_t>::const_iterator j_to=new_to_states.begin(); j_to!=new_to_states.end(); ++j_to)
    {
      new_transitions.insert(transition(t.from(),t.label(),*j_to));
    }
  }

  l.clear_transitions();

  for(state_t i=0; i<l.num_states(); ++i)
  {
    new_transitions.insert(transition(i,l.tau_label_index(),i));
  }
  
  // Add the newly generated transitions
  for(set < transition >::const_iterator i=new_transitions.begin();
            i!=new_transitions.end(); ++i)
  {
    l.add_transition(*i);
  }
}


/// \brief Removes each transition s-a->s' if also transitions s-a->-tau->s' or s-tau->-a->s' are 
///        present. It uses the hidden_label_map to determine whether transitions are internal. 
template < class STATE_LABEL_T, class ACTION_LABEL_T, class LTS_BASE_CLASS >
void remove_redundant_transitions(lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>& l)
{
  typedef typename lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>::states_size_type state_type;
  typedef typename lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>::labels_size_type label_type;

  outgoing_transitions_per_state_t outgoing_transitions=transitions_per_outgoing_state(l.get_transitions());
  l.clear_transitions();
  std::set < state_type > states_reachable_in_one_visible_action;
  std::set < state_type > states_reachable_in_one_hidden_action;

  for(outgoing_transitions_per_state_t::const_iterator i=outgoing_transitions.begin(); i!=outgoing_transitions.end(); ++i)
  {
    const state_type from_=from(i);
    const label_type label_=label(i);
    const state_type to_=to(i);

    states_reachable_in_one_visible_action.clear();
    states_reachable_in_one_hidden_action.clear();

    // For every transition from-label->to we calculate the sets { s | from -a->s } and { s | from -tau-> s }.
    for(outgoing_transitions_per_state_t::const_iterator j=outgoing_transitions.lower_bound(from_);
                    j!=outgoing_transitions.upper_bound(from_); ++j)
    {
      if (l.is_tau(l.apply_hidden_label_map(label(j))))
      {
        states_reachable_in_one_hidden_action.insert(to(j));
      }
      else if (label_==label(j))
      {
        assert(!l.is_tau(l.apply_hidden_label_map(label_)));
        states_reachable_in_one_visible_action.insert(to(j)); 
      }
    }

    // Now check whether to is reachable in one step from one of the two sets constructed above. If no,
    // insert the transition in l.transitions. 
    bool found=false;
    
    for(const state_type& middle: states_reachable_in_one_hidden_action)
    {
      // Find a visible step from state middle to state to, unless label is hidden, in which case we search
      // a hidden step. 
      for(outgoing_transitions_per_state_t::const_iterator j=outgoing_transitions.lower_bound(middle);
                  !found && j!=outgoing_transitions.upper_bound(middle); ++j)
      {
        if (l.is_tau(l.apply_hidden_label_map(label_)))
        { 
          if (l.is_tau(l.apply_hidden_label_map(label(j))) && to(j)==to_)
          {
            assert(!found);
            found=true; break;
          }
        }
        else // label is visible.
        {
          if (label(j)==label_ && to(j)==to_)
          {
            assert(!found);
            found=true; break;
          }
        }
      }
      if (found) break;
    }
    
    if (!found && !l.is_tau(l.apply_hidden_label_map(label_)))
    {
      for(const state_type& middle: states_reachable_in_one_visible_action)
      {
        // Find a hidden step from state middle to state to.
        for(outgoing_transitions_per_state_t::const_iterator j=outgoing_transitions.lower_bound(middle);
                    !found && j!=outgoing_transitions.upper_bound(middle); ++j)
        {
          if (l.is_tau(l.apply_hidden_label_map(label(j))) && to(j)==to_)
          { 
            assert(!found);
            found=true; break;
          } 
        }
        if (found) break;
      }
    }

    // If no alternative transition is found, add this transition to l.transitions().
    if (!found) 
    {
      l.add_transition(transition(from_, label_, to_));
    }
  }  
}


template < class STATE_LABEL_T, class ACTION_LABEL_T, class LTS_BASE_CLASS >
void tau_star_reduce(lts< STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS >& l)
// This method assumes there are no tau loops!
{
  using namespace std;
  typedef typename lts<STATE_LABEL_T, ACTION_LABEL_T, LTS_BASE_CLASS>::states_size_type state_t;
  vector < transition >& original_transitions=l.get_transitions();
  set < transition> new_transitions;

  // Add all the original non tau transitions.
  for(std::vector < transition >::const_iterator i=original_transitions.begin(); i!=original_transitions.end(); ++i)
  {
    if (!l.is_tau(l.apply_hidden_label_map(i->label())))
    {
      new_transitions.insert(*i);
    }
  }

  // Add for every tau*.a transitions sequence a single transition a, provided a is not tau.
  map <state_t, set <state_t> > backward_tau_closure=calculate_non_reflexive_transitive_tau_closure(l,false);
  for(std::vector < transition >::const_iterator i=original_transitions.begin(); i!=original_transitions.end(); ++i)
  {
    if (!l.is_tau(l.apply_hidden_label_map(i->label())))
    {
      set<state_t>& new_from_states=backward_tau_closure[i->from()];
      for(typename set<state_t>::const_iterator j=new_from_states.begin(); j!=new_from_states.end(); ++j)
      {
        new_transitions.insert(transition(*j,i->label(),i->to()));
      }
    }
  }
  l.clear_transitions();
  
  // Add the newly generated transitions
  for(set < transition >::const_iterator i=new_transitions.begin();
            i!=new_transitions.end(); ++i)
  {
    l.add_transition(*i);
  }

  reachability_check(l, true); // Remove unreachable parts.
}

}
}
}
#endif // _LIBLTS_TAUSTARREDUCE_H
