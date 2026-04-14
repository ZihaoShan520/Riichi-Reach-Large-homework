#include <QApplication>
#include "widget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Qt6 高DPI适配（游戏必备，防止高清屏模糊）
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    Widget w;
    w.show();
    return app.exec();
}
