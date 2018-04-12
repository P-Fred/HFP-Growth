#include "gimlet/statistics.hpp"

namespace cool {

  Statistics::Entry::Entry(const char* name, const char* unit) : name_(name), unit_(unit) {}
  void Statistics::Entry::fullWrite(std::ostream& os) {
    os << name_ << " = ";
    write(os);
    if(unit_) os << ' ' << unit_;
  }
  
  Statistics::Entry::~Entry() {}

  Statistics::IntegerEntry::IntegerEntry(const char* name, unsigned int& variable, const char* unit) : Entry(name, unit), value_(&variable) {
    variable = 0;
  }
  void Statistics::IntegerEntry::write(std::ostream& os) {
    os << *value_;
  }

  Statistics::DoubleEntry::DoubleEntry(const char* name, double& variable, const char* unit) : Entry(name, unit), value_(&variable) {
    variable = 0.;
  }
  
  void Statistics::DoubleEntry::write(std::ostream& os) {
    os << *value_;
  }

  Statistics::StringEntry::StringEntry(const char* name, std::string& variable, const char* unit) : Entry(name, unit), value_(&variable) {
  }
  void Statistics::StringEntry::write(std::ostream& os) {
    os << *value_;
  }
  
  Statistics::Statistics(const std::string& testId) : statisticsFile_(), variables_(), fullPrinting_(true), enabled_(false), testId_(testId) {} 

  void Statistics::addInteger(const char* name, unsigned int& variable, const char* unit) {
    variables_.push_back(new IntegerEntry(name, variable, unit));
  }

  void Statistics::addDouble(const char* name, double& variable, const char* unit) {
    variables_.push_back(new DoubleEntry(name, variable, unit));
  }

  void Statistics::setTestId(const std::string& testId) { testId_ = testId; }

  void Statistics::open(const char* statisticsFileName, const char* comment) {
    enabled_ = true;
    ostringstream header;
    bool writeHeader = (comment != 0);
    if(writeHeader) {
      std::string line;
      header << comment;
      if(! testId_.empty()) header << " id,";
      bool first = true;
      for(Entry* entry : variables_) {
	if(first) first = false; else header << ',';
	header << ' ' << entry->name_;
      }
      std::ifstream ifs(statisticsFileName);
      if(ifs.good()) {
	getline(ifs,line);
	if(ifs.good()) {
	  if(line != header.str()) throw std::runtime_error("Statistics file header mismatch");
	  else writeHeader = false;
	}
      }
      ifs.close();
    }

    statisticsFile_.open(statisticsFileName, ios::out | ios::app);
    if(! statisticsFile_.good()) throw std::runtime_error("Cannot open statistics file");
    fullPrinting_ = false;
    if(writeHeader) outputStream() << header.str() << std::endl;
  }

  std::ostream& Statistics::outputStream() const {
    Statistics* stats = const_cast<Statistics*>(this);
    if(statisticsFile_.is_open()) return stats->statisticsFile_;
    return std::cerr;
  }

  void Statistics::write() const {
    if(enabled_) {
      std::ostream& os = outputStream();
      os << '\n';
      if(! testId_.empty()) os << '\r' <<  testId_ << ' ';    
      for(Entry* entry : variables_)
	if(fullPrinting_) {
	  os << '\r'; entry->fullWrite(os); os << std::endl;
	} else {
	  entry->write(os); os << ' ';
	}
      if(! fullPrinting_) os << std::endl;
    }
  }

  Statistics::~Statistics() {
    for(Entry* entry : variables_)
      delete entry;
    if(statisticsFile_.is_open()) statisticsFile_.close();
  }

}
