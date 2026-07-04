<<<<<<< HEAD
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
=======
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QStackedWidget;
class GameManager;

// 🔹 前置声明视图类（虽然它们在 cpp 中定义，但我们需要指针类型）
class MenuView;
class GameView;
class TransitionView;
class ShopView;
class ResultView;
class ClearRewardView;  // 🔹 新增

class Widget : public QWidget {
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private:
    void setupUI();

    QStackedWidget* stackedViews;
    GameManager*    gameMgr;

    // 🔹 使用具体类型而不是 QWidget*
    MenuView*       menuView;
    GameView*       gameView;
    TransitionView* transView;
    ShopView*       shopView;
    ResultView*     resultView;
    ClearRewardView* clearRewardView;  // 🔹 新增
};

#endif // WIDGET_H
>>>>>>> 5fcafe7 (最终版代码提交)
