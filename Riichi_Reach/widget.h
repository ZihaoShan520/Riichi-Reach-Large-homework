#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QStackedWidget;
class GameManager;
class MenuView;  // 前置声明
class GameView;  // 前置声明

class Widget : public QWidget {
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private:
    void setupUI();

    QStackedWidget* stackedViews;
    GameManager*    gameMgr;
    MenuView*       menuView;  // ✅ 修复：声明为成员变量
    GameView*       gameView;  // ✅ 修复：声明为成员变量
};

#endif // WIDGET_H
