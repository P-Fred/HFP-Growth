#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
#include <boost/pool/object_pool.hpp>
#include "gimlet/thread_pool.hpp"

#include <gimlet/itemsets.hpp>

namespace gimlet {	
  namespace itemsets {
  
    template<typename Item>
    std::enable_if_t<std::is_arithmetic_v<Item>, std::string>
    attr_to_string(const Item& attr) {
      return std::to_string(attr);
    }
    
    template<typename Variable, typename Value>
    std::string attr_to_string(const std::pair<Variable, Value>& attr) {
      return std::string("(") + attr_to_string(attr.first) + "," + attr_to_string(attr.second) + ")";
    }
    
    class FPTree {

      using token_type = unsigned short;

    public:

      using count_type = unsigned long;
      using pair_type = std::pair<attribute_type, attribute_value_type>;     
      using pattern_type = std::vector<pair_type>;
      
      struct Link {
	Link *next_;

	Link() : next_() {}
	Link(const Link&) = default;	
      };
      
      struct Level;
      
      struct Node : Link {
	Node* parent_;
	token_type count_;
	Node* master_;
	Level* level_;

	Node* heir_;
	count_type partCount_;
	
	Node(Node* parent, token_type count);
	Node(const Node&) = default;
	
	bool isLast() const;

	void setCount(token_type count);
      };

      struct Level : Link {
	std::vector<Node*> parts_;
	pair_type attr_;
	count_type count_;
	
	Level();
	Level(pair_type attr);
	Level(const Level&) = default;

	template<typename LINK, typename NODE>
	class Iterator {
	  LINK* link_;
	public:
	  Iterator(LINK* link) : link_(link) {}
	  Iterator(const Iterator&) = default;

	  void operator++() {
	    link_ = link_->next_;
	  }

	  bool operator!=(const Iterator& other) const {
	    return link_ != other.link_;
	  }
	  
	  bool operator==(const Iterator& other) const {
	    return link_ == other.link_;
	  }

	  NODE* operator*() {
	    return static_cast<NODE*>(link_);
	  }
	  
	  NODE* operator->() {
	    return static_cast<NODE*>(link_);
	  }

	};
            
	using iterator = Iterator<Link, Node>;
	using const_iterator = Iterator<const Link, const Node>;
	
	iterator begin();
	iterator end();
	
	const_iterator begin() const;
	const_iterator end() const;

	void push_back(Node* n);
	bool empty() const;
      };

      struct Group : std::vector<Level*> {
	attribute_type var_;
	double H_;
	attribute_type index_;
	
	Group(attribute_type var);

	void computeEntropyFromLevels();
	double intersect();
      };

      void skip(Group&);
      static double hyperGeometricProbLog(count_type k, count_type a, count_type b, count_type n);
      double computeInfoBias(const Group& currentGroup) const;
      
      mutable cool::ThreadPool threads_;
      std::map<pair_type, Level> levels_;
      std::map<attribute_type, Group> groups_;
      std::vector<Group*> sortedGroups_;
      
      std::unique_ptr<boost::object_pool<Node>> pool_;
      size_t size_, nbrNodes_;
      Node root_;
      double targetEntropy_;
      Group* targetGroup_;
      int target_;
      
      Group& group(attribute_type attr);
      Level& level(const pair_type& attr);
      Node* addNode(const pair_type& attr, Node* parent);

      template<typename Processor, typename Selector>
      class PatternGenerator;
      
      class Iterator;
            
      template<typename Iterator>
      attribute_type record(const Iterator& begin, const Iterator& end) {
	attribute_type maxAttr = 0;
	for(Iterator it = begin; it != end; ++it) {
	  const pair_type& attr = *it;
	  if(maxAttr < attr.first) maxAttr = attr.first;
	  Level& lvl = level(attr);
	  ++lvl.count_;
	}
	return maxAttr;
      }

      void build(std::vector<pattern_type>& data);

    public:
      FPTree(int target, size_t nThreads);
      FPTree(const FPTree&) = delete;
      FPTree(FPTree&&) = default;

      template<typename DataIterator>
      void build(DataIterator begin, DataIterator end) {
	std::vector<pattern_type> data;
	for(auto it = begin; it != end; ++it) {
	  data.push_back(*it);
	}
	build(data);
      }
      
      void build(std::istream&);
      size_t size() const;
      size_t nbrNodes() const;
      size_t nVars() const;
      double targetEntropy() const;
      void internalState(std::ostream& os);

      using const_iterator = Iterator;
      const_iterator begin() const;
      const_iterator end() const;

      friend std::ostream& operator<<(std::ostream&, const FPTree::Level&);
      friend std::ostream& operator<<(std::ostream&, const FPTree::Group&);
      friend std::ostream& operator<<(std::ostream&, const FPTree&);     

      template<typename Processor, typename Selector>
      void generate(Processor& processor, const Selector& selector);
    };

    template<typename Processor, typename Selector>
    class FPTree::PatternGenerator {
      FPTree& tree_;
      Processor& processor_;
      Selector selector_;
      const Group* targetGroup_;
      count_type n_;
      double HY_, HX_, bias_;

      void develop(size_t varIndex) {
	Group& group = *tree_.sortedGroups_[varIndex];
	//tree_.internalState(std::cerr);
	++varIndex;

	tree_.skip(group);
	if(varIndex != tree_.nVars()) {
	  develop(varIndex);
	  HX_ = group.intersect();
	  bias_ = tree_.computeInfoBias(group) / HY_;
	  double upperBound = 1. - bias_;
	  if(selector_(upperBound)) {
	    processor_.push(group.var_);
	    develop(varIndex);
	    processor_.pop();
	  }
	} else {
	  double HXY = group.intersect();
	  double MIXY = 1. - (HXY - HX_) / HY_;
	  double reliableFractionOfMutualInfo = MIXY - bias_;
	  if(selector_(reliableFractionOfMutualInfo)) {
	    processor_.emit(reliableFractionOfMutualInfo);
	  }
	}
      }

    public:
      PatternGenerator(FPTree& tree, Processor& processor, const Selector& selector) :
	tree_(tree),
	processor_(processor),
	selector_(selector),
	targetGroup_(tree.targetGroup_), n_(tree_.size()),
	HY_(tree.targetEntropy()), HX_(), bias_() {}
      void generate() {
	develop(0);
      }
    };

    template<typename Processor, typename Selector>
    void FPTree::generate(Processor& processor, const Selector& selector) {
      PatternGenerator<Processor, Selector> generator{*this, processor, selector};
      generator.generate();
    }    
  }
}
