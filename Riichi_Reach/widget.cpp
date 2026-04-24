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

// ============ 可点击的牌面标签 ============
class TileLabel : public QLabel {
    Q_OBJECT
public:
    explicit TileLabel(const QString& text, QWidget* parent = nullptr) : QLabel(text, parent) {
        setMouseTracking(true);
    }
    void setDragIndex(int idx) { dragIndex = idx; }

signals:
    void clicked();
    void dragReleased(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override {
        startDragPos = event->globalPosition().toPoint();
        QLabel::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        QPoint endPos = event->globalPosition().toPoint();
        if (QPoint(endPos - startDragPos).manhattanLength() > 12) {
            emit dragReleased(dragIndex);
        } else {
            emit clicked();
        }
    }

private:
    QPoint startDragPos;
    int dragIndex = -1;
};

// ============ 视图1：主菜单 ============
class MenuView : public QWidget {
    Q_OBJECT
public:
    explicit MenuView(QWidget* parent = nullptr) : QWidget(parent) {
        setStyleSheet("background-color: #1a1a2e;");
        auto* layout = new QVBoxLayout(this);
        auto* title = new QLabel("Riichi Reach\nMahjong Roguelike", this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("color: white; font-size: 32px; font-weight: bold;");
        layout->addWidget(title);
        auto* btn = new QPushButton("START LEVEL", this);
        btn->setFixedSize(220, 50);
        btn->setStyleSheet(R"(
            QPushButton { background-color: #e94560; color: white; font-size: 18px; font-weight: bold; border-radius: 8px; border: none; }
            QPushButton:hover { background-color: #ff6b6b; }
            QPushButton:pressed { background-color: #c0392b; }
        )");
        layout->addWidget(btn, 0, Qt::AlignCenter);
        layout->addStretch();
        connect(btn, &QPushButton::clicked, this, [this]() { emit startRequested(); });
    }
signals:
    void startRequested();
};

// ============ 视图2：游戏区 ============
class GameView : public QWidget {
    Q_OBJECT
public:
    explicit GameView(QWidget* parent = nullptr) : QWidget(parent) {
        setStyleSheet("background-color: #16213e;");
        setupUI();
    }

    // 🔹 新增：返回双列表结构
    struct SelectionResult {
        std::vector<Tile> playedSet;   // 用于番型/和牌判定
        std::vector<Tile> playOrder;   // 用于无役前5张计分（保持点击顺序）
    };

    SelectionResult getSelectionResult() const {
        SelectionResult res;
        for (int i = 0; i < isSelected.size(); ++i) {
            if (isSelected[i]) {
                res.playedSet.push_back(handTiles[i]);
            }
        }
        res.playOrder = std::vector<Tile>(clickedOrder.begin(), clickedOrder.end());
        return res;
    }

    void clearSelection() {
        for (int i = 0; i < isSelected.size(); ++i) isSelected[i] = false;
        clickedOrder.clear();  // ✅ 清空点击顺序
        refreshTileStyles();
    }

public slots:
    void updateHUD(int score, int target) {
        hudLabel->setText(QString("Score: %1 / %2").arg(score).arg(target));
    }
    void updateActions(int plays, int discards) {
        actionLabel->setText(QString("Plays: %1 | Discards: %2").arg(plays).arg(discards));
    }
    void refreshHand(const std::vector<Tile>& hand) {
        rebuildHandUI(hand, false);
        clearSelection();
    }

signals:
    void returnToMenuRequested();
    void playRequested(const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder);
    void discardRequested(const std::vector<Tile>& tiles);

private:
    void setupUI() {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(12);

        hudLabel = new QLabel("Score: 0 / 2000", this);
        hudLabel->setStyleSheet("color: white; font-size: 18px; background: #0f3460; padding: 8px; border-radius: 6px;");
        hudLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(hudLabel);

        actionLabel = new QLabel("Plays: 4 | Discards: 3", this);
        actionLabel->setStyleSheet("color: #a0c4ff; font-size: 14px; background: #0f3460; padding: 6px; border-radius: 6px;");
        actionLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(actionLabel);

        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("border: none; background: transparent;");

        handContainerWidget = new QWidget();
        handLayout = new QHBoxLayout(handContainerWidget);
        handLayout->setContentsMargins(10, 10, 10, 10);
        handLayout->setSpacing(8);
        handLayout->addStretch();
        scroll->setWidget(handContainerWidget);
        mainLayout->addWidget(scroll, 1);

        auto* ctrlLayout = new QHBoxLayout();
        playBtn = new QPushButton("出牌 (PLAY 8-14)");
        discardBtn = new QPushButton("弃牌 (DISCARD 1-14)");
        backBtn = new QPushButton("返回菜单");

        for (auto* btn : {playBtn, discardBtn, backBtn}) {
            btn->setFixedSize(160, 45);
            btn->setStyleSheet("background: #4ecca3; color: #0a1929; font-weight: bold; border-radius: 6px; border: none; font-size: 14px;");
            ctrlLayout->addWidget(btn);
        }
        ctrlLayout->addStretch();
        mainLayout->addLayout(ctrlLayout);

        connect(playBtn, &QPushButton::clicked, this, [this]() {
            auto sel = getSelectionResult();
            if (sel.playedSet.size() < 8 || sel.playedSet.size() > 14) {
                qDebug() << "[WARN] Play requires 8~14 tiles!";
                return;
            }
            emit playRequested(sel.playedSet, sel.playOrder);  // ✅ 传递双参数
        });

        connect(discardBtn, &QPushButton::clicked, this, [this]() {
            // ✅ 内联收集已选牌（弃牌不需要顺序，直接用 isSelected + handTiles）
            QVector<Tile> sel;
            for (int i = 0; i < isSelected.size(); ++i) {
                if (isSelected[i]) sel.append(handTiles[i]);
            }

            if (sel.isEmpty() || sel.size() > 14) {
                qDebug() << "[WARN] Discard requires 1~14 tiles!";
                return;
            }
            std::vector<Tile> stdSel(sel.begin(), sel.end());
            emit discardRequested(stdSel);
        });

        connect(backBtn, &QPushButton::clicked, this, &GameView::returnToMenuRequested);
    }

    void rebuildHandUI(const std::vector<Tile>& hand, bool preserveOrder = false) {
        qDeleteAll(tileLabels);
        tileLabels.clear();
        handTiles.clear();
        isSelected.clear();

        std::vector<Tile> renderHand = hand;
        if (!preserveOrder) {
            std::sort(renderHand.begin(), renderHand.end(), [](const Tile& a, const Tile& b) {
                auto suitRank = [](TileSuit s) {
                    switch(s) {
                    case TileSuit::MAN: return 0;
                    case TileSuit::PIN: return 1;
                    case TileSuit::SOU: return 2;
                    case TileSuit::ZI:  return 3;
                    default: return 4;
                    }
                };
                int sa = suitRank(a.suit), sb = suitRank(b.suit);
                if (sa != sb) return sa < sb;
                double va = a.isRed ? 4.5 : static_cast<double>(a.value);
                double vb = b.isRed ? 4.5 : static_cast<double>(b.value);
                return va < vb;
            });
        }

        for (size_t i = 0; i < renderHand.size(); ++i) {
            const auto& t = renderHand[i];
            handTiles.append(t);
            isSelected.append(false);

            auto* label = new TileLabel(t.id(), this);
            label->setFixedSize(60, 80);
            label->setAlignment(Qt::AlignCenter);
            label->setCursor(Qt::PointingHandCursor);
            label->setStyleSheet(R"(
                QLabel { background: #2a2a4a; color: #ddd; border: 2px solid #444; border-radius: 6px; font-size: 16px; font-weight: bold; }
            )");
            label->setDragIndex(static_cast<int>(i));

            int idx = static_cast<int>(i);

            // ✅ 点击时记录/移除顺序
            connect(label, &TileLabel::clicked, this, [this, idx, t = renderHand[idx]]() {
                if (idx < isSelected.size()) {
                    isSelected[idx] = !isSelected[idx];
                    if (isSelected[idx]) {
                        clickedOrder.append(t);
                    } else {
                        clickedOrder.removeOne(t);
                    }
                    refreshTileStyles();
                }
            });

            connect(label, &TileLabel::dragReleased, this, [this](int srcIdx) {
                if (srcIdx < 0 || srcIdx >= tileLabels.size()) return;
                QPoint localCursor = handContainerWidget->mapFromGlobal(QCursor::pos());
                int targetIdx = srcIdx;
                for (int i = 0; i < tileLabels.size(); ++i) {
                    if (i == srcIdx) continue;
                    int left = tileLabels[i]->geometry().left();
                    int right = tileLabels[i]->geometry().right();
                    if (localCursor.x() >= left && localCursor.x() <= right) { targetIdx = i; break; }
                    if (localCursor.x() < left) { targetIdx = i; break; }
                    targetIdx = i + 1;
                }
                if (targetIdx > srcIdx) targetIdx--;
                if (targetIdx != srcIdx) {
                    Tile moved = handTiles[srcIdx];
                    handTiles.removeAt(srcIdx);
                    handTiles.insert(targetIdx, moved);
                    rebuildHandUI(std::vector<Tile>(handTiles.begin(), handTiles.end()), true);
                }
            });

            handLayout->insertWidget(handLayout->count() - 1, label);
            tileLabels.append(label);
        }
    }

    void refreshTileStyles() {
        for (int i = 0; i < tileLabels.size(); ++i) {
            QString style = isSelected[i] ?
                                "QLabel { background: #e94560; color: white; border: 2px solid #ff6b6b; border-radius: 6px; font-size: 16px; font-weight: bold; }" :
                                "QLabel { background: #2a2a4a; color: #ddd; border: 2px solid #444; border-radius: 6px; font-size: 16px; font-weight: bold; }";
            tileLabels[i]->setStyleSheet(style);
        }
    }

private:
    QLabel *hudLabel = nullptr, *actionLabel = nullptr;
    QHBoxLayout* handLayout = nullptr;
    QWidget* handContainerWidget = nullptr;
    QVector<TileLabel*> tileLabels;
    QVector<Tile> handTiles;
    QVector<bool> isSelected;
    QVector<Tile> clickedOrder;  // ✅ 新增：记录点击顺序
    QPushButton *playBtn = nullptr, *discardBtn = nullptr, *backBtn = nullptr;
};

// ============ 主窗口实现 ============
Widget::Widget(QWidget *parent) : QWidget(parent) { setupUI(); }
Widget::~Widget() = default;

void Widget::setupUI() {
    setWindowTitle("Riichi_Reach");
    setFixedSize(1280, 720);

    stackedViews = new QStackedWidget(this);
    gameMgr    = new GameManager(this);
    menuView   = new MenuView(this);
    gameView   = new GameView(this);

    stackedViews->addWidget(menuView);
    stackedViews->addWidget(gameView);
    stackedViews->setCurrentIndex(0);

    connect(menuView, &MenuView::startRequested, gameMgr, &GameManager::startLevel);
    connect(menuView, &MenuView::startRequested, [this]() { stackedViews->setCurrentIndex(1); });
    connect(gameView, &GameView::returnToMenuRequested, [this]() { stackedViews->setCurrentIndex(0); });

    // ✅ 连接双参数信号
    connect(gameView, &GameView::playRequested, [this](const std::vector<Tile>& playedSet, const std::vector<Tile>& playOrder) {
        if (gameMgr->tryPlay(playedSet, playOrder)) gameView->clearSelection();
    });
    connect(gameView, &GameView::discardRequested, [this](const std::vector<Tile>& sel) {
        if (gameMgr->tryDiscard(sel)) gameView->clearSelection();
    });

    connect(gameMgr, &GameManager::scoreUpdated, gameView, &GameView::updateHUD);
    connect(gameMgr, &GameManager::actionsUpdated, gameView, &GameView::updateActions);
    connect(gameMgr, &GameManager::handUpdated, gameView, [this]() {
        gameView->refreshHand(gameMgr->getHand());
    });
    connect(gameMgr, &GameManager::levelCleared, []() { qDebug() << "[UI] Level Cleared!"; });
    connect(gameMgr, &GameManager::gameOver, []() { qDebug() << "[UI] Game Over!"; });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stackedViews);
}

#include "widget.moc"
