#pragma once
#include <QObject>
#include <vector>
#include "../core/tile.h"
#include "../core/hand_manager.h"

class GameManager : public QObject {
    Q_OBJECT
public:
    explicit GameManager(QObject* parent = nullptr);

    void startGame();
    void drawTile();
    void playTiles(const std::vector<Tile>& selected);
    void skipTurn();
    // manager/gamemanager.h 的 public 区域追加：
    int getScore() const { return currentScore; }
    int getTargetScore() const { return targetScore; }
    int getTurnsLeft() const { return turnsLeft; }
    int getFloor() const { return currentFloor; }
    const std::vector<Tile>& getHand() const { return handMgr.getHand(); }
signals:
    void gameStarted();
    void tileDrawn(Tile tile);
    void handChanged();
    void scoreUpdated(int current, int target);
    void turnsUpdated(int left);
    void floorChanged(int currentFloor);
    void floorCleared();
    void gameOver(bool success);

private:
    HandManager handMgr;
    int currentScore = 0;
    int targetScore = 1000;
    int turnsLeft = 10;
    int currentFloor = 1;

    void checkFloorCondition();
    void nextFloor();
};
