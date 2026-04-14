#pragma once
#include <QString>
#include <QMetaType>

enum class TileSuit : uint8_t { MAN, PIN, SOU, ZI };

struct Tile {
    TileSuit suit = TileSuit::MAN;
    uint8_t value = 1; // m/p/s: 0~9 (0=赤宝牌) | z: 1~7(东南西北白发中)
    bool isRed = false; // 赤宝牌标记

    // 🔹 返回标准ID：如 "1m", "0m"(赤五万), "3z", "5p"
    QString id() const {
        char c = 'm';
        switch (suit) {
        case TileSuit::MAN: c = 'm'; break;
        case TileSuit::PIN: c = 'p'; break;
        case TileSuit::SOU: c = 's'; break;
        case TileSuit::ZI:  c = 'z'; break;
        }
        return QString("%1%2").arg(value).arg(c);
    }

    // 🔹 中文显示名
    QString displayName() const {
        static const char* man[] = {"", "一萬", "二萬", "三萬", "四萬", "五萬", "六萬", "七萬", "八萬", "九萬"};
        static const char* pin[] = {"", "一筒", "二筒", "三筒", "四筒", "五筒", "六筒", "七筒", "八筒", "九筒"};
        static const char* sou[] = {"", "一索", "二索", "三索", "四索", "五索", "六索", "七索", "八索", "九索"};
        static const char* zi[]  = {"", "東", "南", "西", "北", "白", "發", "中"};

        if (isRed) {
            if (suit == TileSuit::MAN) return "赤五萬";
            if (suit == TileSuit::PIN) return "赤五筒";
            if (suit == TileSuit::SOU) return "赤五索";
        }

        const char* name = nullptr;
        if (suit == TileSuit::MAN && value >= 1 && value <= 9) name = man[value];
        else if (suit == TileSuit::PIN && value >= 1 && value <= 9) name = pin[value];
        else if (suit == TileSuit::SOU && value >= 1 && value <= 9) name = sou[value];
        else if (suit == TileSuit::ZI && value >= 1 && value <= 7) name = zi[value];

        return name ? QString(name) : "??";
    }

    // 🔹 比较运算符
    bool operator==(const Tile& o) const {
        return suit == o.suit && value == o.value;
    }

    // 🔹 是否为字牌
    bool isYakuhai() const {
        return suit == TileSuit::ZI;
    }

    // 🔹 是否为九牌（1或9或字牌）
    bool isTerminal() const {
        if (suit == TileSuit::ZI) return true;
        return value == 1 || value == 9;
    }
};
Q_DECLARE_METATYPE(Tile)
