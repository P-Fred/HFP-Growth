#pragma once

#include "gimlet/statistics.hpp"
#include "FPTree.hpp"

namespace gimlet {
  namespace itemsets {
    class IFPGrowth {
      class PatternProcessor;
      
      struct Stats : cool::Statistics {
	unsigned int target_;
	double alpha_;
	unsigned int nPatterns_;
	double totalTime_;

	Stats() : Statistics() {
	  addInteger("target", target_);
	  addDouble("alpha", alpha_);
	  addDouble("total time", totalTime_, "s");
	  addInteger("patterns", nPatterns_);
	}
      };
      
      Stats stats_;
      
    public:
      void operator()(int target,
		      size_t K,
		      double alpha,
		      size_t nThreads,
		      const std::string& inputFileName,
		      const std::string& outputFileName,
		      const std::string& statsFileName);

      IFPGrowth();
    };
  }
}
