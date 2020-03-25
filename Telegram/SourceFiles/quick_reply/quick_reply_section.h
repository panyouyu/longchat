#pragma once

#include "window/section_memento.h"
#include "window/section_widget.h"

namespace Window {
    struct SectionShow;
}

namespace QuickReply {
    
class Selector;
    
class Memento : public Window::SectionMemento {
public:
    Memento(object_ptr<Selector> selector);
    
    object_ptr<Window::SectionWidget> createWidget(
        QWidget *parent,
        not_null<Window::Controller*> controller,
        Window::Column column,
        const QRect &geometry) override;
    
    ~Memento();
    
private:
    object_ptr<Selector> _selector;
};
    
class Section : public Window::SectionWidget {
public:
    Section(QWidget *parent, not_null<Window::Controller*> controller);
    Section(QWidget *parent, not_null<Window::Controller*> controller, object_ptr<Selector> selector);
    
    ~Section();
    
    bool showInternal(not_null<Window::SectionMemento*> memento, const Window::SectionShow &params) override;
    
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    object_ptr<Selector> _selector;
};
    
} // namespace QuickReply
