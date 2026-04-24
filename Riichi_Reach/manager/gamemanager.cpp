// gamemanager.cpp
#include "gamemanager.h"
#include <QDebug>

GameManager::GameManager(QObject* parent) : QObject(parent) {}

void GameManager::startLevel() {
    playCount = 4;
    discardCount = 3;
    currentScore = 0;
    handMgr.initDeck();
    int drawCount = qMin(18, handMgr.wallSize());
    handMgr.drawTiles(drawCount);
    updateUIStates();
    emit levelStarted();
    qDebug() << "[LEVEL START] Plays:4 Discards:3 Hand:" << handMgr.handSize();
}

bool GameManager::tryDiscard(const std::vector<Tile>& selected) {
    if (discardCount <= 0) { qDebug() << "[WARN] No discards left!"; return false; }
    if (!handMgr.canDiscard(selected.size())) { qDebug() << "[WARN] Invalid discard count!"; return false; }
    handMgr.removeTiles(selected);
    handMgr.addToDiscard(selected);
    int drawBack = selected.size();
    if (handMgr.canDraw(drawBack)) handMgr.drawTiles(drawBack);
    --discardCount;
    updateUIStates();
    qDebug() << "[DISCARD]" << selected.size() << "tiles | Left:" << discardCount;
    return true;
}

bool GameManager::tryPlay(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
    if (playCount <= 0) { qDebug() << "[WARN] No plays left!"; return false; }
    if (!handMgr.canPlay(playedSet.size())) { qDebug() << "[WARN] Invalid play count (need 8~14)!"; return false; }

    handMgr.removeTiles(playedSet);

    // 🔹 调用新算分系统（双参数）
    int pts = calculatePlayScore(playedSet, playOrder);
    currentScore += pts;
    qDebug() << "[PLAY]" << playedSet.size() << "tiles | +" << pts << "pts | Total:" << currentScore;

    handMgr.addToDiscard(playedSet);
    int drawBack = playedSet.size();
    if (handMgr.canDraw(drawBack)) handMgr.drawTiles(drawBack);

    --playCount;
    updateUIStates();
    checkLevelEnd();
    return true;
}

// gamemanager.cpp
int GameManager::calculatePlayScore(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
    ScoreResult result = YakuCalculator::calculateScore(playedSet, playOrder);

    // 🔹 调试输出：番型列表
    qDebug() << "\n[🎯 YAKU DEBUG] ================================";
    qDebug() << "Played tiles:" << playedSet.size();
    qDebug() << "Base points:" << result.basePoints;
    qDebug() << "Is Win Hand:" << (result.isWinHand ? "✓ YES" : "✗ NO");

    if (result.activeYakus.empty()) {
        qDebug() << "Active Yakus: [无役]";
    } else {
        qDebug() << "Active Yakus:";
        for (YakuType yaku : result.activeYakus) {
            qDebug() << "  •" << YakuCalculator::yakuName(yaku);
        }
    }

    qDebug() << "Total Fan:" << result.totalFan;
    qDebug() << "Final Score:" << result.finalScore;
    qDebug() << "[🎯 YAKU DEBUG] ================================\n";

    return result.finalScore;
}

void GameManager::updateUIStates() {
    emit actionsUpdated(playCount, discardCount);
    emit handUpdated();
    emit wallSizeUpdated(handMgr.wallSize());
    emit scoreUpdated(currentScore, targetScore);
}

void GameManager::checkLevelEnd() {
    if (currentScore >= targetScore) {
        qDebug() << "[SUCCESS] Level Cleared!";
        emit levelCleared();
    } else if (playCount <= 0) {
        qDebug() << "[FAIL] Out of plays!";
        emit gameOver();
    }
}
