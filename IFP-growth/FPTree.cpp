#include <atomic>

#include "FPTree.hpp"

#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>


namespace gimlet {	
  namespace itemsets {
	
    FPTree::Node::Node(Node* parent, token_type count) : parent_(parent), count_(count),
							 heir_(), partCount_() {}

    void FPTree::Node::setCount(token_type count) {
      count_ += count;
      Node* parent = parent_;
      while(parent != nullptr) {
	parent->count_ += count;
	parent = parent->parent_;
      }
    }
    
    bool FPTree::Node::isLast() const {
      return next_ == nullptr;
    }
  	
    FPTree::Level::Level() : Link(), parts_(), count_(0) {}
    FPTree::Level::Level(pair_type attr) : Link(), parts_(), attr_(attr), count_(0) {}
	
    FPTree::Level::iterator FPTree::Level::begin() { return iterator(next_); }
    FPTree::Level::iterator FPTree::Level::end() { return iterator(nullptr); }
	
    FPTree::Level::const_iterator FPTree::Level::begin() const { return const_iterator(next_); }
    FPTree::Level::const_iterator FPTree::Level::end() const { return const_iterator(nullptr); }

    void FPTree::Level::push_back(Node* n) {
      n->next_ = this->next_;
      this->next_ = n;
    }    

    bool FPTree::Level::empty() const { return next_ == nullptr; }
      
    std::ostream& operator<<(std::ostream& os, const FPTree::Level& level) {
      os << '[' << attr_to_string(level.attr_) << ", " << level.count_ << "] |";

      const FPTree::Node* master = nullptr;
      for(const FPTree::Node* node : level) {
	if(node->master_ != master) {
	  master = node->master_;
	  os << "| ";
	} else
	  os << ", ";
	os << static_cast<int>(node->count_);

	const FPTree::Node* n = node;
	os << " x (";
	bool first = true;
	while(n->level_ != nullptr) {
	  if(first) first = false;
	  else os << ' ';
	  os << attr_to_string(n->level_->attr_);
	  n = n->parent_;
	}
	os << ") ";
      }
      os << '|';
      return os;
    }

    FPTree::Group::Group(attribute_type var) : std::vector<Level*>(), var_(var), H_(0.) {}
    
    void FPTree::Group::computeEntropyFromLevels() {
      H_ = 0.;
      count_type total = 0;
      for(Level* level : *this) {
	count_type c = level->count_;
	H_ -= c * std::log2(c);
	total += c;
      }
      if(total != 0) {
	H_ /= total;
	H_ += std::log2(total);
      }
    }

    static double tlog2(size_t n) {
      thread_local std::vector<double> values;
      size_t s = values.size();
      if(n > s) {
	for(size_t i = s; i != n; )
	  values.push_back(std::log2(++i));
      }
      return values[n-1];
    }
    
    double FPTree::hyperGeometricProbLog(count_type k, count_type a, count_type b, count_type n) {
      if(a > n || b > n || (k+n < a+b) || k > a || k > b)
	return 0.;
      if(a < b) std::swap(a,b);

      double res = 0.;
      count_type p1 = a;
      count_type p2 = b;
      count_type p3 = n;
      count_type p4 = k;
      count_type p5 = n-a;

      for(count_type i = 0; i != k; ++i, --p1, --p2, --p3, --p4) 
	//res += std::log2(double(p1) / p3 * p2 / p4);
	res += tlog2(p1) - tlog2(p3) + tlog2(p2) - tlog2(p4);
      
      for(count_type i = 0; i != b-k; ++i, --p3, --p5)
	//res += std::log2(double(p5) / p3);
	res += tlog2(p5) - tlog2(p3);
      
      if(res > 1.) {
	std::cerr << "res = " << res << std::endl;
	throw std::runtime_error("Bad res");
      }
      return res;
    }
    
    double FPTree::computeInfoBias(const Group& currentGroup) const {
      std::mutex mutex;
      count_type n = size();
      double res = 0.;
      
      for(const Level* targetLevel : *targetGroup_) {
	count_type ai = targetLevel->count_;
	
	for(const Level* level : currentGroup) {
	  threads_.emplace_back([n, ai, &res, level, &mutex] () -> void {
	      const std::vector<Node*>& parts = level->parts_;
	      double total = 0.;
	      
	      for(const Node* master : parts) {
		count_type bj = master->partCount_;
		count_type m = ai+bj <= n+1 ? 1 : ai+bj-n;
		count_type M = std::min(ai, bj);
		double logh = hyperGeometricProbLog(m, ai, bj, n);

		double subtotal = 0.;
		for(count_type k = m; k <= M; ++k) {
		  double h = std::exp2(logh);
		  double term = h * k * tlog2(k);
		  subtotal += term;  
		  logh += tlog2(ai-k) - tlog2(k+1) + tlog2(bj -k) - tlog2(n - ai - bj + k + 1);
		}
		total += subtotal / n;
		
		if(ai != 0 && bj != 0) {
		  double p = double(ai) / n * bj / n;
		  total -= p * (tlog2(ai) + tlog2(bj) - tlog2(n));
		}
	      }
	      {
		 std::lock_guard<std::mutex> lock(mutex);
		 res += total;
	      }
	    });	    
	}
      }
      threads_.join();
      return res;
    }
       
    double FPTree::Group::intersect() {
      double H = 0.;
      count_type total = 0;

      for(Level* level : *this) {
	std::vector<Node*>& parts = level->parts_;
	parts.clear();
	auto it = level->begin(), end = level->end();	  
	for(; it != end; ++it) {
	  Node* node = *it;
	  Node* master = node->master_;
	  Node* heir = master->heir_;
	  if(heir == nullptr) {
	    heir = node;
	    master->heir_ = heir;
	    master->partCount_ = 0;
	    parts.push_back(master);
	  }
	  master->partCount_ += node->count_;
	  node->master_ = heir;
	}
	
	for(Node* master : parts) {
	  count_type c = master->partCount_;
	  H -= c * std::log2(c);
	  total += c;
	  master->heir_ = nullptr;
	}
      }

      if(total != 0) {
	H /= total;
	H += std::log2(total);
      }
      return H;
    }

    std::ostream& operator<<(std::ostream& os, const FPTree::Group& group) {
      os << 'V' << group.var_ << " (H = " << group.H_ << "):" << std::endl;
      for(const auto& level : group) os << "  " << *level << std::endl;
      os << std::endl;
      return os;
    }

    void FPTree::skip(Group& group) {
      for(Level* level : group) {
	threads_.emplace_back([level]() {
	    auto it = level->begin(), end = level->end();	  
	    for(; it != end; ++it)
	      it->master_ = it->parent_->master_;
	  });
      }
      threads_.join();
    }
    
    FPTree::FPTree(int target, size_t nThreads) :
      threads_(nThreads), levels_(), groups_(),
      pool_(new boost::object_pool<Node>()),
      size_(0), nbrNodes_(0),
      root_(nullptr, 0),
      targetEntropy_(0.), targetGroup_(),
      target_(target) {
      root_.master_ = &root_;
      root_.level_ = nullptr;
    }

    size_t FPTree::size() const {
      return size_;
    }
    
    size_t FPTree::nbrNodes() const {
      return nbrNodes_;
    }

    size_t FPTree::nVars() const {
      return groups_.size();
    }    

    class FPTree::Iterator {
      using pattern_type = std::vector<pair_type>;
    public:
      using value_type = const std::pair<pattern_type, count_type>;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;
      using iterator_category = std::input_iterator_tag;
    private:
      std::map<pair_type, Level>::const_iterator level_, endLevel_;
      const Node* node_;
      std::pair<pattern_type, count_type> value_;

      const Node* updateLevel();
      void fillValue();
	
    public:
      Iterator();
      Iterator(const std::map<pair_type, Level>::const_iterator& begin, const std::map<pair_type, Level>::const_iterator& end);
      Iterator(const Iterator&) = default;

      value_type& operator*();
      value_type* operator->();
      bool operator!=(const Iterator& other) const;
      Iterator& operator++();
    };
    
    FPTree::const_iterator FPTree::begin() const { return Iterator(levels_.begin(), levels_.end()); }
    FPTree::const_iterator FPTree::end() const { return Iterator(); }

    void FPTree::Iterator::fillValue() {
      pattern_type& pattern = value_.first;
      if(pattern.empty()) {
	value_.second = node_->count_;
	const Node* node = node_;
	while(node->parent_ != nullptr) {
	  node = node->parent_;
	}
      }
    }
    
    FPTree::Iterator::Iterator() : node_(nullptr) {}

    FPTree::Iterator::Iterator(const std::map<pair_type, Level>::const_iterator& level, const std::map<pair_type, Level>::const_iterator& endLevel) : level_(level), endLevel_(endLevel), value_() {
      node_ = updateLevel();
    }

    FPTree::Iterator::value_type& FPTree::Iterator::operator*() {
      fillValue();
      return value_; 
    }
	
    FPTree::Iterator::value_type* FPTree::Iterator::operator->() {
      fillValue();
      return &value_;
    }
	
    bool FPTree::Iterator::operator!=(const Iterator& other) const {
      return node_ != other.node_;
    }

    const FPTree::Node* FPTree::Iterator::updateLevel() {
      for(; level_ != endLevel_; ++level_)
	if(! level_->second.empty())
	  return static_cast<const Node*>(level_->second.next_);
      return nullptr;
    }

    FPTree::Iterator& FPTree::Iterator::operator++() {
      value_.first.clear();
      do {
	if(node_->isLast()) {
	  ++level_;
	  node_ = updateLevel();
	} else
	  node_ = static_cast<const Node*>(node_->next_);
      } while(node_ != nullptr && node_->count_ == 0);
      return *this;
    }
        
    FPTree::Group& FPTree::group(attribute_type attr) {
      auto res = groups_.emplace(attr,Group(attr));
      Group& group = (res.first)->second;
      if(res.second)
	sortedGroups_.push_back(&group);
      return group;
    }

    FPTree::Level& FPTree::level(const pair_type& attr) {
      auto res = levels_.emplace(attr,Level(attr));
      Level& level = (res.first)->second;
      if(res.second) {
	Group& g = group(attr.first);
	g.push_back(&level);
      }
      return level;
    }

    void FPTree::internalState(std::ostream& os) {
      for(auto& g : sortedGroups_)
	os << *g;
    }
    
    FPTree::Node* FPTree::addNode(const pair_type& attr, Node* parent) {
      Level& lvl = this->level(attr);
      Node* node = pool_->construct(parent, 0);
      ++nbrNodes_;
      node->master_ = parent->master_;
      node->level_ = &lvl;      
      lvl.push_back(node);
      return node;
    }

    void FPTree::build(std::vector<pattern_type>& data) {
      auto difference = [] (const pattern_type& p1, const pattern_type &p2) -> int {
	auto b1 = p1.begin(), e1 = p1.end();
	auto b2 = p2.begin(), e2 = p2.end();
	do {
	  if(b1 == e1) return static_cast<int>(b2 - e2);
	  if(b2 == e2) return static_cast<int>(e1 - b1);
	  if(*b1 == *b2) {
	    ++b1; ++b2;
	  } else {
	    return static_cast<int>(e1 - b1);
	  }
	} while(true);
      };
      

      std::vector<const pattern_type*> dataRefs;

      {
	// Store the data pointers and record attributes to compute entropy of variables
	attribute_type maxAttr = 0;
	for(const pattern_type& pattern : data) {
	  dataRefs.push_back(&pattern);
	  attribute_type maxPatternAttr = record(pattern.begin(), pattern.end());
	  if(maxPatternAttr > maxAttr) maxAttr = maxPatternAttr;
	}
	if(target_ < 0) target_ = maxAttr + 1 + target_;
	if(target_ < 0 || target_ > maxAttr)
	  throw std::runtime_error(std::string("out of range target ") + std::to_string(target_));
	  
	// Compute the entropy of every variable
	auto begin =  sortedGroups_.begin(), end = sortedGroups_.end();
	auto targetIt = begin;
	for(auto it = begin; it != end; ++it) {

	  Group* group = *it;
	  group->computeEntropyFromLevels();
	  if(group->var_ == target_) {
	    targetEntropy_ = group->H_;
	    targetGroup_ = group;
	    targetIt = it;
	  }
	}

	if(! targetGroup_)
	  throw std::runtime_error("Unknown target variable");
	
	std::swap(*targetIt, *(--end));

	// Sort groups by increasing order of entropy
	// excluding the target group
	std::sort(begin ,end,
		  [](const Group* g1, const Group* g2) {
		    return g1->H_ < g2->H_;
		  });
      }
      {

	int groupIndex = 0;
	for(Group* group : sortedGroups_)
	  group->index_ = groupIndex++;
      }
      
      {	
	for(pattern_type& pattern : data) {
	  for(auto& attr : pattern) attr.first = group(attr.first).index_;
	  std::sort(pattern.begin(), pattern.end());
	}

	std::sort(dataRefs.begin(), dataRefs.end(),
		  [](const pattern_type* p1, const pattern_type* p2) {
		    return std::lexicographical_compare(p1->begin(), p1->end(), p2->begin(), p2->end());
		  });

	for(pattern_type& pattern : data)
	  for(auto& attr : pattern) attr.first = sortedGroups_[attr.first]->var_;
      }
      
      pattern_type root;
      const pattern_type* pred = &root;
      count_type count = 0;
      Node* node = &root_;
      
      for(const pattern_type* pattern : dataRefs) {
	int diff = -difference(*pred, *pattern);
	if(diff == 0) {
	  ++count;
	} else {
	  node->setCount(count);
	  size_ += count;
	  count = 1;
	  
	  if(diff < 0) {
	    for(int i = diff; i != 0; ++i) {
	      node = node->parent_;
	    }
	  } else {
	    diff = 0;
	  }
	  auto attr = pattern->begin() + (pred->size() + diff), end = pattern->end();
	  for(; attr != end; ++attr) {
	    node = addNode(*attr, node);
	  }
	}
	pred = pattern;
      }
      if(count != 0) {
	node->setCount(count);
	size_ += count;
      }
    }

    double FPTree::targetEntropy() const { return targetEntropy_; }

    void FPTree::build(std::istream& is) {
      auto JSON_parser = gimlet::make_JSON_parser<flow<pattern_type>>();
      auto input_stream = gimlet::make_input_data_stream(is, JSON_parser);
      auto begin = gimlet::make_input_data_begin<decltype(input_stream), pattern_type>(input_stream);
      auto end = gimlet::make_input_data_end<decltype(input_stream), pattern_type>(input_stream);
      
      build(begin, end);
    }
  }
}
