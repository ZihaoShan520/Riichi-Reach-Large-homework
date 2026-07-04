#include "widget.h"
#include "manager/gamemanager.h"
#include "core/tile.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QDebug>
#include <QVector>
#include <algorithm>
#include <QMouseEvent>
#include <QCursor>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPixmap>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QThread>
#include <QSet>
#include <QHideEvent>
#include <QEvent>

// ============ 辅助：可点击的牌面标签 ============
class TileLabel : public QLabel {
    Q_OBJECT
public:
    explicit TileLabel(const Tile& tile, QWidget* p = nullptr) : QLabel(p), currentTile(tile) {
        setMouseTracking(true);
        setFixedSize(42, 58);
        setAlignment(Qt::AlignCenter);
        setCursor(Qt::PointingHandCursor);
        loadTileImage();
    }

    Tile getTile() const { return currentTile; }
    void setDragIndex(int i) { dragIndex = i; }
    void refreshImage() { loadTileImage(); }

signals:
    void clicked();
    void dragReleased(int);

protected:
    void mousePressEvent(QMouseEvent* e) override {
        startDragPos = e->globalPosition().toPoint();
        QLabel::mousePressEvent(e);
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        if (QPoint(e->globalPosition().toPoint() - startDragPos).manhattanLength() > 12)
            emit dragReleased(dragIndex);
        else
            emit clicked();
    }

private:
    Tile currentTile;
    QPoint startDragPos;
    int dragIndex = -1;

    void loadTileImage() {
        QString path;
        if (currentTile.suit == TileSuit::ZI) {
            path = QString(":/tiles/images/%1z.png").arg(currentTile.value);
        } else {
            char c = (currentTile.suit == TileSuit::MAN) ? 'm' :
                         (currentTile.suit == TileSuit::PIN) ? 'p' : 's';
            path = QString(":/tiles/images/%1%2.png").arg(currentTile.value).arg(c);
        }

        QPixmap pm(path);
        if (!pm.isNull() && !pm.width() == 0) {
            setPixmap(pm.scaled(36, 52, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            setText("");
            setStyleSheet("QLabel{background:transparent;border:none;}");
        } else {
            qDebug() << "[⚠️ IMG FAIL]" << path << "| Tile:" << currentTile.id();
            setText(currentTile.id());
            setStyleSheet("QLabel{background:#2a2a4a;color:#ddd;border:2px solid #444;border-radius:6px;font:16px bold;}");
        }
    }
};

// ============ 视图 1：主菜单 ============
class MenuView : public QWidget {
    Q_OBJECT
public:
    explicit MenuView(QWidget* p = nullptr) : QWidget(p) {
        setStyleSheet("background:#1a1a2e;");
        auto* l = new QVBoxLayout(this);
        auto* t = new QLabel("Riichi Reach\nMahjong Roguelike", this);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("color:white;font:32px bold;");
        l->addWidget(t);
        auto* b = new QPushButton("START GAME", this);
        b->setFixedSize(220, 50);
        b->setStyleSheet("QPushButton{background:#e94560;color:white;font:18px bold;border-radius:8px;border:none;}QPushButton:hover{background:#ff6b6b;}");
        l->addWidget(b, 0, Qt::AlignCenter);
        l->addStretch();
        connect(b, &QPushButton::clicked, this, [this]() { emit startRequested(); });
    }
signals:
    void startRequested();
};

// ============ 视图 2：过场动画 (Cutscene) ============
class TransitionView : public QWidget {
    Q_OBJECT
public:
    explicit TransitionView(QWidget* p = nullptr) : QWidget(p) {
        setStyleSheet("background:rgba(0,0,0,0.85);");
        auto* l = new QVBoxLayout(this);
        l->addStretch(2);
        label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color:white;font:48px bold;");
        label->setGraphicsEffect(new QGraphicsOpacityEffect());
        l->addWidget(label);
        l->addStretch(3);
    }
    void play(const QString& msg, bool isSuccess) {
        label->setText(msg);
        label->setStyleSheet(isSuccess ? "color:#4ecca3;font:48px bold;" : "color:#e94560;font:48px bold;");
        auto* eff = static_cast<QGraphicsOpacityEffect*>(label->graphicsEffect());
        auto* anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(1500);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->start();
        QTimer::singleShot(1500, this, [this]() { emit finished(); });
    }
signals:
    void finished();
private:
    QLabel* label;
};

// ============ 视图 3：商店界面 (完整修复版) ============
class ShopView : public QWidget {
    Q_OBJECT
public:
    explicit ShopView(QWidget* p = nullptr) : QWidget(p) {
        setStyleSheet("background:#16213e;");
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(30, 30, 30, 30);
        l->setSpacing(12);

        // 🔹 1. 奖励统计区 (原样保留，默认隐藏)
        rewardContainer = new QWidget(this);
        rewardContainer->setStyleSheet("background:#0f3460;border:2px solid #4ecca3;border-radius:10px;padding:10px;");
        rewardContainer->setVisible(false);
        auto* rLayout = new QVBoxLayout(rewardContainer);
        rLayout->setSpacing(6);

        auto* rTitle = new QLabel("🎉 关卡通关奖励", rewardContainer);
        rTitle->setStyleSheet("color:#ffd166;font:16px bold;"); rTitle->setAlignment(Qt::AlignCenter);
        rLayout->addWidget(rTitle);

        baseRewardLabel = new QLabel("", rewardContainer); baseRewardLabel->setStyleSheet("color:#ddd;font:13px;");
        playCountRewardLabel = new QLabel("", rewardContainer); playCountRewardLabel->setStyleSheet("color:#ddd;font:13px;");
        scoreBonusLabel = new QLabel("", rewardContainer); scoreBonusLabel->setStyleSheet("color:#ddd;font:13px;");

        auto* sep = new QFrame(rewardContainer);
        sep->setFrameShape(QFrame::HLine); sep->setStyleSheet("background:#4ecca3;min-height:2px;");

        totalRewardLabel = new QLabel("", rewardContainer); totalRewardLabel->setStyleSheet("color:#4ecca3;font:15px bold;"); totalRewardLabel->setAlignment(Qt::AlignCenter);
        currentMoneyLabel = new QLabel("", rewardContainer); currentMoneyLabel->setStyleSheet("color:#ffd166;font:14px bold;"); currentMoneyLabel->setAlignment(Qt::AlignCenter);

        rLayout->addWidget(baseRewardLabel);
        rLayout->addWidget(playCountRewardLabel);
        rLayout->addWidget(scoreBonusLabel);
        rLayout->addWidget(sep);
        rLayout->addWidget(totalRewardLabel);
        rLayout->addWidget(currentMoneyLabel);
        l->addWidget(rewardContainer);

        // 🔹 2. 本关统计区 (紧凑圆角版，padding 8px，字体 12-14px)
        statsBox = new QWidget(this);
        statsBox->setStyleSheet("background:#0f3460;border:2px solid #4ecca3;border-radius:10px;padding:2px;");
        statsBox->setMinimumHeight(180);
        statsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto* sLayout = new QVBoxLayout(statsBox);
        sLayout->setContentsMargins(8, 8, 8, 8);
        sLayout->setSpacing(5);

        auto* sTitle = new QLabel("📊 本关统计", statsBox);
        sTitle->setStyleSheet("color:#ffd166;font:14px bold;"); sTitle->setAlignment(Qt::AlignCenter);
        sLayout->addWidget(sTitle);

        auto addRow = [sLayout](const QString& label, const QString& value) {
            auto* row = new QHBoxLayout();
            row->setContentsMargins(0, 0, 0, 0);
            auto* lbl = new QLabel(label); lbl->setStyleSheet("color:#a0c4ff;font:12px;");
            auto* val = new QLabel(value); val->setStyleSheet("color:#fff;font:12px bold;"); val->setAlignment(Qt::AlignRight);
            row->addWidget(lbl); row->addStretch(); row->addWidget(val);
            sLayout->addLayout(row);
        };

        addRow("本关总得分", "<SCORE>");
        scoreStatLabel = new QLabel("", statsBox);
        scoreStatLabel->setStyleSheet("color:#4ecca3;font:13px bold;"); scoreStatLabel->setAlignment(Qt::AlignRight);
        sLayout->addWidget(scoreStatLabel);

        auto* pRow = new QHBoxLayout(); pRow->setContentsMargins(0, 0, 0, 0);
        auto* pLbl = new QLabel("剩余出牌/弃牌:"); pLbl->setStyleSheet("color:#a0c4ff;font:12px;");
        playsLeftLabel = new QLabel("", statsBox); playsLeftLabel->setStyleSheet("color:#fff;font:12px bold;min-width:25px;");
        discardsLeftLabel = new QLabel("", statsBox); discardsLeftLabel->setStyleSheet("color:#fff;font:12px bold;min-width:25px;");
        pRow->addWidget(pLbl); pRow->addWidget(playsLeftLabel); pRow->addWidget(new QLabel("/")); pRow->addWidget(discardsLeftLabel); pRow->addStretch();
        sLayout->addLayout(pRow);

        auto* hyTitle = new QLabel("🏆 高番型 (3 番+)", statsBox);
        hyTitle->setStyleSheet("color:#ffd166;font:12px bold;");
        sLayout->addWidget(hyTitle);

        highYakuScroll = new QScrollArea(statsBox);
        highYakuScroll->setWidgetResizable(true);
        highYakuScroll->setFixedHeight(55);
        highYakuScroll->setFixedWidth(280);  // 🔹 关键：固定宽度，防止内部撑开
        highYakuScroll->setStyleSheet(
            "QScrollArea{border:1px solid #4ecca3;border-radius:4px;background:#1a1a2e;}"
            "QScrollBar:vertical{width:10px;background:#0f3460;}"
            "QScrollBar::handle:vertical{background:#4ecca3;min-height:20px;border-radius:2px;}"
            );
        highYakuContent = new QWidget();
        highYakuContent->setFixedWidth(260);  // 🔹 关键：内部容器比滚动区窄，留出滚动条空间
        highYakuLayout = new QVBoxLayout(highYakuContent);
        highYakuLayout->setContentsMargins(4, 2, 4, 2);
        highYakuLayout->setSpacing(2);
        highYakuScroll->setWidget(highYakuContent);
        sLayout->addWidget(highYakuScroll);

        auto* abTitle = new QLabel("✨ 本关能力", statsBox);
        abTitle->setStyleSheet("color:#ffd166;font:12px bold;"); sLayout->addWidget(abTitle);

        abilityContent = new QLabel("暂无", statsBox);
        abilityContent->setStyleSheet("color:#4ecca3;font:12px;background:#1a1a2e;border:1px solid #444;border-radius:6px;padding:4px 8px;");
        abilityContent->setWordWrap(false); abilityContent->setFixedHeight(24);
        sLayout->addWidget(abilityContent);

        l->addWidget(statsBox);
        l->addStretch();

        // 🔹 3. 底部按钮
        nextLevelBtn = new QPushButton("NEXT LEVEL (免费)", this);
        nextLevelBtn->setFixedSize(250, 55);
        nextLevelBtn->setStyleSheet("QPushButton{background:#4ecca3;color:#0a1929;font:18px bold;border-radius:10px;border:none;}");
        l->addWidget(nextLevelBtn, 0, Qt::AlignCenter);
        l->addStretch();

        connect(nextLevelBtn, &QPushButton::clicked, this, [this]() {
            clearRewardInfo();
            emit nextLevelRequested();
        });
    }

    void showRewardInfo(int base, int play, int bonus, int total, int current) {
        baseRewardLabel->setText(QString("基础奖励：+%1 金钱").arg(base));
        playCountRewardLabel->setText(QString("剩余出牌次数：+%1 金钱").arg(play));
        scoreBonusLabel->setText(QString("得分翻倍奖励：+%1 金钱").arg(bonus));
        totalRewardLabel->setText(QString("总计：+%1 金钱").arg(total));
        currentMoneyLabel->setText(QString("当前总金钱：%1").arg(current));
        rewardContainer->show();
    }

    void clearRewardInfo() {
        rewardContainer->hide();
    }

    void setLevelStats(const LevelStats& stats, const std::map<YakuType, int>& accumulatedYakus) {
        scoreStatLabel->setText(QString::number(stats.finalScore) + " 分");
        playsLeftLabel->setText(QString::number(stats.playsLeft));
        discardsLeftLabel->setText(QString::number(stats.discardsLeft));

        // 🔹 清空旧列表
        QLayoutItem* item;
        while ((item = highYakuLayout->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }

        // 🔹 使用累积数据，暂无改为白色
        if (accumulatedYakus.empty()) {
            auto* none = new QLabel("暂无", highYakuContent);
            none->setStyleSheet("color:white;font:12px;");
            highYakuLayout->addWidget(none);
        } else {
            for (const auto& [yaku, count] : accumulatedYakus) {
                auto* lbl = new QLabel("• " + YakuCalculator::yakuName(yaku) + " ×" + QString::number(count), highYakuContent);
                lbl->setStyleSheet("color:white;font:11px;padding:1px 0;");
                highYakuLayout->addWidget(lbl);
            }
        }

        // 能力部分保持原样
        if (stats.abilitiesGained.isEmpty()) {
            abilityContent->setText("暂无");
            abilityContent->setStyleSheet("color:#666;font:12px;background:#1a1a2e;border:1px solid #444;border-radius:6px;padding:4px 8px;");
        } else {
            QString txt = stats.abilitiesGained.join(" / ");
            if (txt.length() > 28) txt = txt.left(25) + "...";
            abilityContent->setText(txt);
            abilityContent->setStyleSheet("color:#4ecca3;font:12px;background:#1a1a2e;border:1px solid #4ecca3;border-radius:6px;padding:4px 8px;");
        }

        QTimer::singleShot(0, this, [this]() {
            highYakuContent->adjustSize();
            highYakuScroll->update();
        });
    }

signals:
    void nextLevelRequested();

private:
    // 奖励区
    QWidget* rewardContainer;
    QLabel *baseRewardLabel, *playCountRewardLabel, *scoreBonusLabel;
    QLabel *totalRewardLabel, *currentMoneyLabel;
    // 统计区
    QWidget* statsBox;
    QLabel *scoreStatLabel, *playsLeftLabel, *discardsLeftLabel;
    QScrollArea* highYakuScroll;
    QWidget* highYakuContent;
    QVBoxLayout* highYakuLayout;
    QLabel* abilityContent;
    QPushButton* nextLevelBtn;
};
// ============ 视图 4：结算界面 ============
class ResultView : public QWidget {
    Q_OBJECT
public:
    explicit ResultView(QWidget* p = nullptr) : QWidget(p) {
        setStyleSheet("background:#2b1116;");
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(40, 40, 40, 40);
        auto* title = new QLabel("GAME OVER", this);
        title->setStyleSheet("color:#e94560;font:42px bold;");
        title->setAlignment(Qt::AlignCenter);
        l->addWidget(title);
        scoreLabel = new QLabel("Total Score: 0", this);
        maxLabel = new QLabel("Max Single Score: 0", this);
        scoreLabel->setStyleSheet("color:white;font:24px;");
        maxLabel->setStyleSheet("color:#a0c4ff;font:20px;");
        scoreLabel->setAlignment(Qt::AlignCenter);
        maxLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(scoreLabel);
        l->addWidget(maxLabel);
        l->addStretch();
        auto* btn = new QPushButton("MAIN MENU", this);
        btn->setFixedSize(250, 60);
        btn->setStyleSheet("QPushButton{background:#555;color:white;font:20px bold;border-radius:10px;border:none;}");
        l->addWidget(btn, 0, Qt::AlignCenter);
        l->addStretch();
        connect(btn, &QPushButton::clicked, this, [this]() { emit menuRequested(); });
    }
    void setStats(int total, int maxPlay) {
        scoreLabel->setText(QString("Total Score: %1").arg(total));
        maxLabel->setText(QString("Max Single Score: %1").arg(maxPlay));
    }
signals:
    void menuRequested();
private:
    QLabel *scoreLabel, *maxLabel;
};

// ============ 视图 5：结算提示框 ============
class ScorePopupView : public QWidget {
    Q_OBJECT
public:
    explicit ScorePopupView(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(400, 320);

        auto* overlay = new QWidget(this);
        overlay->setGeometry(0, 0, 400, 320);
        overlay->setStyleSheet("background:rgba(0,0,0,0.7);border-radius:12px;");

        auto* content = new QWidget(this);
        content->setGeometry(20, 20, 360, 280);
        content->setStyleSheet("background:#1a1a2e;border:2px solid #4ecca3;border-radius:10px;");
        auto* layout = new QVBoxLayout(content);
        layout->setContentsMargins(15, 15, 15, 15);
        layout->setSpacing(8);

        auto* title = new QLabel("🀄 结算详情", this);
        title->setStyleSheet("color:#ffd166;font:18px bold;");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);

        scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("border:none;background:transparent;");
        detailWidget = new QWidget();
        detailLayout = new QVBoxLayout(detailWidget);
        detailLayout->setContentsMargins(0, 0, 0, 0);
        detailLayout->setSpacing(4);
        scrollArea->setWidget(detailWidget);
        layout->addWidget(scrollArea);

        auto* hint = new QLabel("点击任意位置继续", this);
        hint->setStyleSheet("color:#a0c4ff;font:12px;");
        hint->setAlignment(Qt::AlignCenter);
        layout->addWidget(hint);

        installEventFilter(this);
    }

    void showScore(const ScoreResult& result, const std::vector<Tile>& played) {
        isDismissed = false;

        std::function<void(QLayout*)> clearLayout = [&](QLayout* layout) {
            QLayoutItem* item;
            while ((item = layout->takeAt(0)) != nullptr) {
                if (item->layout()) clearLayout(item->layout());
                else if (item->widget()) delete item->widget();
                delete item;
            }
        };
        clearLayout(detailLayout);

        auto addRow = [this](const QString& label, const QString& value, bool highlight = false) {
            auto* row = new QHBoxLayout();
            auto* lbl = new QLabel(label);
            lbl->setStyleSheet("color:#ddd;font:13px;");
            auto* val = new QLabel(value);
            val->setStyleSheet(highlight ? "color:#4ecca3;font:13px bold;" : "color:#fff;font:13px;");
            val->setAlignment(Qt::AlignRight);
            row->addWidget(lbl);
            row->addStretch();
            row->addWidget(val);
            detailLayout->addLayout(row);
        };

        addRow("打出牌数", QString("%1 张").arg(played.size()));
        addRow("计分牌数", QString("%1 张").arg(result.countedTiles));
        addRow("基础点数", QString("+%1").arg(result.basePoints));

        if (result.doraCount > 0) {
            addRow("宝牌/赤宝", QString("+%1 番 (%2张)").arg(result.doraFan, 0, 'f', 1).arg(result.doraCount), true);
        }

        if (!result.activeYakus.empty() && result.activeYakus[0] != YakuType::NoYaku) {
            addRow("役种加成", QString("+%1 番").arg(result.yakuFan, 0, 'f', 1), true);
            for (YakuType y : result.activeYakus) {
                int fan = 0;
                switch (y) {
                case YakuType::NoYaku: fan = 1; break;
                case YakuType::SequentialSix: fan = 1; break;
                case YakuType::AllSimples: fan = 1; break;
                case YakuType::DragonPung: fan = 1; break;
                case YakuType::PrevalentWind: fan = 1; break;
                case YakuType::SeatWind: fan = 1; break;
                case YakuType::PureDoubleSequence: fan = 1; break;
                case YakuType::FourPairs: fan = 2; break;
                case YakuType::TwoConcealedPungs: fan = 1; break;
                case YakuType::FourIdentical: fan = 2; break;
                case YakuType::SmallThreeDragons: fan = 2; break;
                case YakuType::NotBreaking: fan = 2; break;
                case YakuType::PureStraight: fan = 2; break;
                case YakuType::MixedTripleSequence: fan = 2; break;
                case YakuType::AllTerminals: fan = 2; break;
                case YakuType::ThreeConcealedPungs: fan = 2; break;
                case YakuType::FiveFamilies: fan = 2; break;
                case YakuType::SevenPairs: fan = 3; break;
                case YakuType::TripleTriplets: fan = 3; break;
                case YakuType::MixedTerminalHonors: fan = 3; break;
                case YakuType::TwoPureDoubleSequences: fan = 3; break;
                case YakuType::FullFlush: fan = 5; break;
                case YakuType::PureTripleSequence: fan = 5; break;
                case YakuType::AllHonors: fan = 13; break;
                case YakuType::BigThreeDragons: fan = 13; break;
                case YakuType::FourConcealedPungs: fan = 13; break;
                case YakuType::FullGreen: fan = 13; break;
                case YakuType::FourWinds: fan = 13; break;
                case YakuType::NineGates: fan = 13; break;
                case YakuType::MillionStone: fan = 13; break;
                case YakuType::OnePointRed: fan = 13; break;
                // case YakuType::TwoPureDoubleChows: fan = 20; break;
                case YakuType::guoshi: fan = 20; break;
                default: fan = 1; break;
                }
                addRow("  • " + YakuCalculator::yakuName(y) + " " + QString::number(fan) + "番", "", false);
            }
        } else {
            addRow("役种", "[无役] 0.5番", false);
        }

        addRow("基础番数", "+0.5", false);
        if (result.isWinHand) addRow("和牌加成", "×3", true);

        auto* sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color:#444;");
        detailLayout->addWidget(sep);

        QString totalText = QString("💰 总分: %1").arg(result.finalScore);
        addRow(totalText, "", true);

        if (auto* p = parentWidget()) move(p->rect().center() - rect().center());
        show();
        raise();
        activateWindow();
    }

protected:
    void hideEvent(QHideEvent* event) override {
        QWidget::hideEvent(event);
        if (!isDismissed) {
            isDismissed = true;
            emit dismissed();
        }
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (!isDismissed && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress)) {
            hide();
            return true;
        }
        return QWidget::eventFilter(obj, event);
    }

signals:
    void dismissed();

private:
    bool isDismissed = false;
    QScrollArea* scrollArea;
    QWidget* detailWidget;
    QVBoxLayout* detailLayout;
};

// ============ 视图 6：游戏区 (GameView) ============
class GameView : public QWidget {
    Q_OBJECT
public:
    explicit GameView(QWidget* p = nullptr) : QWidget(p) {
        setStyleSheet("background:#16213e;");
        setupUI();
    }
    struct SelectionResult { std::vector<Tile> playedSet, playOrder; };
    SelectionResult getSelectionResult() const {
        SelectionResult r;
        for (int i = 0; i < isSelected.size(); ++i)
            if (isSelected[i]) r.playedSet.push_back(handTiles[i]);
        r.playOrder = std::vector<Tile>(clickedOrder.begin(), clickedOrder.end());
        return r;
    }
    void clearSelection() {
        for (int i = 0; i < isSelected.size(); ++i) isSelected[i] = false;
        clickedOrder.clear();
        refreshTileStyles();
    }
    void connectPopupDismissed(QObject* receiver, std::function<void()> callback) {
        connect(scorePopup, &ScorePopupView::dismissed, receiver,
                [callback]() { callback(); }, Qt::SingleShotConnection);
    }
public slots:
    void updateHUD(int s, int t) { hudLabel->setText(QString("Score: %1 / %2").arg(s).arg(t)); }
    void updateActions(int p, int d) { actionLabel->setText(QString("Plays: %1 | Discards: %2").arg(p).arg(d)); }
    void updateLevelInfo(int t, int l, int pw, int sw, bool inf) {
        static const QString wn[] = { " ", "東", "南", "西", "北" };
        QString it = inf ? " [∞]" : "";
        levelLabel->setText(QString("Tier: %1 | Level: %2%3").arg(t).arg(l).arg(it));
        windLabel->setText(QString("場風: %1 | 自風: %2").arg(wn[pw]).arg(wn[sw]));
    }
    void updateDora(Tile ind, Tile dora) {
        auto ts = [](const Tile& t) -> QString {
            if (t.isRed) return t.suit == TileSuit::MAN ? "赤五萬" : t.suit == TileSuit::PIN ? "赤五筒" : "赤五索";
            static const QString s[] = { " ", "一萬", "二萬", "三萬", "四萬", "五萬", "六萬", "七萬", "八萬", "九萬",
                                        "一筒", "二筒", "三筒", "四筒", "五筒", "六筒", "七筒", "八筒", "九筒",
                                        "一索", "二索", "三索", "四索", "五索", "六索", "七索", "八索", "九索",
                                        "東", "南", "西", "北", "白", "發", "中" };
            int i = (t.suit == TileSuit::MAN ? t.value : (t.suit == TileSuit::PIN ? t.value + 9 : (t.suit == TileSuit::SOU ? t.value + 18 : t.value + 27)));
            return (i > 0 && i < 35) ? s[i] : t.id();
        };
        doraLabel->setText(QString("指示物: %1 | 宝牌: %2").arg(ts(ind)).arg(ts(dora)));
    }
    void updateMoney(int m) { moneyLabel->setText(QString("💰 金钱: %1").arg(m)); }

    void refreshHand(const std::vector<Tile>& newHand) {
        if (waitForPopup) {
            pendingHand = newHand;
            return;
        }
        if (isAnimating) {
            pendingHand = newHand;
            return;
        }

        if (previousHand.isEmpty()) {
            rebuildHandUI(newHand);
            previousHand = QVector<Tile>(newHand.begin(), newHand.end());
            return;
        }

        std::vector<Tile> oldVec(previousHand.begin(), previousHand.end());
        std::vector<Tile> newVec = newHand;
        std::vector<Tile> toDiscard, toDraw;

        for (const auto& tile : oldVec) {
            auto it = std::find(newVec.begin(), newVec.end(), tile);
            if (it != newVec.end()) newVec.erase(it);
            else toDiscard.push_back(tile);
        }
        toDraw = newVec;

        if (toDiscard.empty() && toDraw.empty()) {
            rebuildHandUI(newHand);
            previousHand = QVector<Tile>(newHand.begin(), newHand.end());
            return;
        }

        isAnimating = true;
        playDiscardAnimation(toDiscard, toDraw, newHand);
    }

    void showScorePopup(const ScoreResult& result,
                        const std::vector<Tile>& played,
                        const std::vector<Tile>& currentHand) {
        if (waitForPopup) return;
        waitForPopup = true;
        pendingHand = currentHand;
        scorePopup->showScore(result, played);
    }

signals:
    void returnToMenuRequested();
    void playRequested(const std::vector<Tile>&, const std::vector<Tile>&);
    void discardRequested(const std::vector<Tile>&);

protected:
    // 🔹 暴力屏蔽：动画或弹窗期间拦截所有鼠标事件（解决情况2）
    bool event(QEvent* event) override {
        if ((isAnimating || waitForPopup) &&
            (event->type() == QEvent::MouseButtonPress ||
             event->type() == QEvent::MouseButtonRelease ||
             event->type() == QEvent::MouseMove)) {
            return true; // 吞掉事件
        }
        return QWidget::event(event);
    }

    // 🔹 视图隐藏时强制重置状态（解决情况1、3）
    void hideEvent(QHideEvent* event) override {
        QWidget::hideEvent(event);
        isAnimating = false;
        waitForPopup = false;
        pendingHand.clear();
        if (handLayout) handLayout->setEnabled(true);
        if (scorePopup && scorePopup->isVisible()) scorePopup->hide();
        clearSelection();
    }

private:
    void setupUI() {
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(12);
        hudLabel = new QLabel("Score: 0 / 2000", this);
        hudLabel->setStyleSheet("color:white;font:18px;background:#0f3460;padding:8px;border-radius:6px;");
        hudLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(hudLabel);
        actionLabel = new QLabel("Plays: 4 | Discards: 3", this);
        actionLabel->setStyleSheet("color:#a0c4ff;font:14px;background:#0f3460;padding:6px;border-radius:6px;");
        actionLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(actionLabel);
        levelLabel = new QLabel("Tier: 1 | Level: 1", this);
        levelLabel->setStyleSheet("color:#ffd166;font:14px;background:#0f3460;padding:6px;border-radius:6px;");
        levelLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(levelLabel);
        windLabel = new QLabel("場風: 東 | 自風: 東", this);
        windLabel->setStyleSheet("color:#a0c4ff;font:14px;background:#0f3460;padding:6px;border-radius:6px;");
        windLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(windLabel);
        doraLabel = new QLabel("指示物: ? | 宝牌: ?", this);
        doraLabel->setStyleSheet("color:#f8ad9d;font:14px;background:#0f3460;padding:6px;border-radius:6px;");
        doraLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(doraLabel);
        moneyLabel = new QLabel("💰 金钱: 0", this);
        moneyLabel->setStyleSheet("color:#ffd166;font:16px bold;background:#0f3460;padding:6px;border-radius:6px;");
        moneyLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(moneyLabel);

        auto* scr = new QScrollArea(this);
        scr->setWidgetResizable(true);
        scr->setStyleSheet("border:none;background:transparent;");
        handContainer = new QWidget();
        handLayout = new QHBoxLayout(handContainer);
        handLayout->setContentsMargins(10, 10, 10, 10);
        handLayout->setSpacing(2);
        handLayout->addStretch();
        scr->setWidget(handContainer);
        l->addWidget(scr, 1);

        auto* cl = new QHBoxLayout();
        playBtn = new QPushButton("出牌 (PLAY 8-14)");
        discardBtn = new QPushButton("弃牌 (DISCARD 1-14)");
        backBtn = new QPushButton("返回菜单");
        for (auto* b : {playBtn, discardBtn, backBtn}) {
            b->setFixedSize(160, 45);
            b->setStyleSheet("background:#4ecca3;color:#0a1929;font:14px bold;border-radius:6px;border:none;");
            cl->addWidget(b);
        }
        cl->addStretch();
        l->addLayout(cl);

        connect(playBtn, &QPushButton::clicked, this, [this]() {
            auto s = getSelectionResult();
            if (s.playedSet.size() < 8 || s.playedSet.size() > 14) return;
            emit playRequested(s.playedSet, s.playOrder);
        });
        connect(discardBtn, &QPushButton::clicked, this, [this]() {
            QVector<Tile> s;
            for (int i = 0; i < isSelected.size(); ++i)
                if (isSelected[i]) s.append(handTiles[i]);
            if (s.isEmpty() || s.size() > 14) return;
            emit discardRequested(std::vector<Tile>(s.begin(), s.end()));
        });
        connect(backBtn, &QPushButton::clicked, this, &GameView::returnToMenuRequested);

        scorePopup = new ScorePopupView(this);
        connect(scorePopup, &ScorePopupView::dismissed, this, [this]() {
            waitForPopup = false;
            if (!pendingHand.empty()) {
                isAnimating = true;
                std::vector<Tile> oldVec(previousHand.begin(), previousHand.end());
                std::vector<Tile> newVec = pendingHand;
                std::vector<Tile> toDiscard, toDraw;

                for (const auto& tile : oldVec) {
                    auto it = std::find(newVec.begin(), newVec.end(), tile);
                    if (it != newVec.end()) newVec.erase(it);
                    else toDiscard.push_back(tile);
                }
                toDraw = newVec;

                playDiscardAnimation(toDiscard, toDraw, pendingHand);
                pendingHand.clear();
            }
        });
    }

    void rebuildHandUI(const std::vector<Tile>& h, bool po = false) {
        qDeleteAll(tileLabels);
        tileLabels.clear();
        handTiles.clear();
        isSelected.clear();

        std::vector<Tile> r = h;
        if (!po) std::sort(r.begin(), r.end(), [](const Tile& a, const Tile& b) {
                auto sr = [](TileSuit s) {
                    switch (s) {
                    case TileSuit::MAN: return 0;
                    case TileSuit::PIN: return 1;
                    case TileSuit::SOU: return 2;
                    case TileSuit::ZI: return 3;
                    default: return 4;
                    }
                };
                int sa = sr(a.suit), sb = sr(b.suit);
                if (sa != sb) return sa < sb;
                double va = a.isRed ? 4.5 : (double)a.value, vb = b.isRed ? 4.5 : (double)b.value;
                return va < vb;
            });

        for (size_t i = 0; i < r.size(); ++i) {
            const auto& t = r[i];
            handTiles.append(t);
            isSelected.append(false);

            TileLabel* lb = new TileLabel(t, this);
            lb->setDragIndex(i);

            int idx = i;
            connect(lb, &TileLabel::clicked, this, [this, idx, t]() {
                if (idx < isSelected.size()) {
                    isSelected[idx] = !isSelected[idx];
                    if (isSelected[idx]) clickedOrder.append(t);
                    else clickedOrder.removeOne(t);
                    refreshTileStyles();
                }
            });

            connect(lb, &TileLabel::dragReleased, this, [this](int si) {
                if (si < 0 || si >= tileLabels.size()) return;
                QPoint c = handContainer->mapFromGlobal(QCursor::pos());
                int ti = si;
                for (int i = 0; i < tileLabels.size(); ++i) {
                    if (i == si) continue;
                    int le = tileLabels[i]->geometry().left(), ri = tileLabels[i]->geometry().right();
                    if (c.x() >= le && c.x() <= ri) { ti = i; break; }
                    if (c.x() < le) { ti = i; break; }
                    ti = i + 1;
                }
                if (ti > si) ti--;
                if (ti != si) {
                    Tile m = handTiles[si];
                    handTiles.removeAt(si);
                    handTiles.insert(ti, m);
                    rebuildHandUI(std::vector<Tile>(handTiles.begin(), handTiles.end()), true);
                }
            });

            handLayout->insertWidget(handLayout->count() - 1, lb);
            tileLabels.append(lb);
        }
    }

    void refreshTileStyles() {
        for (int i = 0; i < tileLabels.size(); ++i) {
            TileLabel* lb = tileLabels[i];
            if (isSelected[i]) {
                if (!lb->pixmap(Qt::ReturnByValue).isNull()) lb->setStyleSheet("QLabel{border:3px solid #ff6b6b;border-radius:8px;}");
                else lb->setStyleSheet("QLabel{background:#e94560;color:white;border:2px solid #ff6b6b;border-radius:6px;font:16px bold;}");
            } else {
                if (!lb->pixmap(Qt::ReturnByValue).isNull()) lb->setStyleSheet("QLabel{border:none;}");
                else lb->setStyleSheet("QLabel{background:#2a2a4a;color:#ddd;border:2px solid #444;border-radius:6px;font:16px bold;}");
            }
        }
    }

    void playDiscardAnimation(const std::vector<Tile>& toDiscard,
                              const std::vector<Tile>& toDraw,
                              const std::vector<Tile>& finalHand) {
        QParallelAnimationGroup* discardGroup = new QParallelAnimationGroup(this);
        bool hasDiscardAnim = false;

        for (int i = 0; i < tileLabels.size(); ++i) {
            if (isSelected[i]) {
                hasDiscardAnim = true;
                TileLabel* lbl = tileLabels[i];
                QParallelAnimationGroup* cardAnim = new QParallelAnimationGroup();

                auto* moveAnim = new QPropertyAnimation(lbl, "pos");
                moveAnim->setDuration(300);
                moveAnim->setStartValue(lbl->pos());
                moveAnim->setEndValue(QPoint(lbl->pos().x(), lbl->pos().y() - 50));

                auto* opacityAnim = new QPropertyAnimation(lbl, "windowOpacity");
                opacityAnim->setDuration(300);
                opacityAnim->setEndValue(0.0);

                cardAnim->addAnimation(moveAnim);
                cardAnim->addAnimation(opacityAnim);
                discardGroup->addAnimation(cardAnim);
            }
        }

        auto startDrawPhase = [this, finalHand]() {
            rebuildHandUI(finalHand);
            previousHand = QVector<Tile>(finalHand.begin(), finalHand.end());

            handLayout->activate();
            qApp->processEvents();

            QParallelAnimationGroup* drawGroup = new QParallelAnimationGroup();
            handLayout->setEnabled(false);
            bool hasDrawAnim = false;

            for (auto* lbl : tileLabels) {
                hasDrawAnim = true;
                QPoint finalPos = lbl->pos();
                lbl->move(finalPos.x(), finalPos.y() - 80);
                lbl->setWindowOpacity(0.0);

                auto* dropAnim = new QPropertyAnimation(lbl, "pos");
                dropAnim->setDuration(400);
                dropAnim->setStartValue(lbl->pos());
                dropAnim->setEndValue(finalPos);
                dropAnim->setEasingCurve(QEasingCurve::OutBounce);

                auto* fadeAnim = new QPropertyAnimation(lbl, "windowOpacity");
                fadeAnim->setDuration(400);
                fadeAnim->setEndValue(1.0);

                drawGroup->addAnimation(dropAnim);
                drawGroup->addAnimation(fadeAnim);
            }

            if (!hasDrawAnim) {
                handLayout->setEnabled(true);
                isAnimating = false;
                checkPendingHand();
                return;
            }

            connect(drawGroup, &QParallelAnimationGroup::finished, this, [this]() {
                handLayout->setEnabled(true);
                isAnimating = false;
                checkPendingHand();
            });
            drawGroup->start();
        };

        if (hasDiscardAnim) {
            connect(discardGroup, &QParallelAnimationGroup::finished, this, startDrawPhase);
            discardGroup->start();
        } else {
            delete discardGroup;
            startDrawPhase();
        }
    }

    void checkPendingHand() {
        if (!pendingHand.empty()) {
            auto nextHand = pendingHand;
            pendingHand.clear();
            QTimer::singleShot(0, this, [this, nextHand]() {
                refreshHand(nextHand);
            });
        }
    }

private:
    QLabel *hudLabel = nullptr, *actionLabel = nullptr, *levelLabel = nullptr, *windLabel = nullptr, *doraLabel = nullptr;
    QHBoxLayout* handLayout = nullptr;
    QWidget* handContainer = nullptr;
    QVector<TileLabel*> tileLabels;
    QVector<Tile> handTiles;
    QVector<bool> isSelected;
    QVector<Tile> clickedOrder;
    QPushButton *playBtn = nullptr, *discardBtn = nullptr, *backBtn = nullptr;
    QVector<Tile> previousHand;
    bool isAnimating = false;
    ScorePopupView* scorePopup = nullptr;
    std::vector<Tile> pendingHand;
    bool waitForPopup = false;
    QLabel* moneyLabel = nullptr;
};

// ============ 主窗口实现 ============
Widget::Widget(QWidget* p) : QWidget(p) {
    QScreen* sc = this->screen() ? this->screen() : QGuiApplication::primaryScreen();
    qreal d = sc->logicalDotsPerInch();
    if (d > 96) setFixedSize(static_cast<int>(1280 * d / 96), static_cast<int>(720 * d / 96));
    else setFixedSize(1280, 720);
    setupUI();
}

Widget::~Widget() = default;

void Widget::setupUI() {
    setWindowTitle("Riichi_Reach");
    QScreen* sc = QGuiApplication::primaryScreen();
    setFixedSize(static_cast<int>(sc->geometry().width() * 0.7), static_cast<int>(sc->geometry().height() * 0.7));
    stackedViews = new QStackedWidget(this);
    gameMgr = new GameManager(this);
    menuView = new MenuView(this);
    gameView = new GameView(this);
    transView = new TransitionView(this);
    shopView = new ShopView(this);
    resultView = new ResultView(this);

    stackedViews->addWidget(menuView);
    stackedViews->addWidget(gameView);
    stackedViews->addWidget(transView);
    stackedViews->addWidget(shopView);
    stackedViews->addWidget(resultView);
    stackedViews->setCurrentIndex(0);

    connect(menuView, &MenuView::startRequested, gameMgr, &GameManager::startLevel);
    connect(menuView, &MenuView::startRequested, [this]() { stackedViews->setCurrentIndex(1); });
    connect(gameView, &GameView::returnToMenuRequested, [this]() { stackedViews->setCurrentIndex(0); });

    connect(gameView, &GameView::playRequested, [this](const std::vector<Tile>& ps, const std::vector<Tile>& po) {
        if (gameMgr->tryPlay(ps, po)) gameView->clearSelection();
    });
    connect(gameView, &GameView::discardRequested, [this](const std::vector<Tile>& s) {
        if (gameMgr->tryDiscard(s)) gameView->clearSelection();
    });

    connect(gameMgr, &GameManager::scoreUpdated, gameView, &GameView::updateHUD);
    connect(gameMgr, &GameManager::actionsUpdated, gameView, &GameView::updateActions);
    connect(gameMgr, &GameManager::levelInfoUpdated, gameView, &GameView::updateLevelInfo);
    connect(gameMgr, &GameManager::doraInfoUpdated, gameView, &GameView::updateDora);
    connect(gameMgr, &GameManager::handUpdated, gameView, [this]() {
        gameView->refreshHand(gameMgr->getHand());
    });
    connect(gameMgr, &GameManager::moneyUpdated, gameView, &GameView::updateMoney);

    connect(gameMgr, &GameManager::levelCleared, [this]() {
        transView->play("LEVEL CLEARED!", true);
        stackedViews->setCurrentWidget(transView);

        // 🔹 计算金币奖励（此时关卡状态未变）
        int baseReward = 4 + gameMgr->getCurrentTier();
        int playsLeft = gameMgr->getPlaysLeft();
        int playCountReward = playsLeft * 2;
        int scoreBonus = (gameMgr->getScore() >= gameMgr->getTargetScore() * 2) ? baseReward : 0;
        int totalReward = baseReward + playCountReward + scoreBonus;

        // 🔹 【关键修复】外层 lambda 显式捕获所有分项变量
        QTimer::singleShot(1500, this, [this, totalReward, baseReward, playCountReward, scoreBonus]() {
            // 1. 显示计分弹窗
            const ScoreResult& realResult = gameMgr->getLastPlayResult();
            const std::vector<Tile>& realPlayed = gameMgr->getLastPlayedTiles();
            gameView->showScorePopup(realResult, realPlayed, gameMgr->getHand());

            // 🔹 传递关卡统计
            const LevelStats& stats = gameMgr->getLastLevelStats();
            if (stats.finalScore == 0 && gameMgr->getScore() > 0) {
                LevelStats fixedStats = stats;
                fixedStats.finalScore = gameMgr->getScore();
                fixedStats.playsLeft = gameMgr->getPlaysLeft();
                fixedStats.discardsLeft = gameMgr->getDiscardsLeft();
                shopView->setLevelStats(fixedStats, gameMgr->getSessionHighYakus());
            } else {
                shopView->setLevelStats(stats, gameMgr->getSessionHighYakus());
            }

            // 🔹 快照当前金钱
            int moneySnap = gameMgr->getMoney();

            // 2. 弹窗关闭后切换商店（内层 lambda 直接使用外层捕获的值）
            gameView->connectPopupDismissed(this, [this, totalReward, baseReward, playCountReward, scoreBonus, moneySnap]() {
                shopView->showRewardInfo(baseReward, playCountReward, scoreBonus, totalReward, moneySnap);
                stackedViews->setCurrentWidget(shopView);
            });
        });
    });

    // 🔹 关键修复：NEXT LEVEL 不再调用 startLevel()，避免重置 tier/level
    connect(shopView, &ShopView::nextLevelRequested, [this]() {
        // advanceLevel() 已在 checkLevelEnd() 中执行，此处只需切回游戏视图
        stackedViews->setCurrentWidget(gameView);
    });

    connect(gameMgr, &GameManager::gameOver, [this]() {
        resultView->setStats(gameMgr->getScore(), gameMgr->getLastPlayScore());
        stackedViews->setCurrentWidget(resultView);
    });
    connect(resultView, &ResultView::menuRequested, [this]() {
        stackedViews->setCurrentWidget(menuView);
    });

    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(stackedViews);

    connect(gameMgr, &GameManager::scorePopupRequested, this, [this](
                                                                  const ScoreResult& res,
                                                                  const std::vector<Tile>& played,
                                                                  const std::vector<Tile>& currentHand) {
        gameView->showScorePopup(res, played, currentHand);
    });
    connect(gameMgr, &GameManager::levelCheckRequested, this, [this]() {
        gameMgr->checkLevelEnd();
    });
    connect(gameMgr, &GameManager::gameCleared, [this]() {
        qDebug() << "[🎊 CONGRATULATIONS!] Game Cleared!";
        resultView->setStats(gameMgr->getScore(), gameMgr->getLastPlayScore());
        stackedViews->setCurrentWidget(resultView);
    });
}

#include "widget.moc"
