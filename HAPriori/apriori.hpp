#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>

#include <gimlet/itemsets.hpp>

namespace gimlet {	
  namespace itemsets {
    template<typename Itemset>
    struct Compare {
      bool operator()(const Itemset& v1, const Itemset& v2) const {
	return std::lexicographical_compare(v1.begin(), v1.end(), v2.begin(), v2.end());
      }
    };

    template<typename Itemset, typename Value>
    using sequence_mat_t =  std::map<Itemset, Value, Compare<Itemset>>;
    template<typename Itemset>
    using sequence_set_t =  std::set<Itemset, Compare<Itemset>>;
    
    template<typename Itemset> struct Level {
      using itemset_type = Itemset;

      using set_t = sequence_set_t<Itemset>;
      using iterator = typename set_t::iterator;
      using const_iterator = typename set_t::const_iterator;

      set_t level_;

      class Candidates {
	class Iterator {
	  iterator it1_, it2_, end_;
	  const set_t& level_;
	  itemset_type value_;
	
	  void next() {
	    while(it1_ != end_) {
	      const itemset_type& previous = *it1_;
	      for(++it2_;it2_ != end_; ++it2_) {
		const itemset_type& itemset = *it2_;
		if(std::equal(itemset.begin(), --itemset.end(), previous.begin(), --previous.end())) {
		  value_ = previous;
		  value_.push_back(*(--itemset.end()));
		  auto preds = make_predecessors(value_);
		  bool found = true;
		  for(const auto& pred : preds) {
		    itemset_type pred2;
		    for(auto& item : pred) pred2.push_back(item);
		    if(level_.find(pred2) == level_.end()) {
		      found = false;
		      break;
		    }
		  }
		  if(found) return;
		}
	      }
	      ++it1_;
	      it2_ = it1_;
	    }
	  }
	
	public:
	  Iterator(const iterator& it, const iterator& end, const set_t& level) : it1_(it), it2_(it), end_(end), level_(level) {
	    next();
	  }
	  Iterator(const Iterator&) = default;

	  bool operator!=(const Iterator& other) const {
	    return it1_ != other.it1_ || it2_ != other.it2_;
	  }

	  const itemset_type& operator*() {
	    return value_;
	  }
	  
	  Iterator& operator++() {
	    next();
	    return *this;
	  }
	};

	set_t& level_;
      public:
	Candidates(set_t& level) : level_(level) {}
	Iterator begin() const { return Iterator(level_.begin(), level_.end(), level_); }
	Iterator end() const { return Iterator(level_.end(), level_.end(), level_); }
      };
      Candidates candidates_;
      
      Level() : level_(), candidates_(level_) {}

      bool empty() { return level_.empty(); }
      void clear() { level_.clear(); }
      
      void add(const Itemset& itemset) {	
	level_.insert(itemset);
      }

      iterator begin() { return level_.begin(); }
      iterator end() { return level_.end(); }
      const_iterator begin() const { return level_.begin(); }
      const_iterator end() const { return level_.end(); }
      
      Candidates& candidates() {
	return candidates_;
      }
    };

    template<typename Itemset, typename Score, typename Attributes, typename Scorer, typename Selector> void apriori(const Attributes& attributes, Scorer& scorer, const Selector& selector) {

      using level_t = Level<Itemset>;
      using map_t = sequence_mat_t<Itemset, Score>;
      level_t patterns;
      map_t candidates;
      for(auto& attr : attributes)
	candidates.insert(std::pair{Itemset({attr}), Score()});
      
      while(! candidates.empty()) {
	scorer(candidates);
	patterns.clear();
	for(auto& candidate : candidates) {
	  if(selector(candidate))
	    patterns.add(candidate.first);
	}
	candidates.clear();
	for(auto& candidate : patterns.candidates())
	  candidates.insert(std::pair{candidate, Score()});
      }
    }
  }
}
