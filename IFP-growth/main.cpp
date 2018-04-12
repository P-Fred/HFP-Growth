#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "IFPGrowth.hpp"

int main(int argc, char *argv[]) {
  using namespace gimlet::itemsets;
  try {
    IFPGrowth ifpgrowth;
    std::string inputFileName, outputFileName, statsFileName;
    int target;
    size_t K;
    double alpha;
    size_t nThreads = std::thread::hardware_concurrency();
    
    {
      namespace po = boost::program_options;
      po::options_description desc("Allowed options");
      desc.add_options()
	("help", "help message")
	("target", po::value<int>(&target)->required(), "target attribute (negative target starts from the end: -1 is the last attribute)")
	("K", po::value<size_t>(&K)->default_value(1), "number K of top-k patterns")
	("alpha", po::value<double>(&alpha)->default_value(1.), "branch & bound alpha relaxation coefficient")
	("threads", po::value<size_t>(&nThreads), "number of threads")
	("input", po::value<std::string>(&inputFileName), "input filename")
	("output", po::value<std::string>(&outputFileName), "output filename")
	("stats", po::value<std::string>(&statsFileName), "statistics filename");

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
      if(argc == 1 || vm.count("help")) {
	std::cout << desc << "\n";
	return EXIT_FAILURE;
      }
      po::notify(vm);
    }
    ifpgrowth(target, K, alpha, nThreads, inputFileName, outputFileName, statsFileName);
    return EXIT_SUCCESS;
  } catch(const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
}
