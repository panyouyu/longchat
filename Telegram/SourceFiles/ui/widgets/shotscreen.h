#pragma once

#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
namespace Ui {
    class ControlWidget;
}

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
    object_ptr<Ui::ControlWidget> _controlWidget;
    QPixmap pixmap;
    QPainterPath globalPath;
    QPoint pressedPoint = QPoint(0, 0);          //鼠标左键按下后的坐标
    bool leftButtonPressed = false;
    QPoint movePoint = QPoint(0, 0);             //终点坐标
};

class ControlWidget : public QWidget
{
	Q_OBJECT
public:
	ControlWidget(QWidget* parent = nullptr);

    void setDrawRectangleAttachHandle(Fn<void()> callback) {
        _drawRectangleAttach->setClickedCallback(callback);
    }
    void setDrawArrowAttachHandle(Fn<void()> callback) {
        _drawArrowAttach->setClickedCallback(callback);
    }
    void setMosaicAttachHandle(Fn<void()> callback) {
        _mosaicAttach->setClickedCallback(callback);
    }
    void setDrawTextAttachHandle(Fn<void()> callback) {
        _drawTextAttach->setClickedCallback(callback);
    }
    void setRevokeAttachHandle(Fn<void()> callback) {
        _revokeAttach->setClickedCallback(callback);
    }
    void setSavePicAttachHandle(Fn<void()> callback) {
        _savePicAttach->setClickedCallback(callback);
    }
    void setCancelShotAttachHandle(Fn<void()> callback) {
        _cancelShotAttach->setClickedCallback(callback);
    }
    void setFinishShotAttachHandle(Fn<void()> callback) {
        _finishShotAttach->setClickedCallback(callback);
    }

private:
    object_ptr<Ui::IconButton> _drawRectangleAttach;
    object_ptr<Ui::IconButton> _drawArrowAttach;
    object_ptr<Ui::IconButton> _mosaicAttach;
    object_ptr<Ui::IconButton> _drawTextAttach;
    object_ptr<Ui::IconButton> _revokeAttach;
    object_ptr<Ui::IconButton> _savePicAttach;
    object_ptr<Ui::IconButton> _cancelShotAttach;
    object_ptr<Ui::IconButton> _finishShotAttach;
};

}
