#pragma once

class QWidget;
namespace Ui {

class ShotScreen : public QWidget
{
    Q_OBJECT
public:
    ShotScreen(QWidget* parent = nullptr);

signals:
    void finished();
public slots:
    void init();
protected:
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent*) override;

private:
    QPainterPath MaskPath();

private:
    QPixmap pixmap;
    QPainterPath globalPath;
    QPoint pressedPoint = QPoint(0, 0);          //鼠标左键按下后的坐标
    bool leftButtonPressed = false;
    QPoint movePoint = QPoint(0, 0);             //终点坐标
};

}
