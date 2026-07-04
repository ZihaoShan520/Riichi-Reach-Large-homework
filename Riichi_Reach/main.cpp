<<<<<<< HEAD
#include <QApplication>
#include <QScreen>
#include <QStyleFactory>
#include <QDir>
#include "widget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 启用高DPI缩放支持 - 最重要的设置
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // 设置高DPI缩放因子舍入策略
    app.setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Round);

    // 获取主屏幕信息，用于调试
    QScreen* primaryScreen = app.primaryScreen();
    qreal dpi = primaryScreen->logicalDotsPerInch();
    qDebug() << "Primary screen DPI:" << dpi;
    qDebug() << "Screen size:" << primaryScreen->size();

    // 设置应用样式以获得更好的跨平台兼容性
    if (!QStyleFactory::keys().isEmpty()) {
        app.setStyle(QStyleFactory::create("Fusion"));
    }

    // 设置应用字体大小以适应不同DPI
    QFont defaultFont = app.font();
    if (dpi > 96) { // 高DPI显示器
        defaultFont.setPointSize(static_cast<int>(defaultFont.pointSize() * dpi / 96.0));
    }
    app.setFont(defaultFont);

    Widget w;

    // 根据屏幕尺寸动态调整窗口大小（如果需要的话）
    QRect screenGeometry = primaryScreen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // 计算合适的窗口大小（不超过屏幕的80%）
    int windowWidth = qMin(1280, static_cast<int>(screenWidth * 0.8));
    int windowHeight = qMin(720, static_cast<int>(screenHeight * 0.8));

    // 如果屏幕太小，使用较小的窗口
    if (screenWidth < 1280 || screenHeight < 720) {
        w.resize(windowWidth, windowHeight);
    } else {
        w.resize(1280, 720); // 原始大小
    }

    // 居中显示窗口
    w.move((screenWidth - w.width()) / 2,
           (screenHeight - w.height()) / 2);

    w.show();
    return app.exec();
}
=======
#include <QApplication>
#include <QScreen>
#include <QStyleFactory>
#include <QDir>
#include "widget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 启用高DPI缩放支持 - 最重要的设置
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // 设置高DPI缩放因子舍入策略
    app.setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Round);

    // 获取主屏幕信息，用于调试
    QScreen* primaryScreen = app.primaryScreen();
    qreal dpi = primaryScreen->logicalDotsPerInch();
    qDebug() << "Primary screen DPI:" << dpi;
    qDebug() << "Screen size:" << primaryScreen->size();

    // 设置应用样式以获得更好的跨平台兼容性
    if (!QStyleFactory::keys().isEmpty()) {
        app.setStyle(QStyleFactory::create("Fusion"));
    }

    // 设置应用字体大小以适应不同DPI
    QFont defaultFont = app.font();
    if (dpi > 96) { // 高DPI显示器
        defaultFont.setPointSize(static_cast<int>(defaultFont.pointSize() * dpi / 96.0));
    }
    app.setFont(defaultFont);

    Widget w;

    // 根据屏幕尺寸动态调整窗口大小（如果需要的话）
    QRect screenGeometry = primaryScreen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // 计算合适的窗口大小（不超过屏幕的80%）
    int windowWidth = qMin(1280, static_cast<int>(screenWidth * 0.8));
    int windowHeight = qMin(720, static_cast<int>(screenHeight * 0.8));

    // 如果屏幕太小，使用较小的窗口
    if (screenWidth < 1280 || screenHeight < 720) {
        w.resize(windowWidth, windowHeight);
    } else {
        w.resize(1280, 720); // 原始大小
    }

    // 居中显示窗口
    w.move((screenWidth - w.width()) / 2,
           (screenHeight - w.height()) / 2);

    w.show();
    return app.exec();
}
>>>>>>> 5fcafe7 (最终版代码提交)
