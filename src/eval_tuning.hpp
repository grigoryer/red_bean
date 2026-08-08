#pragma once

#include "eval_constants.hpp"
#include "search_globals.hpp"
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace eval_tuning
{

// One SPSA-tunable eval weight. The pointer aims straight at the live spsa_eval

// The search weights are not all Score, and two of them are not integers at all,
// so the pointer carries the type it was made from. UCI spin options are integer
// only: kScaled is a double tuned in hundredths, descaled on the way in.
enum class Kind : uint8_t
{
    kScore,
    kInt,
    kUint8,
    kScaled,
};

struct Param
{
    std::string name;   // as printed in the option list
    void* value;
    Kind kind;
    int def;            // captured at registry build, before any setoption
    int min;
    int max;
};

// Divisor for kScaled: an option value of 225 means 2.25 in the search.
constexpr double kScaleUnit = 100.0;

// Built once on first call. Only weights the eval actually reads are listed:
// a parameter that cannot move the score is a wasted SPSA game.
const std::vector<Param>& Params();

// One "option name ... type spin" line per tunable, for the uci response.
void PrintOptions(std::ostream& out);

// weather-factory config.json, generated from the same registry so the two
// cannot drift apart.
void PrintSpsaConfig(std::ostream& out);

// Name must already be lowercased. Returns false if it is not a tunable, so the
// caller can fall through to its own options. Values are clamped to [min, max].
bool SetParam(const std::string& lowercase_name, int value);

}  // namespace eval_tuning
