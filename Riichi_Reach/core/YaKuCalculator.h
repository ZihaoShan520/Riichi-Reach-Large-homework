// yakucalculator.h
#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "tile.h"

enum class YakuType : uint8_t {
    None = 0, NoYaku,
    SequentialSix, AllSimples, DragonPung, PrevalentWind, SeatWind, PureDoubleSequence,
    FourPairs, TwoConcealedPungs, FourIdentical, SmallThreeDragons, NotBreaking,
    PureStraight, MixedTripleSequence, AllTerminals, ThreeConcealedPungs, FiveFamilies,
    SevenPairs, TripleTriplets, MixedTerminalHonors, TwoPureDoubleSequences,
    FullFlush, PureTripleSequence,
    AllHonors, BigThreeDragons, FourConcealedPungs, FullGreen, FourWinds,
    NineGates, MillionStone, OnePointRed
};

struct YakuResult {
    YakuType type;
    int fan;
    std::string name;
};

struct ScoreResult {
    int basePoints = 0;
    double totalFan = 0.0;
    int finalScore = 0;
    bool isWinHand = false;
    std::vector<YakuType> activeYakus;
};

class YakuCalculator {
public:
    static ScoreResult calculateScore(const std::vector<Tile>& played,
                                      const std::vector<Tile>& playOrder,
                                      uint8_t prevalentWind = 1,
                                      uint8_t seatWind = 1);
    static bool checkWinHand(const std::vector<Tile>& tiles);
    static QString yakuName(YakuType type);

private:
    static int calculateBasePoints(const std::vector<Tile>& tiles);

    // 番型检测器（全部接收 vector<Tile>）
    static bool checkNoYaku(const std::vector<Tile>&, const std::vector<Tile>&);
    static bool checkSequentialSix(const std::vector<Tile>&);
    static bool checkAllSimples(const std::vector<Tile>&);
    static bool checkDragonPung(const std::vector<Tile>&, int&);
    static bool checkWindPung(const std::vector<Tile>&, uint8_t);
    static bool checkPureDoubleSequence(const std::vector<Tile>&);
    static bool checkFourPairs(const std::vector<Tile>&);
    static bool checkTwoConcealedPungs(const std::vector<Tile>&, int&);
    static bool checkFourIdentical(const std::vector<Tile>&, int&);
    static bool checkSmallThreeDragons(const std::vector<Tile>&);
    static bool checkNotBreaking(const std::vector<Tile>&);
    static bool checkPureStraight(const std::vector<Tile>&);
    static bool checkMixedTripleSequence(const std::vector<Tile>&);
    static bool checkAllTerminals(const std::vector<Tile>&);
    static bool checkThreeConcealedPungs(const std::vector<Tile>&, int&);
    static bool checkFiveFamilies(const std::vector<Tile>&);
    static bool checkSevenPairs(const std::vector<Tile>&);
    static bool checkTripleTriplets(const std::vector<Tile>&);
    static bool checkMixedTerminalHonors(const std::vector<Tile>&);
    static bool checkTwoPureDoubleSequences(const std::vector<Tile>&);
    static bool checkFullFlush(const std::vector<Tile>&);
    static bool checkPureTripleSequence(const std::vector<Tile>&);
    static bool checkAllHonors(const std::vector<Tile>&);
    static bool checkBigThreeDragons(const std::vector<Tile>&);
    static bool checkFourConcealedPungs(const std::vector<Tile>&, int&);
    static bool checkFullGreen(const std::vector<Tile>&);
    static bool checkFourWinds(const std::vector<Tile>&);
    static bool checkNineGates(const std::vector<Tile>&);
    static bool checkMillionStone(const std::vector<Tile>&);
    static bool checkOnePointRed(const std::vector<Tile>&);

    static void applyExclusions(std::vector<YakuResult>&);
    static std::unordered_map<YakuType, std::vector<YakuType>> buildExclusionMap();
};
