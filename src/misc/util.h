// Author:  Joel Young <jdyoung@nps.edu>
// Modified: Bruce Allen <bdallen@nps.edu>
#ifndef   UTIL_H
#define   UTIL_H

// Standard includes
#include <sstream>
#include <iostream>

// TR1 includes:
#include <tr1/random>
#include <tr1/cmath>     // log2

// Function object to generate uniformly an unsigned integer in a
// closed interval
class randomgenerator {
  private:
    std::tr1::minstd_rand           generator;
    std::tr1::uniform_int<uint64_t> distribution;

  public:
    randomgenerator(uint64_t s, uint64_t e) : distribution(s,e) {
      generator.seed(0);
    }

    uint64_t operator()() { 
      return distribution(generator);
    }
};

#endif
