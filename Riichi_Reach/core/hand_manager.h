#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include "tile.h"

class HandManager {
    std::vector<Tile> wall;
    std::vector<Tile> hand;
    std::vector<Tile> discard;
    std::mt19937 rng;

public:
    HandManager() : rng(std::random_device{}()) {}

    // 🔹 初始化完整136张牌（含3枚赤宝牌：0m, 0p, 0s）
    void initDeck() {
        wall.clear();
        hand.clear();
        discard.clear();

        // 万子 1m~9m 各4张（其中1张5m替换为赤宝牌0m）
        for (uint8_t v = 1; v <= 9; ++v) {
            int count = 4;
            bool isRedFive = (v == 5);
            for (int i = 0; i < count; ++i) {
                if (isRedFive && i == 0) {
                    wall.push_back({TileSuit::MAN, 0, true}); // 赤五万
                } else {
                    wall.push_back({TileSuit::MAN, v, false});
                }
            }
        }

        // 筒子 1p~9p 各4张（其中1张5p替换为赤宝牌0p）
        for (uint8_t v = 1; v <= 9; ++v) {
            int count = 4;
            bool isRedFive = (v == 5);
            for (int i = 0; i < count; ++i) {
                if (isRedFive && i == 0) {
                    wall.push_back({TileSuit::PIN, 0, true}); // 赤五筒
                } else {
                    wall.push_back({TileSuit::PIN, v, false});
                }
            }
        }

        // 索子 1s~9s 各4张（其中1张5s替换为赤宝牌0s）
        for (uint8_t v = 1; v <= 9; ++v) {
            int count = 4;
            bool isRedFive = (v == 5);
            for (int i = 0; i < count; ++i) {
                if (isRedFive && i == 0) {
                    wall.push_back({TileSuit::SOU, 0, true}); // 赤五索
                } else {
                    wall.push_back({TileSuit::SOU, v, false});
                }
            }
        }

        // 字牌 1z~7z 各4张（东南西北白发中）
        for (uint8_t v = 1; v <= 7; ++v) {
            for (int i = 0; i < 4; ++i) {
                wall.push_back({TileSuit::ZI, v, false});
            }
        }

        // 洗牌
        std::shuffle(wall.begin(), wall.end(), rng);
    }

    // 🔹 摸牌
    Tile draw() {
        if (wall.empty()) {
            return {TileSuit::ZI, 1, false}; // 兜底：东风
        }
        Tile t = wall.back();
        wall.pop_back();
        hand.push_back(t);
        return t;
    }

    // 🔹 打牌（从手牌移除，加入弃牌区）
    bool playTiles(const std::vector<Tile>& selected) {
        bool allFound = true;
        for (const auto& t : selected) {
            auto it = std::find(hand.begin(), hand.end(), t);
            if (it != hand.end()) {
                discard.push_back(*it);
                hand.erase(it);
            } else {
                allFound = false;
            }
        }
        return allFound;
    }

    // 🔹 访问器
    const std::vector<Tile>& getHand() const { return hand; }
    const std::vector<Tile>& getDiscard() const { return discard; }
    int wallSize() const { return static_cast<int>(wall.size()); }
    bool isEmpty() const { return wall.empty(); }
};
