#include<limits>

#include "gimlet/stringstream.hpp"

namespace cool {
  
  std::istream & operator>> (std::istream & stream, const skip & x) {
    std::ios_base::fmtflags f = stream.flags();
    stream >> std::noskipws;

    char c;
    const char* text = x.text;
    for(; stream && *text && *text != '\n'; ++text) {
      if(*text == ' ') {
	stream >> std::skipws;
	stream >> c;
	stream.putback(c);
	stream >> std::noskipws;
      } else {
	stream >> c;
	if(c != *text) {
	  stream.setstate(std::ios_base::failbit);
	  break;
	}
      }
    }
    stream.flags(f);
    return stream;
  }

  double_istream& double_istream::parse_on_fail (double &x, bool neg) {
    const char *exp[] = { "", "inf", "NaN" };
    const char *e = exp[0];
    int l = 0;
    char inf[4];
    char *c = inf;
    if (neg) *c++ = '-';
    in_.clear();
    if (!(in_ >> *c).good()) return *this;
    switch (*c) {
    case 'i': e = exp[l=1]; break;
    case 'N': e = exp[l=2]; break;
    }
    while (*c == *e) {
      if ((e-exp[l]) == 2) break;
      ++e; if (!(in_ >> *++c).good()) break;
    }
    if (in_.good() && *c == *e) {
      switch (l) {
      case 1: x = std::numeric_limits<double>::infinity(); break;
      case 2: x = std::numeric_limits<double>::quiet_NaN(); break;
      }
      if (neg) x = -x;
      return *this;
    } else if (!in_.good()) {
      if (!in_.fail()) return *this;
      in_.clear(); --c;
    }
    do { in_.putback(*c); } while (c-- != inf);
    in_.setstate(std::ios_base::failbit);
    return *this;
  }

  double_istream::double_istream(std::istream &in) : in_(in) {}
  
  double_istream& double_istream::operator>> (double &x) {
    bool neg = false;
    char c;
    if (!in_.good()) return *this;
    while (isspace(c = in_.peek())) in_.get();
    if (c == '-') { neg = true; }
    in_ >> x;
    if (! in_.fail()) return *this;
    return parse_on_fail(x, neg);
  }

  const double_imanip& double_imanip::operator>>(double &x) const {
    double_istream(*in) >> x;
    return *this;
  }
  std::istream& double_imanip::operator>>(const double_imanip &) const {
    return *in;
  }

  const double_imanip&  operator>>(std::istream &in, const double_imanip &dm) {
    dm.in = &in;
    return dm;
  }
  
}
