// widget.cpp
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

class TileLabel : public QLabel { Q_OBJECT
public: explicit TileLabel(const QString& t, QWidget* p=nullptr):QLabel(t,p){setMouseTracking(true);}
    void setDragIndex(int i){dragIndex=i;}
signals: void clicked(); void dragReleased(int);
protected: void mousePressEvent(QMouseEvent* e)override{startDragPos=e->globalPosition().toPoint();QLabel::mousePressEvent(e);}
    void mouseReleaseEvent(QMouseEvent* e)override{if(QPoint(e->globalPosition().toPoint()-startDragPos).manhattanLength()>12)emit dragReleased(dragIndex);else emit clicked();}
private: QPoint startDragPos; int dragIndex=-1;
};

class MenuView : public QWidget { Q_OBJECT
public: explicit MenuView(QWidget*p=nullptr):QWidget(p){setStyleSheet("background:#1a1a2e;");auto*l=new QVBoxLayout(this);
        auto*t=new QLabel("Riichi Reach\nMahjong Roguelike",this);t->setAlignment(Qt::AlignCenter);t->setStyleSheet("color:white;font:32px bold;");l->addWidget(t);
        auto*b=new QPushButton("START GAME",this);b->setFixedSize(220,50);b->setStyleSheet("QPushButton{background:#e94560;color:white;font:18px bold;border-radius:8px;border:none;}QPushButton:hover{background:#ff6b6b;}");l->addWidget(b,0,Qt::AlignCenter);l->addStretch();
        connect(b,&QPushButton::clicked,this,[this](){emit startRequested();});}
signals: void startRequested();
};

class GameView : public QWidget { Q_OBJECT
public: explicit GameView(QWidget*p=nullptr):QWidget(p){setStyleSheet("background:#16213e;");setupUI();}
    struct SelectionResult{std::vector<Tile> playedSet, playOrder;};
    SelectionResult getSelectionResult()const{SelectionResult r;for(int i=0;i<isSelected.size();++i)if(isSelected[i])r.playedSet.push_back(handTiles[i]);r.playOrder=std::vector<Tile>(clickedOrder.begin(),clickedOrder.end());return r;}
    void clearSelection() {
        for (int i = 0; i < isSelected.size(); ++i) isSelected[i] = false;
        clickedOrder.clear();  // ✅ QVector<Tile>::clear() 正确
        refreshTileStyles();
    }
public slots:
    void updateHUD(int s,int t){hudLabel->setText(QString("Score: %1 / %2").arg(s).arg(t));}
    void updateActions(int p,int d){actionLabel->setText(QString("Plays: %1 | Discards: %2").arg(p).arg(d));}
    void updateLevelInfo(int t,int l,int pw,int sw,bool inf){static const QString wn[]={"","東","南","西","北"};QString it=inf?" [∞]":"";levelLabel->setText(QString("Tier: %1 | Level: %2%3").arg(t).arg(l).arg(it));windLabel->setText(QString("場風: %1 | 自風: %2").arg(wn[pw]).arg(wn[sw]));}
    // 🔹 新增：更新宝牌显示
    void updateDora(Tile ind, Tile dora){
        auto ts=[](const Tile&t)->QString{
            if(t.isRed)return t.suit==TileSuit::MAN?"赤五萬":t.suit==TileSuit::PIN?"赤五筒":"赤五索";
            static const QString s[]={"","一萬","二萬","三萬","四萬","五萬","六萬","七萬","八萬","九萬",
                                        "一筒","二筒","三筒","四筒","五筒","六筒","七筒","八筒","九筒",
                                        "一索","二索","三索","四索","五索","六索","七索","八索","九索",
                                        "東","南","西","北","白","發","中"};
            int i=(t.suit==TileSuit::MAN?t.value:(t.suit==TileSuit::PIN?t.value+9:(t.suit==TileSuit::SOU?t.value+18:t.value+27)));
            return(i>0&&i<35)?s[i]:t.id();
        };
        doraLabel->setText(QString("指示物: %1 | 宝牌: %2").arg(ts(ind)).arg(ts(dora)));
    }
    void refreshHand(const std::vector<Tile>&h){rebuildHandUI(h,false);clearSelection();}
signals: void returnToMenuRequested(); void playRequested(const std::vector<Tile>&,const std::vector<Tile>&); void discardRequested(const std::vector<Tile>&);
private:
    void setupUI(){auto*l=new QVBoxLayout(this);l->setContentsMargins(20,20,20,20);l->setSpacing(12);
        hudLabel=new QLabel("Score: 0 / 2000",this);hudLabel->setStyleSheet("color:white;font:18px;background:#0f3460;padding:8px;border-radius:6px;");hudLabel->setAlignment(Qt::AlignCenter);l->addWidget(hudLabel);
        actionLabel=new QLabel("Plays: 4 | Discards: 3",this);actionLabel->setStyleSheet("color:#a0c4ff;font:14px;background:#0f3460;padding:6px;border-radius:6px;");actionLabel->setAlignment(Qt::AlignCenter);l->addWidget(actionLabel);
        levelLabel=new QLabel("Tier: 1 | Level: 1",this);levelLabel->setStyleSheet("color:#ffd166;font:14px;background:#0f3460;padding:6px;border-radius:6px;");levelLabel->setAlignment(Qt::AlignCenter);l->addWidget(levelLabel);
        windLabel=new QLabel("場風: 東 | 自風: 東",this);windLabel->setStyleSheet("color:#a0c4ff;font:14px;background:#0f3460;padding:6px;border-radius:6px;");windLabel->setAlignment(Qt::AlignCenter);l->addWidget(windLabel);
        // 🔹 新增：宝牌显示标签
        doraLabel=new QLabel("指示物: ? | 宝牌: ?",this);doraLabel->setStyleSheet("color:#f8ad9d;font:14px;background:#0f3460;padding:6px;border-radius:6px;");doraLabel->setAlignment(Qt::AlignCenter);l->addWidget(doraLabel);
        auto*scr=new QScrollArea(this);scr->setWidgetResizable(true);scr->setStyleSheet("border:none;background:transparent;");handContainer=new QWidget();handLayout=new QHBoxLayout(handContainer);handLayout->setContentsMargins(10,10,10,10);handLayout->setSpacing(8);handLayout->addStretch();scr->setWidget(handContainer);l->addWidget(scr,1);
        auto*cl=new QHBoxLayout();playBtn=new QPushButton("出牌 (PLAY 8-14)");discardBtn=new QPushButton("弃牌 (DISCARD 1-14)");backBtn=new QPushButton("返回菜单");
        for(auto*b:{playBtn,discardBtn,backBtn}){b->setFixedSize(160,45);b->setStyleSheet("background:#4ecca3;color:#0a1929;font:14px bold;border-radius:6px;border:none;");cl->addWidget(b);}cl->addStretch();l->addLayout(cl);
        connect(playBtn,&QPushButton::clicked,this,[this](){auto s=getSelectionResult();if(s.playedSet.size()<8||s.playedSet.size()>14){qDebug()<<"[WARN] Play requires 8~14!";return;}emit playRequested(s.playedSet,s.playOrder);});
        connect(discardBtn,&QPushButton::clicked,this,[this](){QVector<Tile>s;for(int i=0;i<isSelected.size();++i)if(isSelected[i])s.append(handTiles[i]);if(s.isEmpty()||s.size()>14){qDebug()<<"[WARN] Discard requires 1~14!";return;}emit discardRequested(std::vector<Tile>(s.begin(),s.end()));});
        connect(backBtn,&QPushButton::clicked,this,&GameView::returnToMenuRequested);}
    void rebuildHandUI(const std::vector<Tile>&h,bool po=false){qDeleteAll(tileLabels);tileLabels.clear();handTiles.clear();isSelected.clear();
        std::vector<Tile>r=h;if(!po)std::sort(r.begin(),r.end(),[](const Tile&a,const Tile&b){auto sr=[](TileSuit s){switch(s){case TileSuit::MAN:return 0;case TileSuit::PIN:return 1;case TileSuit::SOU:return 2;case TileSuit::ZI:return 3;default:return 4;}};int sa=sr(a.suit),sb=sr(b.suit);if(sa!=sb)return sa<sb;double va=a.isRed?4.5:(double)a.value,vb=b.isRed?4.5:(double)b.value;return va<vb;});
        for(size_t i=0;i<r.size();++i){const auto&t=r[i];handTiles.append(t);isSelected.append(false);auto*lb=new TileLabel(t.id(),this);lb->setFixedSize(60,80);lb->setAlignment(Qt::AlignCenter);lb->setCursor(Qt::PointingHandCursor);lb->setStyleSheet("QLabel{background:#2a2a4a;color:#ddd;border:2px solid #444;border-radius:6px;font:16px bold;}");lb->setDragIndex(i);
            int idx=i;connect(lb,&TileLabel::clicked,this,[this,idx,t](){if(idx<isSelected.size()){isSelected[idx]=!isSelected[idx];if(isSelected[idx])clickedOrder.append(t);else clickedOrder.removeOne(t);refreshTileStyles();}});
            connect(lb,&TileLabel::dragReleased,this,[this](int si){if(si<0||si>=tileLabels.size())return;QPoint c=handContainer->mapFromGlobal(QCursor::pos());int ti=si;for(int i=0;i<tileLabels.size();++i){if(i==si)continue;int le=tileLabels[i]->geometry().left(),ri=tileLabels[i]->geometry().right();if(c.x()>=le&&c.x()<=ri){ti=i;break;}if(c.x()<le){ti=i;break;}ti=i+1;}if(ti>si)ti--;if(ti!=si){Tile m=handTiles[si];handTiles.removeAt(si);handTiles.insert(ti,m);rebuildHandUI(std::vector<Tile>(handTiles.begin(),handTiles.end()),true);}});
            handLayout->insertWidget(handLayout->count()-1,lb);tileLabels.append(lb);}}
    void refreshTileStyles(){for(int i=0;i<tileLabels.size();++i)tileLabels[i]->setStyleSheet(isSelected[i]?"QLabel{background:#e94560;color:white;border:2px solid #ff6b6b;border-radius:6px;font:16px bold;}":"QLabel{background:#2a2a4a;color:#ddd;border:2px solid #444;border-radius:6px;font:16px bold;}");}
private: QLabel*hudLabel=nullptr,*actionLabel=nullptr,*levelLabel=nullptr,*windLabel=nullptr,*doraLabel=nullptr;QHBoxLayout*handLayout=nullptr;QWidget*handContainer=nullptr;QVector<TileLabel*>tileLabels;QVector<Tile>handTiles;QVector<bool>isSelected;QVector<Tile>clickedOrder;QPushButton*playBtn=nullptr,*discardBtn=nullptr,*backBtn=nullptr;
};

Widget::Widget(QWidget*p):QWidget(p){QScreen*sc=this->screen()?this->screen():QGuiApplication::primaryScreen();qreal d=sc->logicalDotsPerInch();if(d>96)setFixedSize(static_cast<int>(1280*d/96),static_cast<int>(720*d/96));else setFixedSize(1280,720);setupUI();}
Widget::~Widget()=default;
void Widget::setupUI(){setWindowTitle("Riichi_Reach");QScreen*sc=QGuiApplication::primaryScreen();setFixedSize(static_cast<int>(sc->geometry().width()*0.7),static_cast<int>(sc->geometry().height()*0.7));
    stackedViews=new QStackedWidget(this);gameMgr=new GameManager(this);menuView=new MenuView(this);gameView=new GameView(this);
    stackedViews->addWidget(menuView);stackedViews->addWidget(gameView);stackedViews->setCurrentIndex(0);
    connect(menuView,&MenuView::startRequested,gameMgr,&GameManager::startLevel);connect(menuView,&MenuView::startRequested,[this](){stackedViews->setCurrentIndex(1);});
    connect(gameView,&GameView::returnToMenuRequested,[this](){stackedViews->setCurrentIndex(0);});
    connect(gameView,&GameView::playRequested,[this](const std::vector<Tile>&ps,const std::vector<Tile>&po){if(gameMgr->tryPlay(ps,po))gameView->clearSelection();});
    connect(gameView,&GameView::discardRequested,[this](const std::vector<Tile>&s){if(gameMgr->tryDiscard(s))gameView->clearSelection();});
    connect(gameMgr,&GameManager::scoreUpdated,gameView,&GameView::updateHUD);connect(gameMgr,&GameManager::actionsUpdated,gameView,&GameView::updateActions);
    connect(gameMgr,&GameManager::handUpdated,gameView,[this](){gameView->refreshHand(gameMgr->getHand());});
    connect(gameMgr,&GameManager::levelInfoUpdated,gameView,&GameView::updateLevelInfo);
    connect(gameMgr,&GameManager::doraInfoUpdated,gameView,&GameView::updateDora);  // 🔹 连接宝牌信号
    connect(gameMgr,&GameManager::levelCleared,[](){qDebug()<<"[UI] Level Cleared!";});connect(gameMgr,&GameManager::gameOver,[](){qDebug()<<"[UI] Game Over!";});
    auto*l=new QVBoxLayout(this);l->setContentsMargins(0,0,0,0);l->addWidget(stackedViews);}
#include "widget.moc"
