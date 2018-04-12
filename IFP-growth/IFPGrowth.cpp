#include <iostream>
#include <fstream>
#include <limits>
#include <map>
#include <algorithm>
#include "gimlet/timer.hpp"
#include "IFPGrowth.hpp"

#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>

namespace gimlet {
  namespace itemsets {

    //    using count_type = FPTree::count_type;
    using pair_type = FPTree::pair_type;
    
    class IFPGrowth::PatternProcessor {
      using pattern_type = std::vector<attribute_type>;
      using output_format = tuple<list<attribute_type>, double>;
      using parser_t = JSONParser<flow<output_format>>;
      using stream_t = output_stream_t<parser_t>;
	
      size_t K_;
      stream_t outputDataStream_;
      output_stream_iterator_t<stream_t> outputIt_;
      pattern_type pattern_;      
      Stats& stats_;
      std::multimap<double, pattern_type> queue_;
      double worstTopKScore_;
      
    public:
      PatternProcessor(size_t K, std::ostream& outputStream, Stats& stats) :
	K_(K),
	outputDataStream_{outputStream, parser_t{}},	
	outputIt_{outputDataStream_},
	stats_(stats),
	queue_(),
	worstTopKScore_(-std::numeric_limits<double>::max()) {}

      ~PatternProcessor() {
	pattern_type pattern;
	for(auto it = queue_.rbegin(); it != queue_.rend(); ++it) {
	  pattern = it->second;
	  std::sort(pattern.begin(), pattern.end());
	  *outputIt_++ = std::make_pair(pattern, it->first);
	}
      }
      
      double worstTopKScore() { return worstTopKScore_; }
      
      void emit(double score) {
	bool toInsert = queue_.size() < K_;
	if(! toInsert) {
	  toInsert = worstTopKScore_ < score;
	  if(toInsert)
	    queue_.erase(queue_.begin());
	}
	if(toInsert) {
	  queue_.insert(std::make_pair(score, pattern_));
	  // outputDataStream_ << std::flush;
	  auto worstTopK = queue_.begin();
	  worstTopKScore_ = worstTopK->first;
	}
	++stats_.nPatterns_;
      }

      void push(attribute_type var) {
	pattern_.push_back(var);
      }

      void pop() {
	pattern_.pop_back();
      } 
    };
    
    void IFPGrowth::operator()(
			       int target,
			       size_t K,
			       double alpha,
			       size_t nThreads,
			       const std::string& inputFileName,
			       const std::string& outputFileName,
			       const std::string& statsFileName
			       ) {
      auto inputStream = std::ref(std::cin);
      std::ifstream inputFile;
      if(! inputFileName.empty()) {
	inputFile.open(inputFileName, std::ios::in | std::ios::binary);
	inputStream = inputFile;
      }
      auto outputStream = std::ref(std::cout);
      std::ofstream outputFile;
      if(! outputFileName.empty()) {
	outputFile.open(outputFileName, std::ios::out | std::ios::binary);
	outputStream = outputFile;
      }
      
      stats_.alpha_ = alpha;
      if(! statsFileName.empty())
	stats_.open(statsFileName.c_str(), "%");
      
      cool::Timer timer;
      timer.start();

      FPTree tree(target, nThreads);
      tree.build(inputStream);
      
      // tree.internalState(std::clog);

      PatternProcessor processor{K, outputStream, stats_};	
	
      auto selector = [&processor, &alpha](double value) {
	bool select = value > processor.worstTopKScore() / alpha;
	// if(! select) std::cerr << "prune" << std::endl;
	return select;
      };

      tree.generate(processor, selector);
      
      stats_.totalTime_ = timer.stop();
      stats_.write();
    }

    IFPGrowth::IFPGrowth() : stats_{} {}
  }
}
