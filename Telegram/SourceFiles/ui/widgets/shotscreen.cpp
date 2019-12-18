#include "stdafx.h"
#include "shotscreen.h"

namespace Ui {

ShotScreen::ShotScreen(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::ToolTip);
    showFullScreen();
    setMouseTracking(true);
    globalPath.lineTo(width(), 0);
    globalPath.lineTo(width(), height());
    globalPath.lineTo(0, height());
    globalPath.lineTo(0, 0);
}

void ShotScreen::init()
{
    pixmap = QGuiApplication::primaryScreen()->grabWindow(0);
}

void ShotScreen::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        emit finished();
    }
}

void ShotScreen::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        pressedPoint = event->pos();
        leftButtonPressed = true;
    }
}

void ShotScreen::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        leftButtonPressed = false;
    }
}

void ShotScreen::mouseMoveEvent(QMouseEvent* event)
{
    if (leftButtonPressed) {
        movePoint = event->pos();
        update();
    }
}

void ShotScreen::paintEvent(QPaintEvent*)
{
    QPainter paint(this);
    paint.drawPixmap(rect(), pixmap);

    paint.setPen(Qt::red);
    paint.setBrush(QColor(0, 0, 0, 100));

    paint.drawPath(MaskPath());
}

QPainterPath ShotScreen::MaskPath()
{
    QPainterPath path;
    if (leftButtonPressed) {
        path.moveTo(pressedPoint.x(), pressedPoint.y());
        path.lineTo(movePoint.x(), pressedPoint.y());
        path.lineTo(movePoint.x(), movePoint.y());
        path.lineTo(pressedPoint.x(), movePoint.y());
        path.lineTo(pressedPoint.x(), pressedPoint.y());
    }
    return globalPath.subtracted(path);
}

}