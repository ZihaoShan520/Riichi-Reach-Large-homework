#pragma once
#include <QString>
#include <QMetaType>
#include <vector>

enum class TileSuit : uint8_t { MAN, PIN, SOU, ZI };

// 🔹 预留单牌属性系统（后续可无缝接入宝牌/锁定/特效等）
struct TileProperties {
    int baseScore = 10;      // 基础分
    bool isLocked = false;   // 是否被锁定（不可打出/弃置）
    bool isDora = false;     // 宝牌标记
    int bonusMultiplier = 0; // 额外倍率加成
    // 后续扩展字段直接在此添加
};

struct Tile {
    TileSuit suit = TileSuit::MAN;
    uint8_t value = 1;       // 1~9(m/p/s), 0(赤), 1~7(z)
    bool isRed = false;
    TileProperties props;

    QString id() const {
        char c = (suit == TileSuit::MAN) ? 'm' :
                     (suit == TileSuit::PIN) ? 'p' :
                     (suit == TileSuit::SOU) ? 's' : 'z';
        return QString("%1%2").arg(value).arg(c);
    }

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
        if (suit == TileSuit::MAN && value <= 9) name = man[value];
        else if (suit == TileSuit::PIN && value <= 9) name = pin[value];
        else if (suit == TileSuit::SOU && value <= 9) name = sou[value];
        else if (suit == TileSuit::ZI && value <= 7) name = zi[value];
        return name ? QString(name) : "??";
    }

    bool operator==(const Tile& o) const {
        return suit == o.suit && value == o.value && isRed == o.isRed;
    }
    bool isYakuhai() const { return suit == TileSuit::ZI; }
    bool isTerminal() const { return suit == TileSuit::ZI || value == 1 || value == 9; }
};
Q_DECLARE_METATYPE(Tile)
