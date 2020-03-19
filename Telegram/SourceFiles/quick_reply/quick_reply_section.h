#pragma once

#include "window/section_memento.h"
#include "window/section_widget.h"

namespace Window {
    struct SectionShow;
}

namespace QuickReply {
    
class QuickReplySelector;
    
class QuickReplyMemento : public Window::SectionMemento {
public:
    QuickReplyMemento(object_ptr<QuickReplySelector> selector);
    
    object_ptr<Window::SectionWidget> createWidget(
        QWidget *parent,
        not_null<Window::Controller*> controller,
        Window::Column column,
        const QRect &geometry) override;
    
    ~QuickReplyMemento();
    
private:
    object_ptr<QuickReplySelector> _selector;
};
    
class QuickReplySection : public Window::SectionWidget {
public:
    QuickReplySection(QWidget *parent, not_null<Window::Controller*> controller);
    QuickReplySection(QWidget *parent, not_null<Window::Controller*> controller, object_ptr<QuickReplySelector> selector);
    
    ~QuickReplySection();
    
    bool showInternal(not_null<Window::SectionMemento*> memento, const Window::SectionShow &params) override;
    
private:
    object_ptr<QuickReplySelector> _selector;
};
    
} // namespace QuickReply
