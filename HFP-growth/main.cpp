#include <boost/program_options.hpp>
#include <iostream>
#include "HFPGrowth.hpp"

int main(int argc, char *argv[]) {
  using namespace gimlet::itemsets;
  try {
    HFPGrowth hfpgrowth;
    std::string inputFileName, outputFileName, statsFileName;
    double threshold;

    {
      namespace po = boost::program_options;
      po::options_description desc("Allowed options");
      desc.add_options()
	("help", "help message")
	("hmax", po::value<double>(&threshold)->required(), "relative entropy maximum threshold")
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
    hfpgrowth(threshold, inputFileName, outputFileName, statsFileName);
    return EXIT_SUCCESS;
  } catch(const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
}
