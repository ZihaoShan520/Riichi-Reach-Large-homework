#include "gamemanager.h"
#include <QDebug>

GameManager::GameManager(QObject* parent)
    : QObject(parent)
{}

void GameManager::startGame() {
    currentScore = 0;
    targetScore = 1000;
    turnsLeft = 10;
    currentFloor = 1;

    handMgr.initDeck();

    emit gameStarted();
    emit scoreUpdated(currentScore, targetScore);
    emit turnsUpdated(turnsLeft);
    emit floorChanged(currentFloor);

    // 初始手牌：摸5张
    for (int i = 0; i < 5; ++i) {
        drawTile();
    }
}

void GameManager::drawTile() {
    if (turnsLeft <= 0) return;

    Tile t = handMgr.draw();
    emit tileDrawn(t);
    emit handChanged();

    --turnsLeft;
    emit turnsUpdated(turnsLeft);

    checkFloorCondition();
}

void GameManager::playTiles(const std::vector<Tile>& selected) {
    if (selected.empty()) return;

    if (handMgr.playTiles(selected)) {
        // 🎯 肉鸽简化计分：
        // - 普通牌：每张100分
        // - 赤宝牌：每张300分
        // - 字牌：每张150分
        int pts = 0;
        for (const auto& t : selected) {
            if (t.isRed) {
                pts += 300;
            } else if (t.isYakuhai()) {
                pts += 150;
            } else {
                pts += 100;
            }
        }

        currentScore += pts;
        emit scoreUpdated(currentScore, targetScore);
        emit handChanged();

        qDebug() << "[Play]" << selected.size() << "tiles | +" << pts
                 << "pts | Total:" << currentScore;

        checkFloorCondition();
    } else {
        qDebug() << "[Error] Failed to play tiles (not in hand)";
    }
}

void GameManager::skipTurn() {
    if (turnsLeft <= 0) return;

    --turnsLeft;
    emit turnsUpdated(turnsLeft);

    qDebug() << "[Skip] Turns left:" << turnsLeft;

    checkFloorCondition();
}

void GameManager::checkFloorCondition() {
    if (currentScore >= targetScore) {
        qDebug() << "🎉 Floor" << currentFloor << "Cleared!";
        emit floorCleared();
        nextFloor();
    } else if (turnsLeft <= 0) {
        qDebug() << "💀 Game Over! Final Score:" << currentScore;
        emit gameOver(false);
    }
}

void GameManager::nextFloor() {
    ++currentFloor;
    targetScore += 800; // 每层目标分+800
    turnsLeft = 10;

    // 重置牌山和手牌
    handMgr = HandManager();
    handMgr.initDeck();

    emit floorChanged(currentFloor);
    emit turnsUpdated(turnsLeft);
    emit scoreUpdated(currentScore, targetScore);

    // 初始手牌：摸5张
    for (int i = 0; i < 5; ++i) {
        Tile t = handMgr.draw();
        emit tileDrawn(t);
    }
    emit handChanged();

    qDebug() << "🏔️ Floor" << currentFloor << "Start | Target:" << targetScore;
}
