#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include "tile.h"

class HandManager {
    std::vector<Tile> wall;
    std::vector<Tile> hand;
    std::vector<Tile> discardPile;
    std::mt19937 rng;
    int maxHandSize = 18;

public:
    HandManager() : rng(std::random_device{}()) {}

    void initDeck() {
        wall.clear(); hand.clear(); discardPile.clear();
        // 标准136张 + 赤宝牌逻辑（同前）
        auto addSuit = [&](TileSuit suit) {
            for (uint8_t v = 1; v <= 9; ++v) {
                for (int i = 0; i < 4; ++i) {
                    bool red = (v == 5 && i == 0);
                    wall.push_back({suit, red ? 0 : v, red});
                }
            }
        };
        addSuit(TileSuit::MAN); addSuit(TileSuit::PIN); addSuit(TileSuit::SOU);
        for (uint8_t v = 1; v <= 7; ++v)
            for (int i = 0; i < 4; ++i) wall.push_back({TileSuit::ZI, v, false});

        // 洗牌
        std::shuffle(wall.begin(), wall.end(), rng);
    }

    // 🔹 动作验证
    bool canDraw(int count) const { return wall.size() >= count && (hand.size() + count) <= maxHandSize; }
    bool canDiscard(int count) const { return count >= 1 && count <= 14 && count <= hand.size(); }
    bool canPlay(int count) const { return count >= 8 && count <= 14 && count <= hand.size(); }

    // 🔹 核心操作
    std::vector<Tile> drawTiles(int count) {
        if (!canDraw(count)) return {};
        std::vector<Tile> drawn;
        for (int i = 0; i < count; ++i) {
            drawn.push_back(wall.back());
            wall.pop_back();
            hand.push_back(drawn.back());
        }
        return drawn;
    }

    bool removeTiles(const std::vector<Tile>& tiles) {
        for (const auto& t : tiles) {
            auto it = std::find(hand.begin(), hand.end(), t);
            if (it != hand.end()) hand.erase(it);
            else return false;
        }
        return true;
    }

    void addToDiscard(const std::vector<Tile>& tiles) {
        discardPile.insert(discardPile.end(), tiles.begin(), tiles.end());
    }

    // 🔹 状态访问
    const std::vector<Tile>& getHand() const { return hand; }
    const std::vector<Tile>& getDiscardPile() const { return discardPile; }
    int wallSize() const { return wall.size(); }
    int handSize() const { return hand.size(); }
    int maxHand() const { return maxHandSize; }
};
