#include <iostream>
#include <fstream>
#include "gimlet/timer.hpp"
#include "HFPGrowth.hpp"

#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>

namespace gimlet {
  namespace itemsets {

    //    using count_type = FPTree::count_type;
    using pair_type = FPTree::pair_type;
    
    class HFPGrowth::PatternProcessor {
      using pattern_type = std::vector<attribute_type>;
      using output_format = tuple<list<attribute_type>, double>;
      using parser_t = JSONParser<flow<output_format>>;
      using stream_t = output_stream_t<parser_t>;
	
      stream_t outputDataStream_;
      output_stream_iterator_t<stream_t> outputIt_;

      pattern_type pattern_;
      int backSymbols_, forwardSymbols_;
      
      Stats& stats_;
      
    public:
      PatternProcessor(std::ostream& outputStream, Stats& stats) :
	outputDataStream_{outputStream, parser_t{}},	
	outputIt_{outputDataStream_},
	stats_(stats) {}

      void emit(double H) {
	*outputIt_++ = std::make_pair(pattern_, H);
	++stats_.nPatterns_;
      }

      void push(attribute_type var) {
	pattern_.push_back(var);
      }

      void pop() {
	pattern_.pop_back();
	++backSymbols_;
      } 
    };
    
    void HFPGrowth::operator()(
			       double threshold,
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
      
      stats_.relativeMaxEntropy_ = threshold;
      if(! statsFileName.empty())
	stats_.open(statsFileName.c_str(), "%");
      
      cool::Timer timer;
      timer.start();

      FPTree tree = FPTree::build(inputStream);
      
      double absoluteMaxEntropy = tree.totalEntropy() * threshold;	
      auto selector = [threshold = absoluteMaxEntropy](double value) {
	return value <= threshold;
      };

      PatternProcessor processor{outputStream, stats_};	
      tree.generate(processor, selector);
      outputFile.close();
      
      stats_.totalTime_ = timer.stop();
      stats_.write();
    }

    HFPGrowth::HFPGrowth() : stats_{} {}
  }
}
