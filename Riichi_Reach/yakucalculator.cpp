// yakucalculator.cpp
#include "core/yakucalculator.h"
#include <algorithm>
#include <map>
#include <unordered_set>
#include <functional>
#include <QString>
#include <QDebug>

static std::map<std::string, int> buildCounts(const std::vector<Tile>& hand) {
    std::map<std::string, int> counts;
    for (const auto& t : hand) {
        uint8_t val = (t.value == 0) ? 5 : t.value;
        std::string key = std::to_string(val) + "mpsz"[static_cast<int>(t.suit)];
        counts[key]++;
    } return counts;
}
static bool isTerminalOrHonor(const Tile& t) { uint8_t v=(t.value==0)?5:t.value; return t.suit==TileSuit::ZI||v==1||v==9; }
static bool isSymmetric(const Tile& t) { uint8_t v=(t.value==0)?5:t.value; return (t.suit==TileSuit::SOU&&(v==2||v==4||v==5||v==6||v==8||v==9))||(t.suit==TileSuit::PIN&&(v==1||v==2||v==3||v==4||v==5||v==8||v==9))||(t.suit==TileSuit::ZI&&v==5); }
static bool isGreen(const Tile& t) { uint8_t v=(t.value==0)?5:t.value; return (t.suit==TileSuit::SOU&&(v==2||v==3||v==4||v==6||v==8))||(t.suit==TileSuit::ZI&&v==6); }

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
        }
        if (pairs >= 7) return true;
    }

    // 国士无双
    if (n == 14) {
        qDebug() << "[Kokushi] Checking Thirteen Orphans...";
        auto counts = buildCounts(tiles);
        std::vector<std::string> terminals = {"1m","9m","1p","9p","1s","9s","1z","2z","3z","4z","5z","6z","7z"};
        bool hasPair = false;
        bool checkflagguoshi = true; // 使用 bool 更规范

        for (const auto& key : terminals) {
            auto it = counts.find(key);
            if (it == counts.end()) {
                qDebug() << "[Kokushi] Missing required terminal:" << QString::fromStdString(key);
                checkflagguoshi = false;
                break;
            }
            qDebug() << "[Kokushi] Found:" << QString::fromStdString(key) << "count:" << it->second;

            if (it->second == 2) {
                if (hasPair) {
                    qDebug() << "[Kokushi] Second pair detected! Failing.";
                    checkflagguoshi = false;
                    break;
                }
                hasPair = true;
                qDebug() << "[Kokushi] Set hasPair = true";
            } else if (it->second != 1) {
                qDebug() << "[Kokushi] Invalid count (not 1 or 2):" << it->second << "for" << QString::fromStdString(key);
                checkflagguoshi = false;
                break;
            }
        }

        qDebug() << "[Kokushi] Final check - counts.size():" << counts.size()
                 << "hasPair:" << hasPair
                 << "checkflagguoshi:" << checkflagguoshi;

        if (counts.size() == 13 && hasPair && checkflagguoshi) {
            qDebug() << "[Kokushi] SUCCESS! Returning true.";
            return true;
        } else {
            qDebug() << "[Kokushi] FAILED. Continuing to standard check...";
        }
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

// ===== 番型检测器 =====
bool YakuCalculator::checkNoYaku(const std::vector<Tile>&, const std::vector<Tile>&) { return true; }

bool YakuCalculator::checkSequentialSix(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    for (auto suit : {TileSuit::MAN, TileSuit::PIN, TileSuit::SOU}) {
        char c = "mpsz"[static_cast<int>(suit)];
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
        char c = "mpsz"[static_cast<int>(suit)];
        for (int v = 1; v <= 7; ++v) {
            std::string k1 = std::to_string(v)+c, k2 = std::to_string(v+1)+c, k3 = std::to_string(v+2)+c;
            auto it1 = counts.find(k1), it2 = counts.find(k2), it3 = counts.find(k3);
            if (it1 != counts.end() && it2 != counts.end() && it3 != counts.end() &&
                it1->second >= 2 && it2->second >= 2 && it3->second >= 2) return true;
        }
    }
    return false;
}

bool YakuCalculator::checkFourPairs(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    int pairs = 0;
    for (const auto& kv : counts) if (kv.second >= 2) pairs++;
    return pairs >= 4;
}

bool YakuCalculator::checkTwoConcealedPungs(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (const auto& kv : counts) if (kv.second >= 3) outCount++;
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
        char c = "mpsz"[static_cast<int>(suit)];
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

bool YakuCalculator::checkAllTerminals(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    std::function<bool(std::map<std::string, int>&, int)> extractTerminalMelds =
        [&](std::map<std::string, int>& cnt, int needed) -> bool {
        if (needed == 0) return true;
        if (cnt.empty()) return false;
        auto it = cnt.begin();
        std::string key = it->first;
        int val = std::stoi(key);
        char suit = key.back();
        bool isTerm = (suit == 'z' || val == 1 || val == 9);

        if (isTerm && it->second >= 3) {
            auto next = cnt;
            next[key] -= 3; if (next[key] == 0) next.erase(key);
            if (extractTerminalMelds(next, needed - 1)) return true;
        }
        if (suit != 'z') {
            if (val == 1) {
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
            } else if (val == 7) {
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
        auto skip = cnt;
        skip[key]--; if (skip[key] == 0) skip.erase(key);
        return extractTerminalMelds(skip, needed);
    };
    return extractTerminalMelds(counts, 3);
}

bool YakuCalculator::checkThreeConcealedPungs(const std::vector<Tile>& hand, int& outCount) {
    auto counts = buildCounts(hand);
    outCount = 0;
    for (const auto& kv : counts) if (kv.second >= 3) outCount++;
    return outCount >= 3;
}

bool YakuCalculator::checkFiveFamilies(const std::vector<Tile>& hand) {
    int countMan = 0, countPin = 0, countSou = 0, countWind = 0, countDragon = 0;
    for (const auto& t : hand) {
        switch (t.suit) {
        case TileSuit::MAN: countMan++; break;
        case TileSuit::PIN: countPin++; break;
        case TileSuit::SOU: countSou++; break;
        case TileSuit::ZI: {
            uint8_t v = (t.value == 0) ? 5 : t.value;
            if (v >= 1 && v <= 4) countWind++;
            else if (v >= 5 && v <= 7) countDragon++;
            break;
        }
        }
    }
    return countMan >= 2 && countPin >= 2 && countSou >= 2 && countWind >= 2 && countDragon >= 2;
}

bool YakuCalculator::checkSevenPairs(const std::vector<Tile>& hand) {
    if (hand.size() != 14) return false;
    auto counts = buildCounts(hand);
    int pairs = 0;
    for (const auto& kv : counts) {
        if (kv.second == 2) pairs++;
        else return false;
    }
    return pairs == 7;
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
        char c = "mpsz"[static_cast<int>(suit)];
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
        char c = "mpsz"[static_cast<int>(suit)];
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
        char c = "mpsz"[static_cast<int>(suit)];
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
        char c = "mpsz"[static_cast<int>(suit)];
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

bool YakuCalculator::checkguoshi(const std::vector<Tile>& hand) {
    auto counts = buildCounts(hand);
    std::vector<std::string> terminals = {"1m","9m","1p","9p","1s","9s","1z","2z","3z","4z","5z","6z","7z"};
    int cnt = 0;
    for (const auto& key : terminals) {
        auto it = counts.find(key);
        if (it == counts.end()) return 0;
    }
    return 1;
}

// 互斥规则
std::unordered_map<YakuType, std::vector<YakuType>> YakuCalculator::buildExclusionMap() {
    using Y = YakuType;
    std::unordered_map<Y, std::vector<Y>> map;

    // 🔹 1番系覆盖
    map[Y::SequentialSix]       = {Y::PureStraight, Y::NineGates};
    map[Y::DragonPung]          = {Y::SmallThreeDragons, Y::BigThreeDragons};
    map[Y::PrevalentWind]       = {Y::FourWinds};
    map[Y::SeatWind]            = {Y::FourWinds};
    map[Y::PureDoubleSequence]  = {Y::TwoPureDoubleSequences, Y::SevenPairs, Y::PureTripleSequence};

    // 🔹 2番系覆盖
    map[Y::FourPairs]           = {Y::SevenPairs, Y::TwoPureDoubleSequences, Y::FourConcealedPungs, Y::FourWinds, Y::ThreeConcealedPungs, Y::PureTripleSequence};
    map[Y::TwoConcealedPungs]   = {Y::ThreeConcealedPungs, Y::FourConcealedPungs, Y::SmallThreeDragons, Y::BigThreeDragons, Y::FourWinds, Y::NineGates, Y::PureTripleSequence};
    map[Y::SmallThreeDragons]   = {Y::BigThreeDragons};
    map[Y::PureStraight]        = {Y::NineGates};
    map[Y::AllTerminals]        = {Y::MixedTerminalHonors, Y::AllHonors, Y::BigThreeDragons, Y::FourWinds};
    map[Y::ThreeConcealedPungs] = {Y::BigThreeDragons, Y::FourWinds, Y::FourConcealedPungs, Y::PureTripleSequence};
    map[Y::FiveFamilies]        = {Y::guoshi};

    // 🔹 3番系覆盖
    map[Y::SevenPairs]          = {Y::FourPairs, Y::PureDoubleSequence};
    map[Y::MixedTerminalHonors] = {Y::BigThreeDragons, Y::FourWinds, Y::AllHonors};
    map[Y::TwoPureDoubleSequences] = {Y::FourPairs, Y::PureDoubleSequence};

    // 🔹 5番系覆盖
    map[Y::FullFlush]           = {Y::MillionStone, Y::OnePointRed, Y::NineGates, Y::FullGreen};
    map[Y::PureTripleSequence]  = {Y::ThreeConcealedPungs, Y::TwoConcealedPungs, Y::FourPairs, Y::PureDoubleSequence};

    // 🔹 13番系覆盖 & 同级互斥（大三元↔字一色，四喜和↔字一色，绿一色↔清一色）
    map[Y::AllHonors]           = {Y::BigThreeDragons, Y::FourWinds, Y::FullGreen};
    map[Y::BigThreeDragons]     = {Y::AllHonors, Y::SmallThreeDragons, Y::DragonPung, Y::AllTerminals, Y::MixedTerminalHonors};
    map[Y::FourConcealedPungs]  = {Y::FourWinds, Y::ThreeConcealedPungs, Y::TwoConcealedPungs, Y::FourPairs};
    map[Y::FullGreen]           = {Y::FullFlush, Y::AllHonors};
    map[Y::FourWinds]           = {Y::AllHonors, Y::BigThreeDragons, Y::PrevalentWind, Y::SeatWind};
    map[Y::NineGates]           = {Y::FullFlush, Y::SequentialSix, Y::PureStraight, Y::TwoConcealedPungs};
    map[Y::MillionStone]        = {Y::FullFlush};
    map[Y::OnePointRed]         = {Y::FullFlush};

    map[Y::guoshi]              = {Y::FiveFamilies};

    return map;
}

void YakuCalculator::applyExclusions(std::vector<YakuResult>& detectedYakus) {
    auto exclMap = buildExclusionMap();
    std::unordered_set<YakuType> excluded;

    // 1️⃣ 核心：按番数降序排序，确保高番型永远优先被判定
    std::sort(detectedYakus.begin(), detectedYakus.end(), [](const YakuResult& a, const YakuResult& b) {
        return a.fan > b.fan;
    });

    std::vector<YakuResult> kept;
    for (const auto& yaku : detectedYakus) {
        // 2️⃣ 若已被更高番型覆盖，直接丢弃
        if (excluded.count(yaku.type)) {
            continue;
        }
        // 3️⃣ 保留当前高番型
        kept.push_back(yaku);
        // 4️⃣ 将其覆盖的所有低番型加入黑名单
        if (exclMap.count(yaku.type)) {
            for (auto ex : exclMap.at(yaku.type)) {
                excluded.insert(ex);
            }
        }
    }
    // 5️⃣ 替换为过滤后的最终列表
    detectedYakus = std::move(kept);
}

// 🔹 核心计分入口（已集成宝牌逻辑）
ScoreResult YakuCalculator::calculateScore(const std::vector<Tile>& played,
                                           const std::vector<Tile>& playOrder,
                                           uint8_t prevalentWind,
                                           uint8_t seatWind,
                                           const Tile& doraTile) {
    ScoreResult res;
    if (played.empty()) return res;

    bool isWin = checkWinHand(played);
    std::vector<YakuResult> active;

    // 1番系
    if (checkSequentialSix(played)) active.push_back({YakuType::SequentialSix, 1, "连六"});
    if (checkAllSimples(played)) active.push_back({YakuType::AllSimples, 1, "断幺"});
    int dragonCnt = 0;
    int pungCnt = 0;
    if (checkTwoConcealedPungs(played, pungCnt)) active.push_back({YakuType::TwoConcealedPungs, 1, "双暗刻"});
    if (checkDragonPung(played, dragonCnt)) active.push_back({YakuType::DragonPung, dragonCnt, "三元牌"});
    if (checkWindPung(played, prevalentWind)) active.push_back({YakuType::PrevalentWind, 1, "场风牌"});
    if (checkWindPung(played, seatWind)) active.push_back({YakuType::SeatWind, 1, "门风牌"});
    if (checkPureDoubleSequence(played)) active.push_back({YakuType::PureDoubleSequence, 1, "一杯口"});

    // 2番系
    if (checkFourPairs(played)) active.push_back({YakuType::FourPairs, 2, "四对"});
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

    if(checkguoshi(played)) active.push_back({YakuType::guoshi, 20, "国士无双"});

    applyExclusions(active);

    // 计算番数和
    double yakuSum = 0;
    for (const auto& y : active) {
        yakuSum += y.fan;
        res.activeYakus.push_back(y.type);
    }
    bool hasYaku = (yakuSum > 0);

    // 🔹 宝牌/赤宝牌计数加成（每张+0.5番）
    double doraBonus = 0;
    uint8_t dVal = (doraTile.value == 0) ? 5 : doraTile.value; // 宝牌点数归一化
    for (const auto& t : played) {
        int tileDoraCount = 0;
        // 1. 赤宝牌必计1番（+0.5）
        if (t.isRed) tileDoraCount++;
        // 2. 若花色/点数匹配宝牌，再计1番（+0.5）
        if (t.suit == doraTile.suit) {
            uint8_t tVal = (t.value == 0) ? 5 : t.value;
            if (tVal == dVal) tileDoraCount++;
        }
        doraBonus += tileDoraCount * 0.5;
    }

    int allPts = calculateBasePoints(played);
    int first5Pts = 0; int lim = std::min((int)playOrder.size(), 5);
    for(int i=0;i<lim;++i){uint8_t v=(playOrder[i].value==0)?5:playOrder[i].value; first5Pts+=(playOrder[i].suit==TileSuit::ZI||v==1||v==9)?15:10;}

    if (!hasYaku) {
        res.activeYakus = {YakuType::NoYaku};
        res.basePoints = isWin ? allPts : first5Pts;
        res.totalFan = (0.5 + doraBonus) * (isWin ? 3.0 : 1.0);
    } else {
        res.basePoints = allPts;
        res.totalFan = (yakuSum + 0.5 + doraBonus) * (isWin ? 3.0 : 1.0);
    }
    res.isWinHand = isWin;
    res.finalScore = static_cast<int>(res.basePoints * res.totalFan);
    return res;
}

// 🔹 番型名称映射（调试用）
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
    case YakuType::guoshi: return "国士无双";
    default: return "未知番型";
    }
}
