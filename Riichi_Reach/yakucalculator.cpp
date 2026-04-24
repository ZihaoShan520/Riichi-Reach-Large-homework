// yakucalculator.cpp
#include "core/yakucalculator.h"
#include <algorithm>
#include <map>
#include <unordered_set>
#include <functional>
#include <Qstring>

// 🔹 修复：显式 switch 生成 key，避免字符串拼接歧义
static std::map<std::string, int> buildCounts(const std::vector<Tile>& hand) {
    std::map<std::string, int> counts;
    for (const auto& t : hand) {
        uint8_t val = (t.value == 0) ? 5 : t.value;  // 赤宝按5处理
        std::string key;
        switch (t.suit) {
        case TileSuit::MAN: key = std::to_string(val) + 'm'; break;
        case TileSuit::PIN: key = std::to_string(val) + 'p'; break;
        case TileSuit::SOU: key = std::to_string(val) + 's'; break;
        case TileSuit::ZI:  key = std::to_string(val) + 'z'; break;
        }
        counts[key]++;
    }
    return counts;
}

static bool isTerminalOrHonor(const Tile& t) {
    uint8_t v = (t.value == 0) ? 5 : t.value;
    return t.suit == TileSuit::ZI || v == 1 || v == 9;
}

static bool isSymmetric(const Tile& t) {
    uint8_t v = (t.value == 0) ? 5 : t.value;
    if (t.suit == TileSuit::SOU && (v==2||v==4||v==5||v==6||v==8||v==9)) return true;
    if (t.suit == TileSuit::PIN && (v==1||v==2||v==3||v==4||v==5||v==8||v==9)) return true;
    if (t.suit == TileSuit::ZI && v == 5) return true;
    return false;
}

static bool isGreen(const Tile& t) {
    uint8_t v = (t.value == 0) ? 5 : t.value;
    if (t.suit == TileSuit::SOU && (v==2||v==3||v==4||v==6||v==8)) return true;
    if (t.suit == TileSuit::ZI && v == 6) return true;
    return false;
}

// 🔹 和牌判定（支持标准型/七对/国士无双）
bool YakuCalculator::checkWinHand(const std::vector<Tile>& tiles) {
    int n = tiles.size();
    if (n != 8 && n != 11 && n != 14) return false;

    // 七对
    if (n == 14) {
        auto counts = buildCounts(tiles);
        int pairs = 0;
        for (const auto& kv : counts) {
            if (kv.second == 2) pairs++;
            else if (kv.second != 4) return false;
        }
        if (pairs >= 7) return true;
    }

    // 国士无双
    if (n == 14) {
        auto counts = buildCounts(tiles);
        std::vector<std::string> terminals = {"1m","9m","1p","9p","1s","9s","1z","2z","3z","4z","5z","6z","7z"};
        bool hasPair = false;
        for (const auto& key : terminals) {
            auto it = counts.find(key);
            if (it == counts.end()) return false;
            if (it->second == 2) { if (hasPair) return false; hasPair = true; }
            else if (it->second != 1) return false;
        }
        if (counts.size() == 13 || (counts.size() == 12 && hasPair)) return true;
    }

    // 标准型：雀头 + N面子
    auto counts = buildCounts(tiles);
    int setsNeeded = (n - 2) / 3;

    for (auto& kv : counts) {
        if (kv.second < 2) continue;
        auto temp = counts;
        temp[kv.first] -= 2;
        if (temp[kv.first] == 0) temp.erase(kv.first);

        std::function<bool(std::map<std::string,int>&, int)> checkMelds =
            [&](std::map<std::string,int>& cnt, int needed) -> bool {
            if (needed == 0) return cnt.empty();
            if (cnt.empty()) return false;
            auto it = cnt.begin();
            std::string key = it->first;
            int val = std::stoi(key);
            char suit = key.back();

            if (it->second >= 3) {
                auto next = cnt;
                next[key] -= 3;
                if (next[key] == 0) next.erase(key);
                if (checkMelds(next, needed - 1)) return true;
            }
            if (suit != 'z' && val <= 7) {
                std::string k2 = std::to_string(val+1) + suit;
                std::string k3 = std::to_string(val+2) + suit;
                auto it2 = cnt.find(k2), it3 = cnt.find(k3);
                if (it2 != cnt.end() && it3 != cnt.end() &&
                    it->second >= 1 && it2->second >= 1 && it3->second >= 1) {
                    auto next = cnt;
                    next[key]--; if (next[key]==0) next.erase(key);
                    next[k2]--; if (next[k2]==0) next.erase(k2);
                    next[k3]--; if (next[k3]==0) next.erase(k3);
                    if (checkMelds(next, needed - 1)) return true;
                }
            }
            return false;
        };
        if (checkMelds(temp, setsNeeded)) return true;
    }
    return false;
}

int YakuCalculator::calculateBasePoints(const std::vector<Tile>& tiles) {
    int pts = 0;
    for (const auto& t : tiles) {
        uint8_t val = (t.value == 0) ? 5 : t.value;
        pts += (t.suit == TileSuit::ZI || val == 1 || val == 9) ? 15 : 10;
    }
    return pts;
}

// ===== 番型检测器（修复版）=====
bool YakuCalculator::checkNoYaku(const std::vector<Tile>&, const std::vector<Tile>&) { return true; }

bool YakuCalculator::checkSequentialSix(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mps z"[static_cast<int>(suit)];
        for (int start = 1; start <= 4; ++start) {
            bool ok = true;
            for (int v = start; v <= start+5; ++v) {
                auto it = counts.find(std::to_string(v) + c);
                if (it == counts.end() || it->second == 0) { ok = false; break; }
            }
            if (ok) return true;
        }
    }
    return false;
}

bool YakuCalculator::checkAllSimples(const std::vector<Tile>& hand) {
    for (const auto& t : hand) if (isTerminalOrHonor(t)) return false;
    return true;
}

bool YakuCalculator::checkDragonPung(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (int v = 5; v <= 7; ++v) {
        auto it = counts.find(std::to_string(v) + 'z');
        if (it != counts.end() && it->second >= 3) outCount++;
    }
    return outCount > 0;
}

bool YakuCalculator::checkWindPung(const std::vector<Tile>& hand, uint8_t windVal) {
    auto counts = buildCounts(hand);
    auto it = counts.find(std::to_string(windVal) + 'z');
    return (it != counts.end() && it->second >= 3);
}

bool YakuCalculator::checkPureDoubleSequence(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mps z"[static_cast<int>(suit)];
        for (int v = 1; v <= 7; ++v) {
            std::string k1 = std::to_string(v)+c, k2 = std::to_string(v+1)+c, k3 = std::to_string(v+2)+c;
            auto it1 = counts.find(k1), it2 = counts.find(k2), it3 = counts.find(k3);
            if (it1 != counts.end() && it2 != counts.end() && it3 != counts.end() &&
                it1->second >= 2 && it2->second >= 2 && it3->second >= 2) return true;
        }
    }
    return false;
}

// 🔹 修复：四对检测（>=2张即算对子）
bool YakuCalculator::checkFourPairs(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pairs = 0;
    for (const auto& kv : counts) if (kv.second >= 2) pairs++;  // ✅ >=2 即可
    return pairs >= 4;
}

// 🔹 修复：双暗刻检测（>=3张即算刻子）
bool YakuCalculator::checkTwoConcealedPungs(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (const auto& kv : counts) if (kv.second >= 3) outCount++;  // ✅ >=3 即可
    return outCount >= 2;
}

bool YakuCalculator::checkFourIdentical(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (const auto& kv : counts) if (kv.second >= 4) outCount++;
    return outCount > 0;
}

bool YakuCalculator::checkSmallThreeDragons(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pungs = 0, pairs = 0;
    for (int v = 5; v <= 7; ++v) {
        auto it = counts.find(std::to_string(v) + 'z');
        if (it == counts.end()) continue;
        if (it->second >= 3) pungs++;
        else if (it->second >= 2) pairs++;
    }
    return pungs == 2 && pairs >= 1;
}

bool YakuCalculator::checkNotBreaking(const std::vector<Tile>& hand) {
    int sym = 0;
    for (const auto& t : hand) if (isSymmetric(t)) sym++;
    return sym >= 9;
}

bool YakuCalculator::checkPureStraight(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mps z"[static_cast<int>(suit)];
        int total = 0;
        for (int v = 1; v <= 9; ++v) {
            auto it = counts.find(std::to_string(v) + c);
            if (it != counts.end()) total += std::min(it->second, 1);
        }
        if (total >= 9) return true;
    }
    return false;
}

bool YakuCalculator::checkMixedTripleSequence(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (int v = 1; v <= 7; ++v) {
        bool ok = true;
        for (char c : {'m','p','s'}) {
            if (counts.find(std::to_string(v)+c) == counts.end() ||
                counts.find(std::to_string(v+1)+c) == counts.end() ||
                counts.find(std::to_string(v+2)+c) == counts.end()) { ok = false; break; }
        }
        if (ok) return true;
    }
    return false;
}

// 🔹 替换 yakucalculator.cpp 中的原 checkAllTerminals 函数
bool YakuCalculator::checkAllTerminals(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);

    // 递归回溯：尝试从计数表中提取 needed 个含幺九的面子
    std::function<bool(std::map<std::string, int>&, int)> extractTerminalMelds =
        [&](std::map<std::string, int>& cnt, int needed) -> bool {
        if (needed == 0) return true;
        if (cnt.empty()) return false;

        auto it = cnt.begin();
        std::string key = it->first;
        int val = std::stoi(key);
        char suit = key.back();
        bool isTerm = (suit == 'z' || val == 1 || val == 9);

        // 1. 尝试用当前牌组成刻子（仅限幺九/字牌）
        if (isTerm && it->second >= 3) {
            auto next = cnt;
            next[key] -= 3; if (next[key] == 0) next.erase(key);
            if (extractTerminalMelds(next, needed - 1)) return true;
        }

        // 2. 尝试组成含幺九的顺子（仅 123 或 789）
        if (suit != 'z') {
            if (val == 1) { // 123
                std::string k2 = std::to_string(val+1) + suit;
                std::string k3 = std::to_string(val+2) + suit;
                auto it2 = cnt.find(k2), it3 = cnt.find(k3);
                if (it2 != cnt.end() && it3 != cnt.end()) {
                    auto next = cnt;
                    next[key]--; if(next[key]==0) next.erase(key);
                    next[k2]--; if(next[k2]==0) next.erase(k2);
                    next[k3]--; if(next[k3]==0) next.erase(k3);
                    if (extractTerminalMelds(next, needed - 1)) return true;
                }
            } else if (val == 7) { // 789
                std::string k2 = std::to_string(val+1) + suit;
                std::string k3 = std::to_string(val+2) + suit;
                auto it2 = cnt.find(k2), it3 = cnt.find(k3);
                if (it2 != cnt.end() && it3 != cnt.end()) {
                    auto next = cnt;
                    next[key]--; if(next[key]==0) next.erase(key);
                    next[k2]--; if(next[k2]==0) next.erase(k2);
                    next[k3]--; if(next[k3]==0) next.erase(k3);
                    if (extractTerminalMelds(next, needed - 1)) return true;
                }
            }
        }

        // 3. 当前牌无法参与构成有效面子，跳过并继续搜索
        auto skip = cnt;
        skip[key]--; if (skip[key] == 0) skip.erase(key);
        return extractTerminalMelds(skip, needed);
    };

    return extractTerminalMelds(counts, 3); // 需成功提取3组
}

bool YakuCalculator::checkThreeConcealedPungs(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (const auto& kv : counts) if (kv.second >= 3) outCount++;
    return outCount >= 3;
}

bool YakuCalculator::checkFiveFamilies(const std::vector<Tile>& hand) {
    bool hasMan=false, hasPin=false, hasSou=false, hasWind=false, hasDragon=false;
    for (const auto& t : hand) {
        if (t.suit == TileSuit::MAN) hasMan = true;
        else if (t.suit == TileSuit::PIN) hasPin = true;
        else if (t.suit == TileSuit::SOU) hasSou = true;
        else if (t.suit == TileSuit::ZI) {
            uint8_t v = (t.value==0)?5:t.value;
            if (v<=4) hasWind = true; else hasDragon = true;
        }
    }
    return hasMan && hasPin && hasSou && hasWind && hasDragon;
}

bool YakuCalculator::checkSevenPairs(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pairs = 0;
    for (const auto& kv : counts) if (kv.second == 2) pairs++;
    return pairs >= 7;
}

bool YakuCalculator::checkTripleTriplets(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (int v = 1; v <= 9; ++v) {
        int found = 0;
        for (char c : {'m','p','s'}) {
            auto it = counts.find(std::to_string(v) + c);
            if (it != counts.end() && it->second >= 3) found++;
        }
        if (found >= 3) return true;
    }
    return false;
}

bool YakuCalculator::checkMixedTerminalHonors(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pungs = 0, pairs = 0;
    for (const auto& kv : counts) {
        uint8_t v = std::stoi(kv.first);
        char c = kv.first.back();
        if (c=='z' || v==1 || v==9) {
            if (kv.second >= 3) pungs++;
            else if (kv.second >= 2) pairs++;
        }
    }
    return pungs >= 3 || pairs >= 6;
}

bool YakuCalculator::checkTwoPureDoubleSequences(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int cups = 0;
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mps z"[static_cast<int>(suit)];
        bool found = false;
        for (int v = 1; v <= 7; ++v) {
            std::string k1 = std::to_string(v)+c, k2 = std::to_string(v+1)+c, k3 = std::to_string(v+2)+c;
            auto it1 = counts.find(k1), it2 = counts.find(k2), it3 = counts.find(k3);
            if (it1 != counts.end() && it2 != counts.end() && it3 != counts.end() &&
                it1->second >= 2 && it2->second >= 2 && it3->second >= 2) { found = true; break; }
        }
        if (found) cups++;
    }
    return cups >= 2;
}

bool YakuCalculator::checkFullFlush(const std::vector<Tile>& hand) {
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        auto counts = buildCounts(hand);
        char c = "mps z"[static_cast<int>(suit)];
        int total = 0;
        for (int v = 1; v <= 9; ++v) {
            auto it = counts.find(std::to_string(v) + c);
            if (it != counts.end()) total += it->second;
        }
        if (total >= 10) return true;
    }
    return false;
}

bool YakuCalculator::checkPureTripleSequence(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mps z"[static_cast<int>(suit)];
        for (int v = 1; v <= 7; ++v) {
            std::string k1 = std::to_string(v)+c, k2 = std::to_string(v+1)+c, k3 = std::to_string(v+2)+c;
            auto it1 = counts.find(k1), it2 = counts.find(k2), it3 = counts.find(k3);
            if (it1 != counts.end() && it2 != counts.end() && it3 != counts.end() &&
                it1->second >= 3 && it2->second >= 3 && it3->second >= 3) return true;
        }
    }
    return false;
}

bool YakuCalculator::checkAllHonors(const std::vector<Tile>& hand) {
    int honors = 0;
    for (const auto& t : hand) if (t.suit == TileSuit::ZI) honors++;
    return honors >= 11;
}

bool YakuCalculator::checkBigThreeDragons(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pungs = 0;
    for (int v = 5; v <= 7; ++v) {
        auto it = counts.find(std::to_string(v) + 'z');
        if (it != counts.end() && it->second >= 3) pungs++;
    }
    return pungs == 3;
}

bool YakuCalculator::checkFourConcealedPungs(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (const auto& kv : counts) if (kv.second >= 3) outCount++;
    return outCount >= 4;
}

bool YakuCalculator::checkFullGreen(const std::vector<Tile>& hand) {
    int green = 0;
    for (const auto& t : hand) if (isGreen(t)) green++;
    return green >= 11;
}

bool YakuCalculator::checkFourWinds(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pungs = 0, pairs = 0;
    for (int v = 1; v <= 4; ++v) {
        auto it = counts.find(std::to_string(v) + 'z');
        if (it == counts.end()) continue;
        if (it->second >= 3) pungs++;
        else if (it->second >= 2) pairs++;
    }
    return pungs == 4 || (pungs == 3 && pairs >= 1);
}

bool YakuCalculator::checkNineGates(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mps z"[static_cast<int>(suit)];
        bool match = true;
        for (int v = 1; v <= 9; ++v) {
            int req = (v==1 || v==9) ? 3 : 1;
            auto it = counts.find(std::to_string(v) + c);
            if (it == counts.end() || it->second < req) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

bool YakuCalculator::checkMillionStone(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int sum = 0;
    char c = 'm';
    for (int v = 1; v <= 9; ++v) {
        auto it = counts.find(std::to_string(v) + c);
        if (it != counts.end()) sum += v * it->second;
    }
    return sum >= 100;
}

bool YakuCalculator::checkOnePointRed(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int total = 0;
    char c = 'p';
    for (int v : {1,3,5,6,7,9}) {
        auto it = counts.find(std::to_string(v) + c);
        if (it != counts.end()) total += it->second;
    }
    return total >= 12;
}

// 互斥规则
std::unordered_map<YakuType, std::vector<YakuType>> YakuCalculator::buildExclusionMap() {
    using Y = YakuType;
    std::unordered_map<Y, std::vector<Y>> map;
    map[Y::SequentialSix] = {Y::PureStraight, Y::NineGates};
    map[Y::DragonPung] = {Y::SmallThreeDragons, Y::BigThreeDragons};
    map[Y::PrevalentWind] = {Y::FourWinds};
    map[Y::SeatWind] = {Y::FourWinds};
    map[Y::PureDoubleSequence] = {Y::TwoPureDoubleSequences, Y::SevenPairs, Y::PureTripleSequence};
    map[Y::FourPairs] = {Y::SevenPairs, Y::TwoPureDoubleSequences, Y::FourConcealedPungs, Y::FourWinds, Y::ThreeConcealedPungs, Y::PureTripleSequence};
    map[Y::TwoConcealedPungs] = {Y::ThreeConcealedPungs, Y::FourConcealedPungs, Y::SmallThreeDragons, Y::BigThreeDragons, Y::FourWinds, Y::NineGates, Y::PureTripleSequence};
    map[Y::SmallThreeDragons] = {Y::BigThreeDragons};
    map[Y::PureStraight] = {Y::NineGates};
    map[Y::AllTerminals] = {Y::MixedTerminalHonors, Y::AllHonors, Y::BigThreeDragons, Y::FourWinds};
    map[Y::ThreeConcealedPungs] = {Y::BigThreeDragons, Y::FourWinds, Y::FourConcealedPungs, Y::PureTripleSequence};
    map[Y::FiveFamilies] = {};
    map[Y::MixedTerminalHonors] = {Y::BigThreeDragons, Y::FourWinds, Y::AllHonors};
    map[Y::FullFlush] = {Y::MillionStone, Y::OnePointRed, Y::NineGates};
    map[Y::FourConcealedPungs] = {Y::FourWinds};
    map[Y::BigThreeDragons] = {Y::AllHonors};
    map[Y::FourWinds] = {Y::AllHonors};
    map[Y::FullGreen] = {Y::FullFlush};
    return map;
}

void YakuCalculator::applyExclusions(std::vector<YakuResult>& detectedYakus) {
    auto excl = buildExclusionMap();
    std::unordered_set<YakuType> excluded;
    std::sort(detectedYakus.begin(), detectedYakus.end(), [](const YakuResult& a, const YakuResult& b){ return a.fan > b.fan; });
    std::vector<YakuResult> kept;
    for (const auto& y : detectedYakus) {
        if (excluded.count(y.type)) continue;
        kept.push_back(y);
        if (excl.count(y.type)) for (auto ex : excl.at(y.type)) excluded.insert(ex);
    }
    detectedYakus = std::move(kept);
}

// 🔹 核心计分入口（严格按4分支流程）
ScoreResult YakuCalculator::calculateScore(const std::vector<Tile>& played,
                                           const std::vector<Tile>& playOrder,
                                           uint8_t prevalentWind,
                                           uint8_t seatWind) {
    ScoreResult res;
    if (played.empty()) return res;

    bool isWin = checkWinHand(played);

    // 检测所有番型
    std::vector<YakuResult> active;

    // 1番系
    if (checkSequentialSix(played)) active.push_back({YakuType::SequentialSix, 1, "连六"});
    if (checkAllSimples(played)) active.push_back({YakuType::AllSimples, 1, "断幺"});

    int dragonCnt = 0;
    if (checkDragonPung(played, dragonCnt)) active.push_back({YakuType::DragonPung, dragonCnt, "三元牌"});
    if (checkWindPung(played, prevalentWind)) active.push_back({YakuType::PrevalentWind, 1, "场风牌"});
    if (checkWindPung(played, seatWind)) active.push_back({YakuType::SeatWind, 1, "门风牌"});
    if (checkPureDoubleSequence(played)) active.push_back({YakuType::PureDoubleSequence, 1, "一杯口"});

    // 2番系
    if (checkFourPairs(played)) active.push_back({YakuType::FourPairs, 2, "四对"});
    int pungCnt = 0;
    if (checkTwoConcealedPungs(played, pungCnt)) active.push_back({YakuType::TwoConcealedPungs, 2, "双暗刻"});
    int quadCnt = 0;
    if (checkFourIdentical(played, quadCnt)) active.push_back({YakuType::FourIdentical, quadCnt * 2, "四归一"});
    if (checkSmallThreeDragons(played)) active.push_back({YakuType::SmallThreeDragons, 2, "小三元"});
    if (checkNotBreaking(played)) active.push_back({YakuType::NotBreaking, 2, "推不倒"});
    if (checkPureStraight(played)) active.push_back({YakuType::PureStraight, 2, "一气通贯"});
    if (checkMixedTripleSequence(played)) active.push_back({YakuType::MixedTripleSequence, 2, "三色同顺"});
    if (checkAllTerminals(played)) active.push_back({YakuType::AllTerminals, 2, "全带幺九"});
    if (checkThreeConcealedPungs(played, pungCnt)) active.push_back({YakuType::ThreeConcealedPungs, 2, "三暗刻"});
    if (checkFiveFamilies(played)) active.push_back({YakuType::FiveFamilies, 2, "五门齐"});

    // 3番系
    if (checkSevenPairs(played)) active.push_back({YakuType::SevenPairs, 3, "七对"});
    if (checkTripleTriplets(played)) active.push_back({YakuType::TripleTriplets, 3, "三色同刻"});
    if (checkMixedTerminalHonors(played)) active.push_back({YakuType::MixedTerminalHonors, 3, "混老头"});
    if (checkTwoPureDoubleSequences(played)) active.push_back({YakuType::TwoPureDoubleSequences, 3, "二杯口"});

    // 5番系
    if (checkFullFlush(played)) active.push_back({YakuType::FullFlush, 5, "清一色"});
    if (checkPureTripleSequence(played)) active.push_back({YakuType::PureTripleSequence, 5, "一色三步高"});

    // 13番系
    if (checkAllHonors(played)) active.push_back({YakuType::AllHonors, 13, "字一色"});
    if (checkBigThreeDragons(played)) active.push_back({YakuType::BigThreeDragons, 13, "大三元"});
    if (checkFourConcealedPungs(played, pungCnt)) active.push_back({YakuType::FourConcealedPungs, 13, "四暗刻"});
    if (checkFullGreen(played)) active.push_back({YakuType::FullGreen, 13, "绿一色"});
    if (checkFourWinds(played)) active.push_back({YakuType::FourWinds, 13, "四喜和"});
    if (checkNineGates(played)) active.push_back({YakuType::NineGates, 13, "九莲宝灯"});
    if (checkMillionStone(played)) active.push_back({YakuType::MillionStone, 13, "百万石"});
    if (checkOnePointRed(played)) active.push_back({YakuType::OnePointRed, 13, "一点红"});

    applyExclusions(active);

    // 计算番数和
    double yakuSum = 0;
    for (const auto& y : active) {
        yakuSum += y.fan;
        res.activeYakus.push_back(y.type);
    }
    bool hasYaku = (yakuSum > 0.0);

    // 🔹 严格按4分支计分
    int allPoints = calculateBasePoints(played);
    int firstFivePoints = 0;
    int limit = std::min((int)playOrder.size(), 5);
    for (int i = 0; i < limit; ++i) {
        firstFivePoints += calculateBasePoints({playOrder[i]});
    }

    if (!hasYaku) {
        // 无役分支
        res.activeYakus = {YakuType::NoYaku};
        res.basePoints = isWin ? allPoints : firstFivePoints;
        res.totalFan = 0.5 * (isWin ? 3.0 : 1.0);
    } else {
        // 有役分支
        res.basePoints = allPoints;
        res.totalFan = (yakuSum + 0.5) * (isWin ? 3.0 : 1.0);
    }

    res.isWinHand = isWin;
    res.finalScore = static_cast<int>(res.basePoints * res.totalFan);
    return res;
}

// yakucalculator.cpp - 文件末尾添加：

QString YakuCalculator::yakuName(YakuType type) {
    switch (type) {
    case YakuType::NoYaku: return "无役";
    case YakuType::SequentialSix: return "连六";
    case YakuType::AllSimples: return "断幺";
    case YakuType::DragonPung: return "三元牌";
    case YakuType::PrevalentWind: return "场风牌";
    case YakuType::SeatWind: return "门风牌";
    case YakuType::PureDoubleSequence: return "一杯口";
    case YakuType::FourPairs: return "四对";
    case YakuType::TwoConcealedPungs: return "双暗刻";
    case YakuType::FourIdentical: return "四归一";
    case YakuType::SmallThreeDragons: return "小三元";
    case YakuType::NotBreaking: return "推不倒";
    case YakuType::PureStraight: return "一气通贯";
    case YakuType::MixedTripleSequence: return "三色同顺";
    case YakuType::AllTerminals: return "全带幺九";
    case YakuType::ThreeConcealedPungs: return "三暗刻";
    case YakuType::FiveFamilies: return "五门齐";
    case YakuType::SevenPairs: return "七对";
    case YakuType::TripleTriplets: return "三色同刻";
    case YakuType::MixedTerminalHonors: return "混老头";
    case YakuType::TwoPureDoubleSequences: return "二杯口";
    case YakuType::FullFlush: return "清一色";
    case YakuType::PureTripleSequence: return "一色三步高";
    case YakuType::AllHonors: return "字一色";
    case YakuType::BigThreeDragons: return "大三元";
    case YakuType::FourConcealedPungs: return "四暗刻";
    case YakuType::FullGreen: return "绿一色";
    case YakuType::FourWinds: return "四喜和";
    case YakuType::NineGates: return "九莲宝灯";
    case YakuType::MillionStone: return "百万石";
    case YakuType::OnePointRed: return "一点红";
    default: return "未知番型";
    }
}
