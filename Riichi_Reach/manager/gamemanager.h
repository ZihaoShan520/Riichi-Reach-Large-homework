<<<<<<< HEAD
// gamemanager.h
#pragma once
#include <QObject>
#include <vector>
#include "../core/tile.h"
#include "../core/hand_manager.h"
#include "../core/yakucalculator.h"

class GameManager : public QObject {
    Q_OBJECT
public:
    explicit GameManager(QObject* parent = nullptr);

    void startLevel();
    void startGame();
    bool tryDiscard(const std::vector<Tile>& selected);
    bool tryPlay(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);

    int getScore() const { return currentScore; }
    int getTargetScore() const { return targetScore; }
    int getPlaysLeft() const { return playCount; }
    int getDiscardsLeft() const { return discardCount; }
    int getLevel() const { return currentLevel; }
    const std::vector<Tile>& getHand() const { return handMgr.getHand(); }
    static QString windToString(int windVal);

signals:
    void levelStarted();
    void actionsUpdated(int playsLeft, int discardsLeft);
    void handUpdated();
    void wallSizeUpdated(int size);
    void scoreUpdated(int current, int target);
    void levelCleared();
    void gameOver();
    void levelInfoUpdated(int tier, int level, int prevWind, int seatWind, bool infinite);
    void doraInfoUpdated(Tile indicator, Tile dora);  // 🔹 新增：宝牌信息

private:
    HandManager handMgr;
    int playCount = 4;
    int discardCount = 3;
    int currentScore = 0;
    int targetScore = 2000;
    int currentLevel = 1;

    // 关卡系统
    int currentTier = 1;
    int currentLevelInTier = 1;
    int currentPrevalentWind = 1;  // 1=东,2=南,3=西,4=北
    int currentSeatWind = 1;
    bool infiniteMode = false;
    int baseTierPrevWind = 1;           // 第一层初始场风种子
    int currentTierBaseSeatWind = 1;    // 当前层初始自风种子

    // 🔹 宝牌系统
    Tile currentDoraIndicator;  // 宝牌指示物
    Tile currentDoraTile;       // 本关实际宝牌
    Tile computeNextDora(const Tile& indicator);  // 🔹 推算宝牌

    void calculateCurrentWinds();
    void loadLevelConfig();
    void advanceLevel();
    void handleFailure();
    void updateUIStates();
    void checkLevelEnd();
    int calculatePlayScore(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);
};
=======
// gamemanager.h
#pragma once
#include <QObject>
#include <vector>
#include "../core/tile.h"
#include "../core/hand_manager.h"
#include "../core/yakucalculator.h"

// 🔹 单关统计快照（用于商店展示）
struct LevelStats {
    int tier = 1;
    int level = 1;
    int finalScore = 0;              // 本关总得分
    int playsLeft = 0;               // 通关时剩余出牌数
    int discardsLeft = 0;            // 通关时剩余弃牌数
    std::map<YakuType, int> highYakus;  // 3 番及以上番种及达成次数
    QStringList abilitiesGained;     // 本关获得的能力描述
};

class GameManager : public QObject {
    Q_OBJECT
public:
    explicit GameManager(QObject* parent = nullptr);

    void startLevel();
    void startGame();
    bool tryDiscard(const std::vector<Tile>& selected);
    bool tryPlay(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);

    int getScore() const { return currentScore; }
    int getTargetScore() const { return targetScore; }
    int getPlaysLeft() const { return playCount; }
    int getDiscardsLeft() const { return discardCount; }
    int getLevel() const { return currentLevel; }
    const std::vector<Tile>& getHand() const { return handMgr.getHand(); }
    static QString windToString(int windVal);
    // 🔹 新增：记录本局数据
    int getLastPlayScore() const { return lastPlayScore; }
    int getMaxSinglePlayScore() const { return maxSinglePlayScore; }
    void checkLevelEnd();
    // 💰 新增公开接口
    int getMoney() const { return currentMoney; }
    void resetMoney() { currentMoney = 0; }
    int getCurrentTier() const { return currentTier; }  // 🔹 新增
    const ScoreResult& getLastPlayResult() const { return lastPlayResult; }
    const std::vector<Tile>& getLastPlayedTiles() const { return lastPlayedTiles; }
    const LevelStats& getCurrentLevelStats() const { return currentLevelStats; }
    void resetLevelStats(int tier, int level);
    const LevelStats& getLastLevelStats() const { return lastLevelStats; }  // 🔹 新增 getter
    const std::map<YakuType, int>& getSessionHighYakus() const { return sessionHighYakus; }
    void resetSessionStats() { sessionHighYakus.clear(); }

signals:
    void levelStarted();
    void actionsUpdated(int playsLeft, int discardsLeft);
    void handUpdated();
    void wallSizeUpdated(int size);
    void scoreUpdated(int current, int target);
    void levelCleared();
    void gameOver();
    void levelInfoUpdated(int tier, int level, int prevWind, int seatWind, bool infinite);
    void doraInfoUpdated(Tile indicator, Tile dora);  // 🔹 新增：宝牌信息
    void tilesRemoved(const std::vector<Tile>& removedTiles);
    void scorePopupRequested(const ScoreResult& result,
                             const std::vector<Tile>& played,
                             const std::vector<Tile>& currentHand);  // 🔹 新增
    void levelCheckRequested();
    void gameCleared();  // 🔹 通关信号
    // 💰 新增信号
    void moneyUpdated(int currentMoney);

private:
    HandManager handMgr;
    int playCount = 4;
    int discardCount = 3;
    int currentScore = 0;
    int targetScore = 2000;
    int currentLevel = 1;

    // 关卡系统
    int currentTier = 1;
    int currentLevelInTier = 1;
    int currentPrevalentWind = 1;  // 1=东,2=南,3=西,4=北
    int currentSeatWind = 1;
    bool infiniteMode = false;
    int baseTierPrevWind = 1;           // 第一层初始场风种子
    int currentTierBaseSeatWind = 1;    // 当前层初始自风种子

    // 🔹 宝牌系统
    Tile currentDoraIndicator;  // 宝牌指示物
    Tile currentDoraTile;       // 本关实际宝牌
    Tile computeNextDora(const Tile& indicator);  // 🔹 推算宝牌

    void calculateCurrentWinds();
    void loadLevelConfig();
    void advanceLevel();
    void handleFailure();
    void updateUIStates();

    int calculatePlayScore(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);
    int lastPlayScore = 0;
    int maxSinglePlayScore = 0;

    // 💰 新增成员变量
    int currentMoney = 0;

    // 🔹 新增：奖励计算函数
    void awardTierClearReward();

    // 🔹 通关奖励加成状态（每小关通关后获得）
    int bonusTotalScore = 0;      // 总分加成
    int bonusTotalFan = 0;        // 总番数加成
    int bonusDoraFan = 0;         // 宝牌番数加成
    int bonusPlayCount = 0;       // 出牌次数加成
    int bonusDiscardCount = 0;    // 弃牌次数加成

    // 🔹 新增：永久累积 Bonus 状态
    int bonusBaseScore = 0;              // 1-3: 基础分+100
    bool bonusDoubleOneFanYaku = false;  // 2-1: 1番番种额外结算一次
    bool bonusDiscardToFanScore = false; // 2-2: 弃牌转番数&分数 (动态)
    bool bonusAllSameSuitAsDora = false; // 3-4: 同花色全宝牌
    double bonusFanMultiplier = 1.0;     // 4-2: 番数乘区 (默认1.0，通关后永久变为3.0)

    // 🔹 应用通关奖励加成
    void applyLevelClearBonus();
    // 🔹 清空通关奖励加成（新大关开始时）
    void clearLevelBonuses();
    // gamemanager.h - private 区域追加：
    ScoreResult lastPlayResult;  // 🔹 缓存上一手详细计分结果
    std::vector<Tile> lastPlayedTiles; // 🔹 缓存上一手打出的牌
    LevelStats currentLevelStats;  // 🔹 当前关卡统计

    void recordHighYaku(YakuType yaku);
    void applyLevelAbility(int tier, int level);
    void finalizeLevelStats();
    void recordAbility(const QString& desc);
    LevelStats lastLevelStats;      // 🔹 新增：上一关最终统计（持久化）
    std::map<YakuType, int> sessionHighYakus;  // 🔹 全局累积高番型
};
>>>>>>> 5fcafe7 (最终版代码提交)
