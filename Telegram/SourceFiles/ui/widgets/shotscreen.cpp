#include "stdafx.h"
#include "shotscreen.h"

#include "styles/style_history.h"
#include "lang/lang_keys.h"
#include <QClipboard>
#include <algorithm>

namespace Ui {

ShotScreen::ShotScreen(QWidget* parent)
    : QWidget(parent)
    , _controlWidget(this)
    , _textEdit(this)
{
#ifdef Q_OS_MAC
    showMaximized();
#else
    setWindowFlags(Qt::ToolTip);
    resize(QApplication::desktop()->screenGeometry().size());
#endif
    _controlWidget->hide();
    _textEdit->hide();
    QPalette p1 = _textEdit->palette();
    p1.setBrush(QPalette::Base, QBrush(QColor(255, 0, 0, 0)));
    _textEdit->setPalette(p1);
    _textEdit->setFont(textFont());
    _textEdit->setTextColor(Qt::red);
    connect(_textEdit, SIGNAL(textChanged()), this, SLOT(ontextchanged()));

    _controlWidget->setRevokeAttachHandle([&] {
        if (!_list.isEmpty()) {
            _list.takeLast();            
            update();
        }});

    _controlWidget->setSavePicAttachHandle([&] {
        QString picName;
        QTime time = QTime::currentTime();
        qsrand(time.msec() + time.second() * 1000);
        QString randStr;
        randStr.setNum(qrand());
        picName.append(randStr);

        QString filename = QFileDialog::getSaveFileName(this, lang(lng_save_screenshot_image), picName, "JPEG Files(*.jpg)");

        if (filename.length() > 0) {
            QImage pimage = QPixmap::grabWidget(this, _highLightRect).toImage();
            pimage.save(filename, "jpg");
        }
        });

    _controlWidget->setCancelShotAttachHandle([&] { emit finished(false); });

    _controlWidget->setFinishShotAttachHandle([&] { 
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setImage(QPixmap::grabWidget(this, _highLightRect).toImage());
        emit finished(true); 
        });

    setMouseTracking(true);
    _globalPath.lineTo(width(), 0);
    _globalPath.lineTo(width(), height());
    _globalPath.lineTo(0, height());
    _globalPath.lineTo(0, 0);

    _list.clear();
    _mosaicList.clear();
}

ShotScreen::~ShotScreen()
{
    qDeleteAll(_list);
    _list.clear();
    _mosaicList.clear();
}

void ShotScreen::init()
{
    _pixmap = QGuiApplication::primaryScreen()->grabWindow(0);
    int width_p = _pixmap.width() / Mosaic::length + 1;
    int heigth_p = _pixmap.height() / Mosaic::length + 1;
    _pointArray.resize(width_p);
    for (auto i = 0; i < width_p; i++) {
        _pointArray[i].resize(heigth_p);
    }
    for (auto i = 0; i < width_p; i++) {
        for (auto j = 0; j < heigth_p; j++) {
            _pointArray[i][j] = false;
        }
    }
}

void ShotScreen::ontextchanged()
{
    int max_width = _highLightRect.x() + _highLightRect.width() - pressedPoint.x();
    int max_height = _highLightRect.y() + _highLightRect.height() - pressedPoint.y();
    QFontMetrics fontMetrics(_textEdit->font());
    QStringList strList = _textEdit->toPlainText().split('\n');
    int width = 0;
    for (int i = 0; i < strList.size(); i++) {
        width = std::max(width, fontMetrics.width(strList.at(i)));
    }
    width += 30;
    width = width < max_width ? width : max_width;
    int height = _textEdit->document()->size().rheight() + 2 < max_height ? _textEdit->document()->size().rheight() + 2 : max_height;
    _textEdit->resize(width, height);
}

void ShotScreen::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        emit finished(false);
    }
}

void ShotScreen::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (!_textEdit->toPlainText().isEmpty())
        {
            QRect rect = QRect(QPoint(_textEdit->x(), _textEdit->y()), _textEdit->size());
            _list.append(new Text(rect, _textEdit->toPlainText()));
            update();
        }
        _textEdit->hide();
        _textEdit->clear();
    }
    else if (event->button() == Qt::LeftButton) {
        pressedPoint = event->pos();
        switch (_controlWidget->getType()) {
        case ControlWidget::None:
            qDeleteAll(_list);
            _list.clear();
            _controlWidget->hide();
            update();
            break;
        case ControlWidget::RectAngle:
        case ControlWidget::Arrow:
            if (!_highLightRect.contains(pressedPoint)) return;
            break;
        case ControlWidget::Mosaic:
        {
            if (!_highLightRect.contains(pressedPoint)) return;
            auto x = pressedPoint.x() / Mosaic::length;
            auto y = pressedPoint.y() / Mosaic::length;
            if (_pointArray[x][y] == false && 
                _highLightRect.contains(QRect(QPoint(x * Mosaic::length, y * Mosaic::length), QSize(Mosaic::length, Mosaic::length)))) {
                _mosaicList.append(QPoint(x * Mosaic::length, y * Mosaic::length));
                _pointArray[x][y] = true;
            }
        }
        break;
        case ControlWidget::Text:
            if (!_highLightRect.contains(pressedPoint)) return;
            if (!_textEdit->toPlainText().isEmpty())
            {
                QRect rect = QRect(QPoint(_textEdit->x(), _textEdit->y()), _textEdit->size());
                _list.append(new Text(rect, _textEdit->toPlainText()));
                update();
            }
            _textEdit->move(pressedPoint);
            _textEdit->show();
            _textEdit->clear();
            break;
        }
        leftButtonPressed = true;
    }    
}

void ShotScreen::mouseReleaseEvent(QMouseEvent* event)
{
    if (!leftButtonPressed) return;  

    switch (_controlWidget->getType()) {
    case ControlWidget::None:
        _controlWidget->setGeometry(controlWidgetRect());
        _controlWidget->show();
        break;
    case ControlWidget::RectAngle:
        _list.append(new RectAngle(_rectAngleRect));
        _rectAngleRect = QRect();
        break;
    case ControlWidget::Arrow:
        _list.append(new Arrow(pressedPoint, movePoint));
        break;
    case ControlWidget::Mosaic:
        _list.append(new Mosaic(_mosaicList));
        _mosaicList.clear();
        for (auto i = 0; i < _pointArray.size(); i++) {
            for (auto j = 0; j < _pointArray[i].size(); j++) {
                _pointArray[i][j] = false;
            }
        }
        break;
    case ControlWidget::Text:
        break;
    }
    leftButtonPressed = false;
}

void ShotScreen::mouseMoveEvent(QMouseEvent* event)
{
    if (!leftButtonPressed) return;
    movePoint = event->pos();
    switch (_controlWidget->getType()) {
    case ControlWidget::None:
        _highLightRect = activeRect();
        update();
        break;
    case ControlWidget::RectAngle:        
        if (!_highLightRect.contains(movePoint)) return;
        _rectAngleRect = activeRect();
        update();
        break;
    case ControlWidget::Arrow:
        if (!_highLightRect.contains(movePoint)) return;
        update();
        break;
    case ControlWidget::Mosaic:
    {
        if (!_highLightRect.contains(movePoint)) return;
        auto x = movePoint.x() / Mosaic::length;
        auto y = movePoint.y() / Mosaic::length;
        if (_pointArray[x][y] == false && 
            _highLightRect.contains(QRect(QPoint(x * Mosaic::length, y * Mosaic::length), QSize(Mosaic::length, Mosaic::length)))) {
            _mosaicList.append(QPoint(x * Mosaic::length, y * Mosaic::length));
            _pointArray[x][y] = true;
            update();
        }
    }
        break;
    case ControlWidget::Text:
        break;
    }    
}

void ShotScreen::paintEvent(QPaintEvent*)
{
    QPainter paint(this);
    paint.drawPixmap(rect(), _pixmap);
    paint.save();
    paint.setBrush(QColor(0, 0, 0, 150));
    paint.drawPath(maskPath());
    paint.restore();
    paint.setPen(QPen(Qt::red, 2));

    for (int i = 0; i < _list.size(); i++) {
        _list.at(i)->paint(paint);
    }

    if (leftButtonPressed) {
        switch (_controlWidget->getType()) {
        case ControlWidget::None:
            break;
        case ControlWidget::RectAngle:
            paint.drawRect(_rectAngleRect);
            break;
        case ControlWidget::Arrow:
            drawArrow(pressedPoint, movePoint, paint);
            break;
        case ControlWidget::Mosaic:
            drawMosaic(_mosaicList, paint);
            break;
        case ControlWidget::Text:
            break;
        }
    }
}

QPainterPath ShotScreen::maskPath()
{
    QPainterPath path;
    path.addRect(_highLightRect);
    
    return _globalPath.subtracted(path);
}

QRect ShotScreen::activeRect()
{
    if (!leftButtonPressed) return QRect();

    auto x = qMin(pressedPoint.x(), movePoint.x());
    auto y = qMin(pressedPoint.y(), movePoint.y());
    auto w = qAbs(pressedPoint.x() - movePoint.x());
    auto h = qAbs(pressedPoint.y() - movePoint.y());
    return QRect(x, y, w, h);
}

QRect ShotScreen::controlWidgetRect()
{    
    int right = qMax(pressedPoint.x(), movePoint.x());
    int x = right - _controlWidget->width();
    if (x < 0) {
        x = qMin(pressedPoint.x(), movePoint.x());
    }
    int y = qMax(pressedPoint.y(), movePoint.y());
    if (height() - y < _controlWidget->height()) {
        y = y - _controlWidget->height();
    }
    return QRect(x, y, _controlWidget->width(), _controlWidget->height());
}

ControlWidget::ControlWidget(QWidget *parent)
    : QWidget(parent)
    , _type(None)
    , _drawRectangleAttach(this, st::historyDrawRectangleAttach)
    , _drawArrowAttach(this, st::historyDrawArrowAttach)
    , _mosaicAttach(this, st::historyMosaicAttach)
    , _drawTextAttach(this, st::historyDrawTextAttach)
    , _revokeAttach(this, st::historyRevokeAttach)
    , _savePicAttach(this, st::historySavePicAttach)
    , _cancelShotAttach(this, st::historyCancelShotAttach)
    , _finishShotAttach(this, st::historyFinishShotAttach)
{
    _optionsGroup.append(_drawRectangleAttach.data());
    _optionsGroup.append(_drawArrowAttach.data());
    _optionsGroup.append(_mosaicAttach.data());
    _optionsGroup.append(_drawTextAttach.data());    

    auto width = st::historyDrawRectangleAttach.width + st::historyDrawArrowAttach.width +
        st::historyMosaicAttach.width + st::historyDrawTextAttach.width +
        st::historyRevokeAttach.width + st::historySavePicAttach.width +
        st::historyCancelShotAttach.width + st::historyFinishShotAttach.width;
    auto height = std::max({ st::historyDrawRectangleAttach.height, st::historyDrawArrowAttach.height,
        st::historyMosaicAttach.height, st::historyDrawTextAttach.height,
        st::historyRevokeAttach.height, st::historySavePicAttach.height,
        st::historyCancelShotAttach.height, st::historyFinishShotAttach.height });
    setFixedSize(width, height);    

    auto callback = [&](Type type = None) {
        if (_type == type) {
            _type = None;
        }
        else {
            _type = type;
        }
        update();
    };

    _drawRectangleAttach->setClickedCallback([=] { callback(RectAngle); });
    _drawArrowAttach->setClickedCallback([=] { callback(Arrow); });
    _mosaicAttach->setClickedCallback([=] { callback(Mosaic); });
    _drawTextAttach->setClickedCallback([=] { callback(Text); });
}

void ControlWidget::resizeEvent(QResizeEvent* event)
{
    (void)event;
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

void ControlWidget::paintEvent(QPaintEvent* event)
{
    (void)event;
    QPainter painter(this);
    painter.fillRect(rect(), qRgb(247, 245, 218));
    if (_type != None) {
        auto button = _optionsGroup.at(_type);
        painter.fillRect(QRect(button->x(), button->y(), button->width(), button->height()), qRgb(255, 147, 70));
    }
}

void drawArrow(QPoint &startpoint, QPoint &endpoint, QPainter& p)
{
    double par = 15.0;
    double slopy = atan2((endpoint.y() - startpoint.y()), (endpoint.x() - startpoint.x()));
    double cosy = cos(slopy);
    double siny = sin(slopy);
    QPoint point1 = QPoint(endpoint.x() + int(-par * cosy - (par / 2.0 * siny)), endpoint.y() + int(-par * siny + (par / 2.0 * cosy)));
    QPoint point2 = QPoint(endpoint.x() + int(-par * cosy + (par / 2.0 * siny)), endpoint.y() - int(par / 2.0 * cosy + par * siny));
    QPoint points[3] = { endpoint,point1,point2 };
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPolygon(points, 3);
    int offsetx = int(par * siny / 3);
    int offsety = int(par * cosy / 3);
    QPoint point3, point4;
    point3 = QPoint(endpoint.x() + int(-par * cosy - (par / 2.0 * siny)) + offsetx, endpoint.y() + int(-par * siny + (par / 2.0 * cosy)) - offsety);
    point4 = QPoint(endpoint.x() + int(-par * cosy + (par / 2.0 * siny) - offsetx), endpoint.y() - int(par / 2.0 * cosy + par * siny) + offsety);
    QPoint arrbodypoints[3] = { startpoint,point3,point4 };
    p.drawPolygon(arrbodypoints, 3);
}

void drawMosaic(QList<QPoint> &list, QPainter& p)
{
    QRect rect1, rect2, rect3, rect4;
    int width = Mosaic::length >> 1;
    int height = Mosaic::length >> 1;
    QSize size = QSize(width, height);
    for (int i = 0; i < list.size(); i++) {
        rect1 = QRect(list.at(i), size);
        rect2 = QRect(list.at(i) + QPoint(width, 0), size);
        rect3 = QRect(list.at(i) + QPoint(0, height), size);
        rect4 = QRect(list.at(i) + QPoint(width, height), size);

        p.fillRect(rect1, Qt::black);
        p.fillRect(rect2, Qt::white);
        p.fillRect(rect3, Qt::white);
        p.fillRect(rect4, Qt::black);
    }
}

void drawText(QRect& rect, QString& text, QPainter& p)
{
    p.save();
    p.setFont(textFont());
    p.drawText(rect.adjusted(5, 5, -5, -5), Qt::TextWrapAnywhere, text);
    p.restore();
}

QFont textFont()
{
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPixelSize(15);
    return font;
}

}