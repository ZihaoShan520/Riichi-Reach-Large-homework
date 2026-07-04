// gamemanager.cpp
#include "gamemanager.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QStringList>

GameManager::GameManager(QObject* parent) : QObject(parent), currentMoney(0) {}

QString GameManager::windToString(int windVal) {
    static const QString names[] = {"", "東", "南", "西", "北"};
    return (windVal >= 1 && windVal <= 4) ? names[windVal] : "?";
}

Tile GameManager::computeNextDora(const Tile& ind) {
    Tile d = ind;
    d.isRed = false;
    uint8_t v = (ind.value == 0) ? 5 : ind.value;
    if (d.suit == TileSuit::ZI) {
        if (v >= 1 && v <= 3) d.value = v + 1;
        else if (v == 4) d.value = 1;
        else if (v == 5) d.value = 6;
        else if (v == 6) d.value = 7;
        else if (v == 7) d.value = 5;
    } else {
        d.value = (v % 9) + 1;
    }
    qDebug() << "[🀄 DORA MAP] 指示物[" << ind.id() << "] → 宝牌[" << d.id()
             << "] | Suit:" << (int)d.suit << " Val:" << (int)d.value;
    return d;
}

void GameManager::startLevel() {
    resetSessionStats();
    clearLevelBonuses();
    currentTier = 1; currentLevelInTier = 1; infiniteMode = false;
    baseTierPrevWind = QRandomGenerator::global()->bounded(1, 5);
    currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
    loadLevelConfig();
}

void GameManager::loadLevelConfig() {
    playCount = 4 + bonusPlayCount;
    discardCount = 3 + bonusDiscardCount;
    currentScore = 0;

    // 🔹 目标分数配置（保持原有）
    if (currentTier == 1) {
        if (currentLevelInTier == 1) targetScore = 600;
        else if (currentLevelInTier == 2) targetScore = 1000;
        else if (currentLevelInTier == 3) targetScore = 1500;
        else if (currentLevelInTier == 4) targetScore = 2800;
    } else if (currentTier == 2) {
        if (currentLevelInTier == 1) targetScore = 4600;
        else if (currentLevelInTier == 2) targetScore = 6700;
        else if (currentLevelInTier == 3) targetScore = 9500;
        else if (currentLevelInTier == 4) targetScore = 10600;
    } else if (currentTier == 3) {
        if (currentLevelInTier == 1) targetScore = 16500;
        else if (currentLevelInTier == 2) targetScore = 22000;
        else if (currentLevelInTier == 3) targetScore = 28000;
        else if (currentLevelInTier == 4) targetScore = 32000;
    } else if (currentTier == 4) {
        if (currentLevelInTier == 1) targetScore = 45000;
        else if (currentLevelInTier == 2) targetScore = 52000;
        else if (currentLevelInTier == 3) targetScore = 76000;
        else if (currentLevelInTier == 4) targetScore = 100000;
    }

    if (infiniteMode) {
        targetScore += (currentTier - 1) * 300;
        playCount = qMax(2, playCount - (currentTier - 1) / 2);
    }

    handMgr.initDeck();
    if (handMgr.canDraw(1)) {
        currentDoraIndicator = handMgr.drawTiles(1)[0];
        currentDoraTile = computeNextDora(currentDoraIndicator);
        emit doraInfoUpdated(currentDoraIndicator, currentDoraTile);
    }

    int initDraw = qMin(18, handMgr.wallSize());
    for (int i = 0; i < initDraw; ++i) handMgr.drawTiles(1);

    calculateCurrentWinds();
    updateUIStates();

    // 🔹 重置并初始化本关统计
    resetLevelStats(currentTier, currentLevelInTier);

    emit levelInfoUpdated(currentTier, currentLevelInTier, currentPrevalentWind, currentSeatWind, infiniteMode);
    emit levelStarted();

    qDebug() << "[🎮 LEVEL LOAD] T" << currentTier << "L" << currentLevelInTier
             << "| Target:" << targetScore
             << "| Plays:" << playCount << "| Discards:" << discardCount;
}

void GameManager::calculateCurrentWinds() {
    currentPrevalentWind = ((currentTier - 1) % 4) + 1;
    currentSeatWind = ((currentLevelInTier - 1) % 4) + 1;
}

// 🔹 【新增】记录高番型（3 番+）
void GameManager::recordHighYaku(YakuType yaku) {
    auto getFan = [](YakuType y) -> int {
        switch (y) {
        case YakuType::SequentialSix: case YakuType::AllSimples:
        case YakuType::DragonPung: case YakuType::PrevalentWind:
        case YakuType::SeatWind: case YakuType::PureDoubleSequence: return 1;
        case YakuType::FourPairs: case YakuType::TwoConcealedPungs:
        case YakuType::FourIdentical: case YakuType::SmallThreeDragons:
        case YakuType::NotBreaking: case YakuType::PureStraight:
        case YakuType::MixedTripleSequence: case YakuType::AllTerminals:
        case YakuType::ThreeConcealedPungs: case YakuType::FiveFamilies: return 2;
        case YakuType::SevenPairs: case YakuType::TripleTriplets:
        case YakuType::MixedTerminalHonors: case YakuType::TwoPureDoubleSequences: return 3;
        case YakuType::FullFlush: case YakuType::PureTripleSequence: return 5;
        case YakuType::AllHonors: case YakuType::BigThreeDragons:
        case YakuType::FourConcealedPungs: case YakuType::FullGreen:
        case YakuType::FourWinds: case YakuType::NineGates:
        case YakuType::MillionStone: case YakuType::OnePointRed: return 13;
        default: return 0;
        }
    };
    if (getFan(yaku) >= 3) {
        currentLevelStats.highYakus[yaku]++;
    }
}

// 🔹 【新增】记录本关获得的能力描述
void GameManager::recordAbility(const QString& desc) {
    if (!desc.isEmpty()) {
        currentLevelStats.abilitiesGained.append(desc);  // ✅ 使用 append 而非 =
        qDebug() << "[✨ ABILITY RECORDED]" << desc
                 << "| Total abilities:" << currentLevelStats.abilitiesGained.size();
    }
}

void GameManager::finalizeLevelStats() {
    // 🔹 累积本关高番型到全局
    for (const auto& [yaku, count] : currentLevelStats.highYakus) {
        sessionHighYakus[yaku] += count;
    }

    currentLevelStats.finalScore = currentScore;
    currentLevelStats.playsLeft = playCount;
    currentLevelStats.discardsLeft = discardCount;
    lastLevelStats = currentLevelStats;
}

// 🔹 【新增】新关卡开始时重置统计
void GameManager::resetLevelStats(int tier, int level) {
    currentLevelStats = LevelStats{tier, level, 0, playCount, discardCount, {}, {}};
}

void GameManager::advanceLevel() {
    currentLevelInTier++;
    if (currentLevelInTier > 4) {
        currentTier++;
        currentLevelInTier = 1;
        if (currentTier > 4) {
            infiniteMode = true;
            currentTier = 1;
            currentLevelInTier = 1;
        }
    }
    loadLevelConfig();  // loadLevelConfig 内会调用 resetLevelStats
}

void GameManager::handleFailure() {
    resetSessionStats();
    qDebug() << "[💀 FAILURE] Resetting to Tier 1 Level 1...";
    clearLevelBonuses();
    currentTier = 1; currentLevelInTier = 1; infiniteMode = false;
    loadLevelConfig();
    emit gameOver();
}

void GameManager::checkLevelEnd() {
    if (currentScore >= targetScore) {
        qDebug() << "[✅ SUCCESS] Level Cleared!";

        // 🔹 Boss 关处理（保持原有）
        if (currentLevelInTier == 4) {
            if (currentTier >= 4) {
                qDebug() << "[🎉 GAME CLEARED!] All bosses defeated!";
                awardTierClearReward();
                emit gameCleared();
                return;
            }
        }

        // 🔹 普通关卡通关奖励
        awardTierClearReward();

        // 🔹 【关键修复 1】记录本关获得的能力（在 finalize 之前）
        if (currentTier == 1 && currentLevelInTier == 1) {
            bonusPlayCount += 1;
            recordAbility("出牌次数 +1");
        }
        else if (currentTier == 1 && currentLevelInTier == 2) {
            bonusDiscardCount += 1;
            recordAbility("弃牌次数 +1");
        }
        else if (currentTier == 1 && currentLevelInTier == 3) {
            bonusBaseScore += 100;
            recordAbility("基础分数 +100");
        }
        else if (currentTier == 1 && currentLevelInTier == 4) {
            bonusDoraFan += 2.0;
            recordAbility("宝牌/红宝牌番数 +2");
        }
        else if (currentTier == 2 && currentLevelInTier == 1) {
            bonusDoubleOneFanYaku = true;
            recordAbility("1 番役种额外结算一次");
        }
        else if (currentTier == 2 && currentLevelInTier == 2) {
            bonusDiscardToFanScore = true;
            recordAbility("每剩 1 弃牌：+1 番 +50 分");
        }
        else if (currentTier == 2 && currentLevelInTier == 4) {
            bonusTotalFan += 5;
            recordAbility("获得番数 +5");
        }
        else if (currentTier == 3 && currentLevelInTier == 2) {
            bonusPlayCount += 2; bonusDiscardCount += 1;
            recordAbility("出牌次数 +2 / 弃牌次数 +1");
        }
        else if (currentTier == 3 && currentLevelInTier == 4) {
            bonusAllSameSuitAsDora = true;
            recordAbility("同花色牌均视为宝牌");
        }
        else if (currentTier == 4 && currentLevelInTier == 2) {
            bonusFanMultiplier = 3.0;
            recordAbility("获得番数 ×3");
        }

        // 🔹 【关键修复 2】固化统计（必须在 currentScore 更新后调用！）
        // 注意：currentScore 已在 tryPlay 中累加，此处直接使用
        finalizeLevelStats();

        // 🔹 调试日志：验证数据
        qDebug() << "[📊 STATS FINALIZED] Score:" << currentLevelStats.finalScore
                 << " PlaysLeft:" << currentLevelStats.playsLeft
                 << " DiscardsLeft:" << currentLevelStats.discardsLeft
                 << " HighYakus:" << currentLevelStats.highYakus.size()
                 << " Abilities:" << currentLevelStats.abilitiesGained;

        emit levelCleared();
        advanceLevel();
    }
    else if (playCount <= 0) {
        qDebug() << "[❌ FAIL] Out of plays!";
        handleFailure();
    }
}

void GameManager::awardTierClearReward() {
    int baseReward = 4 + currentTier;
    int playReward = playCount * 2;
    int scoreBonus = (currentScore >= targetScore * 2) ? baseReward : 0;
    int totalReward = baseReward + playReward + scoreBonus;

    currentMoney += totalReward;
    emit moneyUpdated(currentMoney);

    qDebug() << "💰 [PASS REWARD] Tier:" << currentTier
             << "| 基础:" << baseReward << "| 剩余次数:" << playReward
             << "| 得分翻倍:" << scoreBonus << "| 本次:" << totalReward
             << "| 总金钱:" << currentMoney;
}

bool GameManager::tryDiscard(const std::vector<Tile>& selected) {
    if (discardCount <= 0 || !handMgr.canDiscard(selected.size())) return false;
    handMgr.removeTiles(selected); handMgr.addToDiscard(selected);
    if (handMgr.canDraw(selected.size())) handMgr.drawTiles(selected.size());
    --discardCount; updateUIStates();
    qDebug() << "[🗑️ DISCARD]" << selected.size() << "tiles | Left:" << discardCount;
    return true;
}

bool GameManager::tryPlay(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
    if (playCount <= 0 || !handMgr.canPlay(playedSet.size())) return false;

    handMgr.removeTiles(playedSet);

    ScoreResult result = YakuCalculator::calculateScore(
        playedSet, playOrder,
        static_cast<uint8_t>(currentPrevalentWind),
        static_cast<uint8_t>(currentSeatWind),
        currentDoraTile
        );

    // 🔹 填充 UI 字段
    result.doraCount = 0;
    uint8_t dVal = (currentDoraTile.value == 0) ? 5 : currentDoraTile.value;
    for (const auto& t : playedSet) {
        if (t.isRed) result.doraCount++;
        else if (t.suit == currentDoraTile.suit) {
            uint8_t tVal = (t.value == 0) ? 5 : t.value;
            if (tVal == dVal) result.doraCount++;
        }
    }
    result.doraFan = result.doraCount * 0.5;
    result.yakuFan = result.totalFan - 0.5 - result.doraFan;
    result.hasYaku = !result.activeYakus.empty() || result.activeYakus[0] != YakuType::NoYaku;
    result.countedTiles = (!result.hasYaku && !result.isWinHand) ?
                              std::min((int)playOrder.size(), 5) : (int)playedSet.size();

    // 🔹 应用 Bonus 效果（保持原有逻辑）
    result.basePoints += bonusBaseScore;
    if (bonusAllSameSuitAsDora) {
        double oldDoraFan = result.doraFan;
        int sameSuitCnt = 0;
        for (const auto& t : playedSet) if (t.suit == currentDoraTile.suit) sameSuitCnt++;
        result.doraFan = sameSuitCnt * 0.5;
        result.doraCount = sameSuitCnt;
        result.totalFan += (result.doraFan - oldDoraFan);
    }
    result.doraFan += bonusDoraFan;
    result.totalFan += bonusDoraFan;
    if (bonusDoubleOneFanYaku) {
        std::vector<YakuType> oneFanTypes = {
            YakuType::SequentialSix, YakuType::AllSimples, YakuType::DragonPung,
            YakuType::PrevalentWind, YakuType::SeatWind, YakuType::PureDoubleSequence,
            YakuType::TwoConcealedPungs
        };
        double extraFan = 0;
        for (YakuType y : result.activeYakus) {
            for (YakuType of : oneFanTypes) if (y == of) { extraFan += 1.0; break; }
        }
        result.totalFan += extraFan;
        result.yakuFan += extraFan;
    }
    result.totalFan += bonusTotalFan;
    int discardBonusFan = bonusDiscardToFanScore ? discardCount * 1 : 0;
    int discardBonusFlat = bonusDiscardToFanScore ? discardCount * 50 : 0;
    result.totalFan += discardBonusFan;
    result.totalFan *= bonusFanMultiplier;

    int finalScore = static_cast<int>((result.basePoints + discardBonusFlat) * result.totalFan);
    lastPlayScore = finalScore;
    if (finalScore > maxSinglePlayScore) maxSinglePlayScore = finalScore;
    currentScore += finalScore;
    result.finalScore = finalScore;

    handMgr.addToDiscard(playedSet);
    if (handMgr.canDraw(playedSet.size())) handMgr.drawTiles(playedSet.size());

    --playCount;
    updateUIStates();

    // 🔹 【关键修复】记录高番型（3 番+）
    for (YakuType y : result.activeYakus) {
        if (y != YakuType::NoYaku) {
            recordHighYaku(y);  // ✅ 确保调用
            qDebug() << "[🏆 RECORDED YAKU]" << YakuCalculator::yakuName(y);
        }
    }

    lastPlayResult = result;
    lastPlayedTiles = playedSet;

    emit scorePopupRequested(result, playedSet, handMgr.getHand());
    emit levelCheckRequested();

    qDebug() << "[🀄 PLAY] +" << finalScore << "pts | Total:" << currentScore;
    return true;
}

static QString tileToString(const Tile& t) {
    if (t.isRed) return t.suit==TileSuit::MAN?"赤五萬":t.suit==TileSuit::PIN?"赤五筒":"赤五索";
    static const QString s[] = {"","一萬","二萬","三萬","四萬","五萬","六萬","七萬","八萬","九萬",
                                "一筒","二筒","三筒","四筒","五筒","六筒","七筒","八筒","九筒",
                                "一索","二索","三索","四索","五索","六索","七索","八索","九索",
                                "東","南","西","北","白","發","中"};
    int idx = (t.suit==TileSuit::MAN?t.value:(t.suit==TileSuit::PIN?t.value+9:(t.suit==TileSuit::SOU?t.value+18:t.value+27)));
    return (idx>0&&idx<35)?s[idx]:t.id();
}
static int tileBasePoint(const Tile& t) { uint8_t v=(t.value==0)?5:t.value; return (t.suit==TileSuit::ZI||v==1||v==9)?15:10; }

int GameManager::calculatePlayScore(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
    qDebug() << "\n🀄 [SCORE START] | Dora:" << currentDoraTile.id();
    QStringList tileList; int totalBasePoints = 0;
    for (const auto& t : playedSet) { int p = tileBasePoint(t); totalBasePoints += p; tileList.append(QString("%1(%2pt)").arg(tileToString(t)).arg(p)); }
    qDebug() << "📦 Played (" << playedSet.size() << "): [" << tileList.join(", ") << "] | Base:" << totalBasePoints;
    bool isWin = YakuCalculator::checkWinHand(playedSet);
    qDebug() << "🎯 Win Hand:" << (isWin ? "✅ YES" : "❌ NO");
    ScoreResult result = YakuCalculator::calculateScore(playedSet, playOrder,
                                                        static_cast<uint8_t>(currentPrevalentWind),
                                                        static_cast<uint8_t>(currentSeatWind),
                                                        currentDoraTile);
    qDebug() << "\n🔍 Active Yakus:";
    if (result.activeYakus.empty()) { qDebug() << "   [无役]"; }
    else { for (YakuType y : result.activeYakus) qDebug() << "   •" << YakuCalculator::yakuName(y); }
    qDebug() << "💰 FINAL: " << result.finalScore << " pts | Total Fan:" << result.totalFan << "\n" << QString(60, '=');
    return result.finalScore;
}

void GameManager::updateUIStates() {
    emit actionsUpdated(playCount, discardCount); emit handUpdated();
    emit wallSizeUpdated(handMgr.wallSize()); emit scoreUpdated(currentScore, targetScore);
}

void GameManager::applyLevelClearBonus() {
    playCount += bonusPlayCount;
    discardCount += bonusDiscardCount;
    qDebug() << "[🎁 LEVEL CLEAR BONUS] Applied:"
             << "| Score+" << bonusTotalScore
             << "| Fan+" << bonusTotalFan
             << "| DoraFan+" << bonusDoraFan
             << "| PlayCount+" << bonusPlayCount
             << "| DiscardCount+" << bonusDiscardCount;
}

void GameManager::clearLevelBonuses() {
    bonusTotalScore = 0; bonusTotalFan = 0; bonusDoraFan = 0;
    bonusPlayCount = 0; bonusDiscardCount = 0;
    bonusBaseScore = 0; bonusDoubleOneFanYaku = false;
    bonusDiscardToFanScore = false; bonusAllSameSuitAsDora = false;
    bonusFanMultiplier = 1.0;
    qDebug() << "[🔄 LEVEL BONUSES CLEARED]";
}
