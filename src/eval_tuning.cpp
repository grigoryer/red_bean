#include "eval_tuning.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ostream>
#include <unordered_map>

namespace eval_tuning
{
namespace
{

using namespace spsa_eval;

constexpr size_t kMg = static_cast<size_t>(Phase::kMg);
constexpr size_t kEg = static_cast<size_t>(Phase::kEg);

// Pawns can only stand on ranks 2-7, so index 0 and 7 of every per-rank table
// are unreachable and stay out of the registry.
constexpr size_t kFirstPawnRank = 1;
constexpr size_t kLastPawnRank = 6;

// Bands are deliberately loose: SPSA needs room to walk, and a parameter that
// sits pinned against its bound learns nothing. The floor keeps weights that
// currently sit at 0 from getting a zero-width range.
void Add(std::vector<Param>& out, std::string name, Score& value, int floor = 15)
{
    const int def = value;
    const int half = std::max(floor, 2 * std::abs(def));
    out.push_back({std::move(name), &value, Kind::kScore, def, def - half, def + half});
}

// Search weights get their bounds by hand. A derived band is wrong for them: a
// depth cap has no meaning below 1, and the two ratios have a useful range that
// has nothing to do with how far they sit from zero.
template <typename T>
void AddBounded(std::vector<Param>& out, std::string name, T& value, Kind kind, int min, int max)
{
    const int def = (kind == Kind::kScaled)
        ? static_cast<int>(std::lround(static_cast<double>(value) * kScaleUnit))
        : static_cast<int>(value);

    out.push_back({std::move(name), &value, kind, def, min, max});
}

// Material is anchored on the pawn, so the pieces move by a fraction of
// themselves rather than by the flat band the small weights use.
void AddMaterial(std::vector<Param>& out, std::string name, Score& value, int percent)
{
    const int def = value;
    const int half = (std::abs(def) * percent) / 100;
    out.push_back({std::move(name), &value, Kind::kScore, def, def - half, def + half});
}

void AddPhasePair(std::vector<Param>& out, const std::string& prefix, std::array<Score, 2>& pair)
{
    Add(out, prefix + "MG", pair[kMg]);
    Add(out, prefix + "EG", pair[kEg]);
}

template <size_t kSize>
void AddPhaseTable(std::vector<Param>& out, const std::string& prefix,
                   std::array<std::array<Score, kSize>, 2>& table, size_t first, size_t last)
{
    for (size_t idx = first; idx <= last; ++idx)
    {
        Add(out, prefix + "MG" + std::to_string(idx), table[kMg][idx]);
    }
    for (size_t idx = first; idx <= last; ++idx)
    {
        Add(out, prefix + "EG" + std::to_string(idx), table[kEg][idx]);
    }
}

// Search heuristics. LMRBase and LMRDivisor are hundredths: 225 is the 2.25
// divisor, so a tuned result has to be divided by 100 before it goes back into
// search_globals.hpp.
void AddSearch(std::vector<Param>& out)
{
    using namespace search_spsa;

    // Bounds are held to the range these heuristics take in practice. Tuning runs
    // at a fast control, where over-pruning scores well because the search is never
    // deep enough to be punished for it, so the aggressive end of each range is the
    // one to keep closed: low divisors, low margins, high depth caps.
    // NMPDepthDivisor is an integer divisor -- its min must stay >= 1 either way or
    // the search divides by zero.
    AddBounded(out, "NMPBase", kNMP_Base, Kind::kInt, 2, 5);
    AddBounded(out, "NMPDepthDivisor", kNMP_DepthDivisor, Kind::kInt, 3, 6);
    AddBounded(out, "NMPMinDepth", kNMP_MinDepth, Kind::kUint8, 2, 6);

    AddBounded(out, "LMRBase", kLMR_Base, Kind::kScaled, 25, 155);
    AddBounded(out, "LMRDivisor", kLMR_Divisor, Kind::kScaled, 50, 350);
    AddBounded(out, "LMRHistReduction", kLMR_HistReduction, Kind::kInt, 0, 4);

    AddBounded(out, "LMPMaxDepth", kLMP_MaxDepth, Kind::kUint8, 3, 8);
    AddBounded(out, "LMPBase", kLMP_Base, Kind::kInt, 2, 6);

    AddBounded(out, "RFPMaxDepth", kRFP_MaxDepth, Kind::kUint8, 4, 8);
    AddBounded(out, "RFPMargin", kRFP_Margin, Kind::kScore, 50, 170);

    AddBounded(out, "FutilityMargin", kFutilityMargin, Kind::kScore, 60, 180);
    AddBounded(out, "FutilityMaxDepth", kFutilityMaxDepth, Kind::kUint8, 2, 5);
}

std::vector<Param> Build()
{
    constexpr auto kQueen = static_cast<size_t>(Piece::kQueen);
    constexpr auto kRook = static_cast<size_t>(Piece::kRook);
    constexpr auto kBishop = static_cast<size_t>(Piece::kBishop);
    constexpr auto kKnight = static_cast<size_t>(Piece::kKnight);
    constexpr auto kKing = static_cast<size_t>(Piece::kKing);

    std::vector<Param> out;
    out.reserve(96);

    // Material. The pawn stays at 100: the eval is expressed in centipawns, so
    // letting it float rescales every other weight at once.
    AddMaterial(out, "PieceValueQueen", kPieceValues[kQueen], 30);
    AddMaterial(out, "PieceValueRook", kPieceValues[kRook], 30);
    AddMaterial(out, "PieceValueBishop", kPieceValues[kBishop], 30);
    AddMaterial(out, "PieceValueKnight", kPieceValues[kKnight], 30);

    Add(out, "BishopPair", kBishopPairBonus);
    Add(out, "KnightPair", kKnightPairPenalty);
    Add(out, "RookPair", kRookPairPenalty);
    Add(out, "TradeDown", kTradeDownBonus);
    Add(out, "NoPawns", kNoPawnsPenalty);

    // Mobility. Each unit is multiplied by up to ~27 attacked squares, so these
    // get a tighter floor than the once-per-position weights.
    Add(out, "VisionMGQueen", kMG_VisionBonuses[kQueen], 6);
    Add(out, "VisionMGRook", kMG_VisionBonuses[kRook], 6);
    Add(out, "VisionMGBishop", kMG_VisionBonuses[kBishop], 6);
    Add(out, "VisionMGKnight", kMG_VisionBonuses[kKnight], 6);
    Add(out, "VisionMGKing", kMG_VisionBonuses[kKing], 6);

    // No EG king entry: KingSafety only ever adds king vision to the mg score.
    Add(out, "VisionEGQueen", kEG_VisionBonuses[kQueen], 6);
    Add(out, "VisionEGRook", kEG_VisionBonuses[kRook], 6);
    Add(out, "VisionEGBishop", kEG_VisionBonuses[kBishop], 6);
    Add(out, "VisionEGKnight", kEG_VisionBonuses[kKnight], 6);

    // King safety. Index 0 is the king's own rank, which the shield loop skips.
    for (size_t row = 1; row < static_cast<size_t>(kPawnShieldRowsInfront); ++row)
    {
        Add(out, "PawnShield" + std::to_string(row), kPawnShieldBonus[row]);
    }
    Add(out, "PawnShieldPenalty", kPawnShieldPenality);

    // Pawn structure.
    AddPhaseTable(out, "PassedPawn", kPassedPawnBonus, kFirstPawnRank, kLastPawnRank);
    AddPhaseTable(out, "CandidatePassed", kCandidatePassedPawnBonus, kFirstPawnRank, kLastPawnRank);
    AddPhaseTable(out, "ChainPawn", kChainPawnBonus, kFirstPawnRank, kLastPawnRank);
    AddPhaseTable(out, "PhalanxPawn", kPhalanxPawnBonus, kFirstPawnRank, kLastPawnRank);

    AddPhasePair(out, "IsolatedPawn", kIsolatedPawnPenalty);
    AddPhasePair(out, "DoubledPawn", kDoubledPawnPenalty);
    AddPhasePair(out, "BackwardPawn", kBackwardPawnPenalty);

    AddSearch(out);
    return out;
}

const std::unordered_map<std::string, size_t>& Index()
{
    static const std::unordered_map<std::string, size_t> index = []
    {
        std::unordered_map<std::string, size_t> map;
        const std::vector<Param>& params = Params();
        map.reserve(params.size());

        for (size_t i = 0; i < params.size(); ++i)
        {
            std::string key = params[i].name;
            for (char& c : key) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            map.emplace(std::move(key), i);
        }
        return map;
    }();
    return index;
}

}  // namespace

const std::vector<Param>& Params()
{
    static const std::vector<Param> params = Build();
    return params;
}

void PrintOptions(std::ostream& out)
{
    for (const Param& param : Params())
    {
        out << "option name " << param.name << " type spin"
            << " default " << param.def
            << " min " << param.min
            << " max " << param.max << '\n';
    }
}

void PrintSpsaConfig(std::ostream& out)
{
    const std::vector<Param>& params = Params();

    out << "{\n";
    for (size_t i = 0; i < params.size(); ++i)
    {
        const Param& param = params[i];

        // A step of a twentieth of the range puts the first perturbations well
        // inside the bounds while still being large enough to score. Everything but
        // kScaled lands in an integer, so a sub-1.0 step there would round most
        // perturbations back onto the current value and contribute no gradient.
        const double floor = (param.kind == Kind::kScaled) ? 0.5 : 1.0;
        const double step = std::max(floor, (param.max - param.min) / 20.0);

        out << "    \"" << param.name << "\": {"
            << "\"value\": " << param.def << ", "
            << "\"min_value\": " << param.min << ", "
            << "\"max_value\": " << param.max << ", "
            << "\"step\": " << step << "}";
        out << ((i + 1 < params.size()) ? ",\n" : "\n");
    }
    out << "}\n";
}

bool SetParam(const std::string& lowercase_name, int value)
{
    const auto it = Index().find(lowercase_name);
    if (it == Index().end()) { return false; }

    const Param& param = Params()[it->second];
    const int clamped = std::clamp(value, param.min, param.max);

    switch (param.kind)
    {
        case Kind::kScore:  *static_cast<Score*>(param.value) = static_cast<Score>(clamped); break;
        case Kind::kInt:    *static_cast<int*>(param.value) = clamped; break;
        case Kind::kUint8:  *static_cast<uint8_t*>(param.value) = static_cast<uint8_t>(clamped); break;
        case Kind::kScaled: *static_cast<double*>(param.value) = clamped / kScaleUnit; break;
    }
    return true;
}

}  // namespace eval_tuning
