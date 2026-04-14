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

// ============ 可点击的牌面标签 ============
class TileLabel : public QLabel {
    Q_OBJECT
public:
    explicit TileLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent) {}

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QLabel::mousePressEvent(event);
        emit clicked();
    }
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

        auto* btn = new QPushButton("START GAME", this);
        btn->setFixedSize(220, 50);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: #e94560; color: white;
                font-size: 18px; font-weight: bold;
                border-radius: 8px; border: none;
            }
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

    QVector<Tile> getSelectedTiles() const {
        QVector<Tile> selected;
        for (int i = 0; i < isSelected.size(); ++i)
            if (isSelected[i]) selected.append(handTiles[i]);
        return selected;
    }

    void clearSelection() {
        for (int i = 0; i < isSelected.size(); ++i) isSelected[i] = false;
        refreshTileStyles();
    }

public slots:
    void updateHUD(int score, int target, int turns, int floor) {
        hudLabel->setText(QString("Score: %1/%2 | Turns: %3 | Floor: %4")
                              .arg(score).arg(target).arg(turns).arg(floor));
    }

    void refreshHand(const std::vector<Tile>& hand) {
        rebuildHandUI(hand);
        clearSelection();
    }

signals:
    void returnToMenuRequested();
    void playRequested();
    void drawRequested();
    void skipRequested();

private:
    void setupUI() {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        hudLabel = new QLabel("Score: 0/1000 | Turns: 10 | Floor: 1", this);
        hudLabel->setStyleSheet("color: white; font-size: 16px; background: #0f3460; padding: 8px; border-radius: 6px;");
        hudLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(hudLabel);

        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("border: none; background: transparent;");

        auto* handContainer = new QWidget();
        handLayout = new QHBoxLayout(handContainer);
        handLayout->setContentsMargins(10, 10, 10, 10);
        handLayout->setSpacing(8);
        handLayout->addStretch();
        scroll->setWidget(handContainer);
        mainLayout->addWidget(scroll, 1);

        auto* ctrlLayout = new QHBoxLayout();
        playBtn = new QPushButton("PLAY SELECTED");
        drawBtn = new QPushButton("DRAW");
        skipBtn = new QPushButton("SKIP");
        backBtn = new QPushButton("MENU");

        for (auto* btn : {playBtn, drawBtn, skipBtn, backBtn}) {
            btn->setFixedSize(120, 40);
            btn->setStyleSheet("background: #4ecca3; color: #0a1929; font-weight: bold; border-radius: 6px; border: none;");
            ctrlLayout->addWidget(btn);
        }
        ctrlLayout->addStretch();
        mainLayout->addLayout(ctrlLayout);

        connect(playBtn, &QPushButton::clicked, this, &GameView::playRequested);
        connect(drawBtn, &QPushButton::clicked, this, &GameView::drawRequested);
        connect(skipBtn, &QPushButton::clicked, this, &GameView::skipRequested);
        connect(backBtn, &QPushButton::clicked, this, &GameView::returnToMenuRequested);
    }

    void rebuildHandUI(const std::vector<Tile>& hand) {
        qDeleteAll(tileLabels);
        tileLabels.clear();
        handTiles.clear();
        isSelected.clear();

        for (const auto& t : hand) {
            handTiles.append(t);
            isSelected.append(false);

            auto* label = new TileLabel(t.id(), this);  // ✅ 改用 TileLabel
            label->setFixedSize(60, 80);
            label->setAlignment(Qt::AlignCenter);
            label->setCursor(Qt::PointingHandCursor);
            label->setStyleSheet(R"(
            QLabel {
                background: #2a2a4a; color: #ddd;
                border: 2px solid #444; border-radius: 6px;
                font-size: 16px; font-weight: bold;
            }
        )");

            int idx = tileLabels.size();
            connect(label, &TileLabel::clicked, this, [this, idx]() {  // ✅ 连接 clicked 信号
                if (idx < isSelected.size()) {
                    isSelected[idx] = !isSelected[idx];
                    refreshTileStyles();
                }
            });

            handLayout->insertWidget(handLayout->count() - 1, label);
            tileLabels.append(label);  // 注意：这里需要把 TileLabel* 存起来
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
    QLabel* hudLabel = nullptr;
    QHBoxLayout* handLayout = nullptr;
    QVector<TileLabel*> tileLabels;
    QVector<Tile> handTiles;
    QVector<bool> isSelected;
    QPushButton *playBtn = nullptr, *drawBtn = nullptr, *skipBtn = nullptr, *backBtn = nullptr;
};

// ============ 主窗口实现 ============
Widget::Widget(QWidget *parent) : QWidget(parent) {
    setupUI();
}

Widget::~Widget() = default;

void Widget::setupUI() {
    setWindowTitle("Riichi_Reach");
    setFixedSize(1280, 720);

    stackedViews = new QStackedWidget(this);
    gameMgr    = new GameManager(this);
    menuView   = new MenuView(this);   // ✅ 实例化成员变量
    gameView   = new GameView(this);   // ✅ 实例化成员变量

    stackedViews->addWidget(menuView);
    stackedViews->addWidget(gameView);
    stackedViews->setCurrentIndex(0);

    // 🔗 路由控制
    connect(menuView, &MenuView::startRequested, gameMgr, &GameManager::startGame);
    connect(menuView, &MenuView::startRequested, [this]() { stackedViews->setCurrentIndex(1); });
    connect(gameView, &GameView::returnToMenuRequested, [this]() { stackedViews->setCurrentIndex(0); });

    // 🔗 操作指令 → 逻辑层
    connect(gameView, &GameView::drawRequested, gameMgr, &GameManager::drawTile);
    connect(gameView, &GameView::skipRequested, gameMgr, &GameManager::skipTurn);
    connect(gameView, &GameView::playRequested, [this]() {
        auto selected = gameView->getSelectedTiles();
        if (selected.isEmpty()) {
            qDebug() << "[WARN] No tiles selected!";
            return;
        }
        std::vector<Tile> stdSel(selected.begin(), selected.end());
        gameMgr->playTiles(stdSel);
        gameView->clearSelection();
    });

    // 🔗 逻辑层 → UI刷新 (HUD & 手牌)
    // 注：GameManager 需暴露 getHand()，已在下方说明补充
    connect(gameMgr, &GameManager::scoreUpdated, this, [this](int cur, int tgt) {
        gameView->updateHUD(cur, tgt, gameMgr->getTurnsLeft(), gameMgr->getFloor());
    });
    connect(gameMgr, &GameManager::turnsUpdated, this, [this](int t) {
        gameView->updateHUD(gameMgr->getScore(), gameMgr->getTargetScore(), t, gameMgr->getFloor());
    });
    connect(gameMgr, &GameManager::floorChanged, this, [this](int f) {
        gameView->updateHUD(gameMgr->getScore(), gameMgr->getTargetScore(), gameMgr->getTurnsLeft(), f);
    });
    connect(gameMgr, &GameManager::handChanged, this, [this]() {
        gameView->refreshHand(gameMgr->getHand());
    });

    // 🔗 调试输出 (纯ASCII)
    connect(gameMgr, &GameManager::tileDrawn, [](const Tile& t) {
        qDebug() << "[DRAW]" << t.id().toStdString().c_str();
    });
    connect(gameMgr, &GameManager::gameOver, [](bool win) {
        qDebug() << (win ? "[WIN] Floor Cleared!" : "[LOSE] Game Over");
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stackedViews);
}

#include "widget.moc"
