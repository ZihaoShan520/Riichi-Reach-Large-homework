// gamemanager.cpp
#include "gamemanager.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QStringList>

GameManager::GameManager(QObject* parent) : QObject(parent) {}

// 🔹 风值转中文名称
QString GameManager::windToString(int windVal) {
    static const QString names[] = {"", "東", "南", "西", "北"};
    return (windVal >= 1 && windVal <= 4) ? names[windVal] : "?";
}

// 🔹 单张牌转详细字符串（用于日志）
static QString tileToString(const Tile& t) {
    if (t.isRed) {
        if (t.suit == TileSuit::MAN) return "赤五萬";
        if (t.suit == TileSuit::PIN) return "赤五筒";
        if (t.suit == TileSuit::SOU) return "赤五索";
    }
    static const QString man[] = {"","一萬","二萬","三萬","四萬","五萬","六萬","七萬","八萬","九萬"};
    static const QString pin[] = {"","一筒","二筒","三筒","四筒","五筒","六筒","七筒","八筒","九筒"};
    static const QString sou[] = {"","一索","二索","三索","四索","五索","六索","七索","八索","九索"};
    static const QString zi[]  = {"","東","南","西","北","白","發","中"};

    if (t.suit == TileSuit::MAN && t.value >= 1 && t.value <= 9) return man[t.value];
    if (t.suit == TileSuit::PIN && t.value >= 1 && t.value <= 9) return pin[t.value];
    if (t.suit == TileSuit::SOU && t.value >= 1 && t.value <= 9) return sou[t.value];
    if (t.suit == TileSuit::ZI  && t.value >= 1 && t.value <= 7)  return zi[t.value];
    return t.id();
}

// 🔹 计算单张牌基础点数
static int tileBasePoint(const Tile& t) {
    uint8_t v = (t.value == 0) ? 5 : t.value;
    return (t.suit == TileSuit::ZI || v == 1 || v == 9) ? 15 : 10;
}

// 🔹 游戏启动入口
void GameManager::startLevel() {
    currentTier = 1;
    currentLevelInTier = 1;
    infiniteMode = false;
    baseTierPrevWind = QRandomGenerator::global()->bounded(1, 5);
    currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
    loadLevelConfig();
}

// 🔹 加载当前关卡配置
void GameManager::loadLevelConfig() {
    playCount = 4;
    discardCount = 3;
    currentScore = 0;
    targetScore = 2000;

    if (infiniteMode) {
        targetScore += (currentTier - 1) * 300;
        playCount = qMax(2, playCount - (currentTier - 1) / 2);
    }

    handMgr.initDeck();
    int initDraw = qMin(18, handMgr.wallSize());
    for (int i = 0; i < initDraw; ++i) handMgr.drawTiles(1);

    calculateCurrentWinds();
    updateUIStates();
    emit levelInfoUpdated(currentTier, currentLevelInTier,
                          currentPrevalentWind, currentSeatWind,
                          infiniteMode);
    emit levelStarted();

    qDebug() << "[🎮 LEVEL LOAD] T" << currentTier << "L" << currentLevelInTier
             << "| Prev:" << windToString(currentPrevalentWind)
             << "| Seat:" << windToString(currentSeatWind)
             << "| Target:" << targetScore;
}

void GameManager::calculateCurrentWinds() {
    auto getWind = [](int base, int offset) { return ((base - 1 + offset) % 4) + 1; };
    currentPrevalentWind = getWind(baseTierPrevWind, currentTier - 1);
    currentSeatWind = getWind(currentTierBaseSeatWind, currentLevelInTier - 1);
}

void GameManager::advanceLevel() {
    currentLevelInTier++;
    if (currentLevelInTier > 4) {
        currentTier++;
        currentLevelInTier = 1;
        currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
        if (currentTier > 4) {
            infiniteMode = true;
            currentTier = 1;
            currentLevelInTier = 1;
            qDebug() << "[🌟 INFINITE MODE ACTIVATED]";
        }
    }
    loadLevelConfig();
}

void GameManager::handleFailure() {
    qDebug() << "[💀 FAILURE] Resetting to Tier 1 Level 1...";
    currentTier = 1;
    currentLevelInTier = 1;
    infiniteMode = false;
    baseTierPrevWind = QRandomGenerator::global()->bounded(1, 5);
    currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
    loadLevelConfig();
    emit gameOver();
}

void GameManager::checkLevelEnd() {
    if (currentScore >= targetScore) {
        qDebug() << "[✅ SUCCESS] Level Cleared! Score:" << currentScore;
        emit levelCleared();
        advanceLevel();
    } else if (playCount <= 0) {
        qDebug() << "[❌ FAIL] Out of plays! Final Score:" << currentScore;
        handleFailure();
    }
}

// 🔹 弃牌操作
bool GameManager::tryDiscard(const std::vector<Tile>& selected) {
    if (discardCount <= 0) { qDebug() << "[WARN] No discards left!"; return false; }
    if (!handMgr.canDiscard(selected.size())) { qDebug() << "[WARN] Invalid discard count!"; return false; }

    handMgr.removeTiles(selected);
    handMgr.addToDiscard(selected);

    int drawBack = selected.size();
    if (handMgr.canDraw(drawBack)) handMgr.drawTiles(drawBack);

    --discardCount;
    updateUIStates();
    qDebug() << "[🗑️ DISCARD]" << selected.size() << "tiles | Left:" << discardCount;
    return true;
}

// 🔹 【核心修复】出牌操作 - 之前遗漏的函数
bool GameManager::tryPlay(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
    if (playCount <= 0) {
        qDebug() << "[WARN] No plays left!";
        return false;
    }
    if (!handMgr.canPlay(playedSet.size())) {
        qDebug() << "[WARN] Invalid play count (need 8~14)!";
        return false;
    }

    // 1. 移除手牌
    handMgr.removeTiles(playedSet);

    // 2. 🔹 调用算分引擎（传入风场参数！）
    int pts = calculatePlayScore(playedSet, playOrder);
    currentScore += pts;

    // 3. 加入弃牌堆并摸回等量牌
    handMgr.addToDiscard(playedSet);
    int drawBack = playedSet.size();
    if (handMgr.canDraw(drawBack)) {
        handMgr.drawTiles(drawBack);
    }

    // 4. 消耗出牌次数并检查关卡
    --playCount;
    updateUIStates();
    checkLevelEnd();

    qDebug() << "[🀄 PLAY]" << playedSet.size() << "tiles | +" << pts << "pts | Total:" << currentScore;
    return true;
}

// 🔹 核心算分函数（带详细日志）
int GameManager::calculatePlayScore(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
    // ========== 📋 日志头 ==========
    qDebug() << "\n" << QString(60, '=');
    qDebug() << "🀄 [SCORE CALCULATION START]";
    qDebug() << QString(60, '=');

    // 1️⃣ 显示打出牌列表
    qDebug() << "\n📦 Played Tiles (" << playedSet.size() << "):";
    QStringList tileList;
    int totalBasePoints = 0;
    for (const auto& t : playedSet) {
        int pts = tileBasePoint(t);
        totalBasePoints += pts;
        tileList.append(QString("%1(%2pt)").arg(tileToString(t)).arg(pts));
    }
    qDebug() << "   [" << tileList.join(", ") << "]";
    qDebug() << "📊 Base Points Sum: " << totalBasePoints;

    // 2️⃣ 和牌判定
    bool isWin = YakuCalculator::checkWinHand(playedSet);
    qDebug() << "\n🎯 Win Hand Check: " << (isWin ? "✅ YES (×3 multiplier)" : "❌ NO");

    // 3️⃣ 番型检测（原始列表）
    qDebug() << "\n🔍 Detecting Yakus (before exclusions):";
    std::vector<YakuResult> detected;

    // 1番系
    if (YakuCalculator::checkSequentialSix(playedSet)) {
        detected.push_back({YakuType::SequentialSix, 1, "连六"});
        qDebug() << "   ✅ [1 fan] 连六";
    }
    if (YakuCalculator::checkAllSimples(playedSet)) {
        detected.push_back({YakuType::AllSimples, 1, "断幺"});
        qDebug() << "   ✅ [1 fan] 断幺";
    }
    int dragonCnt = 0;
    if (YakuCalculator::checkDragonPung(playedSet, dragonCnt) && dragonCnt > 0) {
        detected.push_back({YakuType::DragonPung, dragonCnt, "三元牌"});
        qDebug() << "   ✅ [1 fan×" << dragonCnt << "] 三元牌";
    }
    if (YakuCalculator::checkWindPung(playedSet, currentPrevalentWind)) {
        detected.push_back({YakuType::PrevalentWind, 1, "场风牌"});
        qDebug() << "   ✅ [1 fan] 场风牌(" << windToString(currentPrevalentWind) << ")";
    }
    if (YakuCalculator::checkWindPung(playedSet, currentSeatWind)) {
        detected.push_back({YakuType::SeatWind, 1, "门风牌"});
        qDebug() << "   ✅ [1 fan] 门风牌(" << windToString(currentSeatWind) << ")";
    }
    if (YakuCalculator::checkPureDoubleSequence(playedSet)) {
        detected.push_back({YakuType::PureDoubleSequence, 1, "一杯口"});
        qDebug() << "   ✅ [1 fan] 一杯口";
    }

    // 2番系
    if (YakuCalculator::checkFourPairs(playedSet)) {
        detected.push_back({YakuType::FourPairs, 2, "四对"});
        qDebug() << "   ✅ [2 fan] 四对";
    }
    int pungCnt = 0;
    if (YakuCalculator::checkTwoConcealedPungs(playedSet, pungCnt) && pungCnt >= 2) {
        detected.push_back({YakuType::TwoConcealedPungs, 2, "双暗刻"});
        qDebug() << "   ✅ [2 fan] 双暗刻";
    }
    int quadCnt = 0;
    if (YakuCalculator::checkFourIdentical(playedSet, quadCnt) && quadCnt > 0) {
        detected.push_back({YakuType::FourIdentical, quadCnt * 2, "四归一"});
        qDebug() << "   ✅ [2 fan×" << quadCnt << "] 四归一";
    }
    if (YakuCalculator::checkSmallThreeDragons(playedSet)) {
        detected.push_back({YakuType::SmallThreeDragons, 2, "小三元"});
        qDebug() << "   ✅ [2 fan] 小三元";
    }
    if (YakuCalculator::checkNotBreaking(playedSet)) {
        detected.push_back({YakuType::NotBreaking, 2, "推不倒"});
        qDebug() << "   ✅ [2 fan] 推不倒";
    }
    if (YakuCalculator::checkPureStraight(playedSet)) {
        detected.push_back({YakuType::PureStraight, 2, "一气通贯"});
        qDebug() << "   ✅ [2 fan] 一气通贯";
    }
    if (YakuCalculator::checkMixedTripleSequence(playedSet)) {
        detected.push_back({YakuType::MixedTripleSequence, 2, "三色同顺"});
        qDebug() << "   ✅ [2 fan] 三色同顺";
    }
    if (YakuCalculator::checkAllTerminals(playedSet)) {
        detected.push_back({YakuType::AllTerminals, 2, "全带幺九"});
        qDebug() << "   ✅ [2 fan] 全带幺九";
    }
    if (YakuCalculator::checkThreeConcealedPungs(playedSet, pungCnt) && pungCnt >= 3) {
        detected.push_back({YakuType::ThreeConcealedPungs, 2, "三暗刻"});
        qDebug() << "   ✅ [2 fan] 三暗刻";
    }
    if (YakuCalculator::checkFiveFamilies(playedSet)) {
        detected.push_back({YakuType::FiveFamilies, 2, "五门齐"});
        qDebug() << "   ✅ [2 fan] 五门齐";
    }

    // 3番系
    if (YakuCalculator::checkSevenPairs(playedSet)) {
        detected.push_back({YakuType::SevenPairs, 3, "七对"});
        qDebug() << "   ✅ [3 fan] 七对";
    }
    if (YakuCalculator::checkTripleTriplets(playedSet)) {
        detected.push_back({YakuType::TripleTriplets, 3, "三色同刻"});
        qDebug() << "   ✅ [3 fan] 三色同刻";
    }
    if (YakuCalculator::checkMixedTerminalHonors(playedSet)) {
        detected.push_back({YakuType::MixedTerminalHonors, 3, "混老头"});
        qDebug() << "   ✅ [3 fan] 混老头";
    }
    if (YakuCalculator::checkTwoPureDoubleSequences(playedSet)) {
        detected.push_back({YakuType::TwoPureDoubleSequences, 3, "二杯口"});
        qDebug() << "   ✅ [3 fan] 二杯口";
    }

    // 5番系
    if (YakuCalculator::checkFullFlush(playedSet)) {
        detected.push_back({YakuType::FullFlush, 5, "清一色"});
        qDebug() << "   ✅ [5 fan] 清一色";
    }
    if (YakuCalculator::checkPureTripleSequence(playedSet)) {
        detected.push_back({YakuType::PureTripleSequence, 5, "一色三步高"});
        qDebug() << "   ✅ [5 fan] 一色三步高";
    }

    // 13番系
    if (YakuCalculator::checkAllHonors(playedSet)) {
        detected.push_back({YakuType::AllHonors, 13, "字一色"});
        qDebug() << "   ✅ [13 fan] 字一色";
    }
    if (YakuCalculator::checkBigThreeDragons(playedSet)) {
        detected.push_back({YakuType::BigThreeDragons, 13, "大三元"});
        qDebug() << "   ✅ [13 fan] 大三元";
    }
    if (YakuCalculator::checkFourConcealedPungs(playedSet, pungCnt) && pungCnt >= 4) {
        detected.push_back({YakuType::FourConcealedPungs, 13, "四暗刻"});
        qDebug() << "   ✅ [13 fan] 四暗刻";
    }
    if (YakuCalculator::checkFullGreen(playedSet)) {
        detected.push_back({YakuType::FullGreen, 13, "绿一色"});
        qDebug() << "   ✅ [13 fan] 绿一色";
    }
    if (YakuCalculator::checkFourWinds(playedSet)) {
        detected.push_back({YakuType::FourWinds, 13, "四喜和"});
        qDebug() << "   ✅ [13 fan] 四喜和";
    }
    if (YakuCalculator::checkNineGates(playedSet)) {
        detected.push_back({YakuType::NineGates, 13, "九莲宝灯"});
        qDebug() << "   ✅ [13 fan] 九莲宝灯";
    }
    if (YakuCalculator::checkMillionStone(playedSet)) {
        detected.push_back({YakuType::MillionStone, 13, "百万石"});
        qDebug() << "   ✅ [13 fan] 百万石";
    }
    if (YakuCalculator::checkOnePointRed(playedSet)) {
        detected.push_back({YakuType::OnePointRed, 13, "一点红"});
        qDebug() << "   ✅ [13 fan] 一点红";
    }

    if (detected.empty()) {
        qDebug() << "   ❌ No yakus detected";
    }

    // 4️⃣ 互斥过滤
    qDebug() << "\n🔒 Applying Exclusions (high fan covers low):";
    std::vector<YakuResult> active;
    auto exclMap = YakuCalculator::buildExclusionMap();
    std::unordered_set<YakuType> excluded;

    std::sort(detected.begin(), detected.end(), [](const YakuResult& a, const YakuResult& b) {
        return a.fan > b.fan;
    });

    for (const auto& y : detected) {
        if (excluded.count(y.type)) {
            qDebug() << "   ⛔ SKIP [" << YakuCalculator::yakuName(y.type)
                     << "] (covered by higher yaku)";
            continue;
        }
        active.push_back(y);
        qDebug() << "   ✅ KEEP [" << YakuCalculator::yakuName(y.type)
                 << "] +" << y.fan << " fan";
        if (exclMap.count(y.type)) {
            for (auto ex : exclMap.at(y.type)) {
                excluded.insert(ex);
                qDebug() << "      → Excludes: " << YakuCalculator::yakuName(ex);
            }
        }
    }

    // 5️⃣ 计算番数和 + 基础番数0.5
    double yakuSum = 0;
    for (const auto& y : active) yakuSum += y.fan;
    bool hasYaku = (yakuSum > 0);

    qDebug() << "\n📈 Fan Calculation:";
    if (!hasYaku) {
        qDebug() << "   → No main yaku → Apply [无役] 0.5 fan";
        yakuSum = 0.5;
    } else {
        qDebug() << "   → Yaku sum: " << yakuSum;
        qDebug() << "   → + Base fan 0.5 → Total: " << (yakuSum + 0.5);
        yakuSum += 0.5;
    }

    // 6️⃣ 确定计分范围（无役未和牌仅前5张）
    int finalBasePoints = totalBasePoints;
    if (!hasYaku && !isWin) {
        int limit = qMin((int)playOrder.size(), 5);
        finalBasePoints = 0;
        qDebug() << "\n🎲 No-Yaku + No-Win → Count only first 5 clicked tiles:";
        for (int i = 0; i < limit; ++i) {
            int pts = tileBasePoint(playOrder[i]);
            finalBasePoints += pts;
            qDebug() << "   [" << i+1 << "] " << tileToString(playOrder[i]) << " +" << pts << "pt";
        }
    }

    // 7️⃣ 和牌×3加成
    if (isWin) {
        qDebug() << "\n🏆 Win Hand Bonus: ×3 multiplier";
        yakuSum *= 3.0;
    }

    // 8️⃣ 最终分数
    int finalScore = static_cast<int>(finalBasePoints * yakuSum);

    qDebug() << "\n💰 FINAL SCORE:";
    qDebug() << "   Base Points: " << finalBasePoints;
    qDebug() << "   Total Fan:   " << yakuSum;
    qDebug() << "   ─────────────";
    qDebug() << "   FINAL:       " << finalScore << " points";
    qDebug() << QString(60, '=') << "\n";

    return finalScore;
}

// 🔹 刷新所有UI状态
void GameManager::updateUIStates() {
    emit actionsUpdated(playCount, discardCount);
    emit handUpdated();
    emit wallSizeUpdated(handMgr.wallSize());
    emit scoreUpdated(currentScore, targetScore);
}
