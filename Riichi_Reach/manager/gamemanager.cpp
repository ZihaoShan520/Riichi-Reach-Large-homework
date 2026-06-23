// gamemanager.cpp
#include "gamemanager.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QStringList>

GameManager::GameManager(QObject* parent) : QObject(parent) {}

QString GameManager::windToString(int windVal) {
    static const QString names[] = {"", "東", "南", "西", "北"};
    return (windVal >= 1 && windVal <= 4) ? names[windVal] : "?";
}

// 🔹 【严格修复】宝牌推算逻辑（完全隔离花色，精准映射）
Tile GameManager::computeNextDora(const Tile& ind) {
    Tile d = ind;          // ✅ 完整继承 suit，杜绝跨花色
    d.isRed = false;       // 宝牌指示物绝不带赤宝标记

    uint8_t v = (ind.value == 0) ? 5 : ind.value; // 赤宝按5参与推算

    if (d.suit == TileSuit::ZI) {
        if (v >= 1 && v <= 3) d.value = v + 1; // 1东→2南, 2南→3西, 3西→4北
        else if (v == 4)      d.value = 1;     // 4北→1东
        else if (v == 5)      d.value = 6;     // 5白→6發
        else if (v == 6)      d.value = 7;     // 6發→7中
        else if (v == 7)      d.value = 5;     // 7中→5白
    } else {
        d.value = (v % 9) + 1; // 数牌 1→2 ... 9→1
    }

    qDebug() << "[🀄 DORA MAP] 指示物[" << ind.id() << "] → 宝牌[" << d.id()
             << "] | Suit:" << (int)d.suit << " Val:" << (int)d.value;
    return d;
}

void GameManager::startLevel() {
    currentTier = 1; currentLevelInTier = 1; infiniteMode = false;
    baseTierPrevWind = QRandomGenerator::global()->bounded(1, 5);
    currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
    loadLevelConfig();
}

void GameManager::loadLevelConfig() {
    playCount = 4; discardCount = 3; currentScore = 0; targetScore = 2000;
    if (infiniteMode) { targetScore += (currentTier - 1) * 300; playCount = qMax(2, playCount - (currentTier - 1) / 2); }
    handMgr.initDeck();

    if (handMgr.canDraw(1)) {
        currentDoraIndicator = handMgr.drawTiles(1)[0];
        currentDoraTile = computeNextDora(currentDoraIndicator);
        emit doraInfoUpdated(currentDoraIndicator, currentDoraTile);
    }

    int initDraw = qMin(18, handMgr.wallSize());
    for (int i = 0; i < initDraw; ++i) handMgr.drawTiles(1);

    calculateCurrentWinds(); updateUIStates();
    emit levelInfoUpdated(currentTier, currentLevelInTier, currentPrevalentWind, currentSeatWind, infiniteMode);
    emit levelStarted();
    qDebug() << "[🎮 LEVEL LOAD] T" << currentTier << "L" << currentLevelInTier << "| Target:" << targetScore;
}

void GameManager::calculateCurrentWinds() {
    auto getWind = [](int base, int offset) { return ((base - 1 + offset) % 4) + 1; };
    currentPrevalentWind = getWind(baseTierPrevWind, currentTier - 1);
    currentSeatWind = getWind(currentTierBaseSeatWind, currentLevelInTier - 1);
}

void GameManager::advanceLevel() {
    currentLevelInTier++;
    if (currentLevelInTier > 4) {
        currentTier++; currentLevelInTier = 1;
        currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
        if (currentTier > 4) { infiniteMode = true; currentTier = 1; currentLevelInTier = 1; }
    }
    loadLevelConfig();
}

void GameManager::handleFailure() {
    qDebug() << "[💀 FAILURE] Resetting to Tier 1 Level 1...";
    currentTier = 1; currentLevelInTier = 1; infiniteMode = false;
    baseTierPrevWind = QRandomGenerator::global()->bounded(1, 5);
    currentTierBaseSeatWind = QRandomGenerator::global()->bounded(1, 5);
    loadLevelConfig(); emit gameOver();
}

void GameManager::checkLevelEnd() {
    if (currentScore >= targetScore) { qDebug() << "[✅ SUCCESS] Level Cleared!"; emit levelCleared(); advanceLevel(); }
    else if (playCount <= 0) { qDebug() << "[❌ FAIL] Out of plays!"; handleFailure(); }
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
    int pts = calculatePlayScore(playedSet, playOrder);
    currentScore += pts;
    handMgr.addToDiscard(playedSet);
    if (handMgr.canDraw(playedSet.size())) handMgr.drawTiles(playedSet.size());
    --playCount; updateUIStates(); checkLevelEnd();
    qDebug() << "[🀄 PLAY] +" << pts << "pts | Total:" << currentScore;
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

    // 传入宝牌进行计算
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
