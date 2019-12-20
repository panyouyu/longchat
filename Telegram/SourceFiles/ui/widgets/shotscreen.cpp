#include "stdafx.h"
#include "shotscreen.h"

#include "styles/style_history.h"

#include <algorithm>

namespace Ui {

ShotScreen::ShotScreen(QWidget* parent)
    : QWidget(parent)
    , _controlWidget(this)
{
    setWindowFlags(Qt::ToolTip);
    showFullScreen();


    _controlWidget->show();

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
    paint.fillRect(_controlWidget->rect(), Qt::white);

    paint.setPen(Qt::red);
    paint.setBrush(QColor(0, 0, 0, 50));

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
    path.addRect(_controlWidget->rect());
    return globalPath.subtracted(path);
}

ControlWidget::ControlWidget(QWidget *parent)
    : QWidget(parent)
    , _drawRectangleAttach(this, st::historyDrawRectangleAttach)
    , _drawArrowAttach(this, st::historyDrawArrowAttach)
    , _mosaicAttach(this, st::historyMosaicAttach)
    , _drawTextAttach(this, st::historyDrawTextAttach)
    , _revokeAttach(this, st::historyRevokeAttach)
    , _savePicAttach(this, st::historySavePicAttach)
    , _cancelShotAttach(this, st::historyCancelShotAttach)
    , _finishShotAttach(this, st::historyFinishShotAttach)
{
    auto width = st::historyDrawRectangleAttach.width + st::historyDrawArrowAttach.width +
        st::historyMosaicAttach.width + st::historyDrawTextAttach.width +
        st::historyRevokeAttach.width + st::historySavePicAttach.width +
        st::historyCancelShotAttach.width + st::historyFinishShotAttach.width;
    auto height = std::max({ st::historyDrawRectangleAttach.height, st::historyDrawArrowAttach.height,
        st::historyMosaicAttach.height, st::historyDrawTextAttach.height,
        st::historyRevokeAttach.height, st::historySavePicAttach.height,
        st::historyCancelShotAttach.height, st::historyFinishShotAttach.height });
    setFixedSize(width, height);

    auto buttonsBottom = 0;
    auto left = 0;

    _drawRectangleAttach->moveToLeft(left, buttonsBottom); left += _drawRectangleAttach->width();
    _drawArrowAttach->moveToLeft(left, buttonsBottom); left += _drawArrowAttach->width();
    _mosaicAttach->moveToLeft(left, buttonsBottom); left += _mosaicAttach->width();
    _drawTextAttach->moveToLeft(left, buttonsBottom); left += _drawTextAttach->width();
    _revokeAttach->moveToLeft(left, buttonsBottom); left += _revokeAttach->width();
    _savePicAttach->moveToLeft(left, buttonsBottom); left += _savePicAttach->width();
    _cancelShotAttach->moveToLeft(left, buttonsBottom); left += _cancelShotAttach->width();
    _finishShotAttach->moveToLeft(left, buttonsBottom); left += _finishShotAttach->width();
}

}