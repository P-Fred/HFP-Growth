#pragma once

#include "gimlet/statistics.hpp"
#include "FPTree.hpp"

namespace gimlet {
  namespace itemsets {
    class HFPGrowth {
      class PatternProcessor;
      
      struct Stats : cool::Statistics {
	unsigned int nPatterns_;
	double totalTime_;
	double relativeMaxEntropy_;

	Stats() : Statistics() {
	  addDouble("threshold", relativeMaxEntropy_);
	  addDouble("total time", totalTime_, "s");
	  addInteger("patterns", nPatterns_);
	}
      };
      
      Stats stats_;
      
    public:
      void operator()(
	      double threshold,
	      const std::string& inputFileName,
	      const std::string& outputFileName,
	      const std::string& statsFileName);

      HFPGrowth();
    };
  }
}
