#pragma once

#include "ui/rp_widget.h"

namespace Window {
    class Controller;
}

namespace Ui {
    class ScrollArea;
}

namespace QuickReply {
    
class TopBar;
class SimpleTree;

class Selector : public Ui::RpWidget {
public:
    Selector(QWidget *parent, not_null<Window::Controller*> controller);
    ~Selector();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
private:
    QRect scrollRect() const;
    void updateControlsGeometry();

    Window::Controller* _controller;
    object_ptr<TopBar> _topBar;
    not_null<Ui::ScrollArea*> _scroll;
    QPointer<SimpleTree> _inner;
};
    
} // namespace QuickReply
