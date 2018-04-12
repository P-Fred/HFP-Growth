#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

namespace cool {
  using namespace std;

  struct Statistics {
    //enum { long_integer, double_fp } variable_type;

    struct Entry {
      const char *name_, *unit_;
      Entry(const char* name, const char* unit = nullptr);
      void fullWrite(std::ostream& os);
      virtual void write(std::ostream& os) = 0;
      virtual ~Entry();
    };

    struct IntegerEntry : Entry {
      unsigned int* value_;
      IntegerEntry(const char* name, unsigned int& variable, const char* unit = nullptr);
      virtual void write(std::ostream& os);
    };

    struct DoubleEntry : Entry {
      double* value_;
      DoubleEntry(const char* name, double& variable, const char* unit = nullptr);
      virtual void write(std::ostream& os);
    };

    struct StringEntry : Entry {
      std::string* value_;
      StringEntry(const char* name, std::string& variable, const char* unit = nullptr);
      virtual void write(std::ostream& os);
    };

    std::fstream statisticsFile_;
    std::vector<Entry*> variables_;
    bool fullPrinting_, enabled_;
    std::string testId_;

    Statistics(const std::string& testId = std::string());
    ~Statistics();

    void addInteger(const char* name, unsigned int& variable, const char* unit = nullptr);

    void addDouble(const char* name, double& variable, const char* unit = nullptr);

    void setTestId(const std::string& testId);

    void open(const char* statisticsFileName, const char* comment = nullptr);

    std::ostream& outputStream() const;

    void write() const;
  };
}
