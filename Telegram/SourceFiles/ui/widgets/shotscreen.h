#pragma once

#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
namespace Ui {
    class ControlWidget;
    class Graphics;
    class RectAngle;
    class Arrow;
    class Mosaic;
    class Text;
}

namespace Ui {

void drawArrow(QPoint &startpoint, QPoint &endpoint, QPainter& p);
void drawMosaic(QList<QPoint> &list, QPainter& p);
void drawText(QRect& rect, QString& text, QPainter& p);
QFont textFont();

class ShotScreen : public QWidget
{
    Q_OBJECT
public:
    ShotScreen(QWidget* parent = nullptr);
    ~ShotScreen();

signals:
    void finished(bool);
public slots:
    void init();
private slots:
    void ontextchanged();
    void onScreenResized(int screen);
protected:
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent*) override;

private:
    void moveToScreen(bool force = false);
    QPainterPath maskPath();
    QRect activeRect();
    QRect controlWidgetRect();
    void appendText();
private:
    object_ptr<Ui::ControlWidget> _controlWidget;
    object_ptr<QTextEdit> _textEdit;
    QPixmap _pixmap;
    QPainterPath _globalPath;
    QList<Graphics*> _list;
    QRect _highLightRect = QRect();
    QRect _rectAngleRect = QRect();
    QPoint pressedPoint = QPoint(0, 0);          //鼠标左键按下后的坐标
    bool leftButtonPressed = false;
    QPoint movePoint = QPoint(0, 0);             //终点坐标
    QPoint arrowEndPoint = QPoint(0, 0);
    QVector<QVector<bool>> _pointArray;
    QList<QPoint> _mosaicList;
};

class ControlWidget : public QWidget
{
	Q_OBJECT
public:
    enum Type {
        RectAngle,
        Arrow,
        Mosaic,
        Text,
        None
    };

public:
	ControlWidget(QWidget* parent = nullptr);

    Type getType() const {
        return _type;
    }

    void setTypeChangedHandle(Fn<void()> callback);

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

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
private:
    Type _type;
    Fn<void()> _callback;
    object_ptr<Ui::IconButton> _drawRectangleAttach;
    object_ptr<Ui::IconButton> _drawArrowAttach;
    object_ptr<Ui::IconButton> _mosaicAttach;
    object_ptr<Ui::IconButton> _drawTextAttach;
    QVector<Ui::IconButton*> _optionsGroup;
    object_ptr<Ui::IconButton> _revokeAttach;
    object_ptr<Ui::IconButton> _savePicAttach;
    object_ptr<Ui::IconButton> _cancelShotAttach;
    object_ptr<Ui::IconButton> _finishShotAttach;
};

class Graphics
{
public:
    virtual ~Graphics(){}
    virtual void paint(QPainter& p) = 0;
};

class RectAngle : public Graphics
{
public:
    RectAngle(QRect rect)
        : Graphics()
        , _rect(rect) {}
protected:
    void paint(QPainter& p) override {
        p.drawRect(_rect);
    }
private:
    QRect _rect;
};

class Arrow : public Graphics
{
public:
    Arrow(QPoint startpoint, QPoint endpoint)
        : Graphics()
        , _startpoint(startpoint)
        , _endpoint(endpoint) {}
protected:
    void paint(QPainter& p) override {
        drawArrow(_startpoint, _endpoint, p);
    }
private:
    QPoint _startpoint;
    QPoint _endpoint;
};

class Mosaic : public Graphics
{
public:
    Mosaic(QList<QPoint> pointList)
        : Graphics()
        , _pointList(pointList) {}
    static const int length = 24;
protected:
    void paint(QPainter& p) override {
        drawMosaic(_pointList, p);
    }
private:
    QList<QPoint> _pointList;
};

class Text : public Graphics
{
public:
    Text(QRect rect, QString text)
        : Graphics()
        , _rect(rect)
        , _text(text) {}

protected:
    void paint(QPainter& p) override {
        drawText(_rect, _text, p);        
    }
private:
    QRect _rect;
    QString _text;
};

}
