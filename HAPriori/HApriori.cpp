#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <set>
#include <cmath>

#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>
#include <gimlet/itemsets.hpp>
#include "apriori.hpp"

#define TMP_DATA_FILE "tmp.json"

using namespace gimlet;
using namespace gimlet::itemsets;

template<typename Data>
class Scorer {
  using data_type = Data;
  using format_type = Data;

  using map_type = sequence_mat_t<valued_varset_type, size_t>;
  using value_type = typename map_type::value_type;
  
  std::string fileName_;
  size_t n_;
  map_type map_;
				  
  void process(const data_type& data, const varset_type& pattern) {
    valued_varset_type values;
    auto varIt = pattern.begin(), varEnd = pattern.end();
    auto dataIt = data.begin(), dataEnd = data.end();
    
    for(; varIt != varEnd; ++varIt) {
      for(; dataIt != dataEnd; ++dataIt) {
	if(std::get<0>(*dataIt) >= *varIt) break;
      }
      if((dataIt != dataEnd) && (std::get<0>(*dataIt) == *varIt)) {
	values.push_back(*dataIt);
      } else {
      }	  
    }
    if(values.size() == pattern.size()) {
      ++map_[values];
    } else {
      map_[values] = 10E6;
    }
  }
    
  
public:
  Scorer(const std::string& fileName, size_t n) : fileName_(fileName), n_(n), map_() {}
  
  template <typename Map> void operator()(Map& patterns) {
    std::ifstream ifile(fileName_, std::ios::binary);
    static auto JSON_parser = gimlet::make_JSON_parser<gimlet::flow<format_type>>();
    auto input_stream = gimlet::make_input_data_stream(ifile, JSON_parser);
    auto begin = gimlet::make_input_data_begin(input_stream);
    auto end = gimlet::make_input_data_end(input_stream);

    map_.clear();    
    for(auto it = begin; it != end; ++it)
      for(auto& pattern : patterns) process(*it, pattern.first);

    for(auto& count : map_) {
      const valued_varset_type& data = count.first;
      varset_type varset;
      for(auto& pair : data) varset.push_back(std::get<0>(pair));      
      double p = double(count.second) / n_;
      
      patterns[varset] -= p * std::log2(p);
    }
    
  }
};

int main(int argc, char *argv[]) {
  std::istream* input = &std::cin;
  std::ifstream ifile;
  std::ostream* output = &std::cout;
  std::ofstream ofile;
  
  try {
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

      if(vm.count("input")) {
	ifile.open(inputFileName, std::ios::binary);
	input = &ifile;
      }

      if(vm.count("output")) {
	ofile.open(outputFileName, std::ios::binary);
	output = &ofile;
      }      
    }

    {
      
      std::set<attribute_type> features;
      size_t n = 0;
      double htot = 0.;
      {
	using data_type = valued_varset_type;
	using map_type = sequence_mat_t<valued_varset_type, size_t>;
	
	auto JSON_parser = make_JSON_parser<gimlet::flow<data_type>>();

	auto input_stream = make_input_data_stream(*input, JSON_parser);
	auto begin = make_input_data_begin(input_stream);
	auto end = make_input_data_end(input_stream);
	std::ofstream ofile(TMP_DATA_FILE, std::ios::binary);

	auto output_stream = make_output_data_stream(ofile, JSON_parser);
	auto out = gimlet::make_output_iterator(output_stream);
	map_type values;
	for(auto it = begin; it != end; ++it) {
	  ++n;
	  ++values[*it];
	  for(auto& pair : *it) features.insert(std::get<0>(pair));
	  *out++ = *it;
	}
	for(auto& pair : values)
	  htot -= double(pair.second) * std::log2(pair.second);
	htot = htot / n + std::log2(n);
      }
      
      auto output_JSON_parser = make_JSON_parser<gimlet::flow<std::pair<varset_type,double>>>();
      auto output_stream = make_output_data_stream(*output, output_JSON_parser);
      auto out = gimlet::make_output_iterator(output_stream);

      double hmax = threshold * htot;
      *out++  = std::pair{varset_type{}, 0.};
      
      Scorer<valued_varset_type> scorer(TMP_DATA_FILE, n);
      auto selector = [hmax, &out] (const std::pair<varset_type, double>& pattern) {
	if(pattern.second <= hmax) {
	  *out++ = pattern;
	  return true;
	} else
	  return false;
      };

      apriori<varset_type, double>(features, scorer, selector);
    }
    return EXIT_SUCCESS;
  } catch(const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
}
