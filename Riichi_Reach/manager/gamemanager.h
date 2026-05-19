// gamemanager.h
#pragma once
#include <QObject>
#include <vector>
#include "../core/tile.h"
#include "../core/hand_manager.h"
#include "../core/yakucalculator.h"  // ✅ 新增：引入算分引擎

class GameManager : public QObject {
    Q_OBJECT
public:
    explicit GameManager(QObject* parent = nullptr);

    void startLevel();
    void startGame();
    bool tryDiscard(const std::vector<Tile>& selected);
    bool tryPlay(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);  // ✅ 双参数

    int getScore() const { return currentScore; }
    int getTargetScore() const { return targetScore; }
    int getPlaysLeft() const { return playCount; }
    int getDiscardsLeft() const { return discardCount; }
    int getLevel() const { return currentLevel; }
    const std::vector<Tile>& getHand() const { return handMgr.getHand(); }

signals:
    void levelStarted();
    void actionsUpdated(int playsLeft, int discardsLeft);
    void handUpdated();
    void wallSizeUpdated(int size);
    void scoreUpdated(int current, int target);
    void levelCleared();
    void gameOver();
    void levelInfoUpdated(int tier, int level, int prevWind, int seatWind, bool infinite);

private:
    HandManager handMgr;
    int playCount = 4;
    int discardCount = 3;
    int currentScore = 0;
    int targetScore = 2000;
    int currentLevel = 1;

    void updateUIStates();
    void checkLevelEnd();
    int calculatePlayScore(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);  // ✅ 双参数

    int currentTier = 1;
    int currentLevelInTier = 1;
    int currentPrevalentWind = 1; // 1=东, 2=南, 3=西, 4=北
    int currentSeatWind = 1;
    bool infiniteMode = false;

    int baseTierPrevWind = 1;      // 第一层初始场风种子
    int currentTierBaseSeatWind = 1; // 当前层初始自风种子

    void calculateCurrentWinds();
    void loadLevelConfig();
    void advanceLevel();
    void handleFailure();
    static QString windToString(int windVal); // UI辅助
};
