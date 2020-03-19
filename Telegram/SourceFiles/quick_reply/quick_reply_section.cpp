#include "quick_reply/quick_reply_section.h"

#include "quick_reply/quick_reply_selector.h"
#include "window/window_controller.h"

namespace QuickReply {

QuickReplyMemento::QuickReplyMemento(object_ptr<QuickReplySelector> selector)
: _selector(std::move(selector)) {
}

object_ptr<Window::SectionWidget> QuickReplyMemento::createWidget(
        QWidget *parent,
        not_null<Window::Controller *> controller,
        Window::Column column,
        const QRect &geometry) {
    auto result = object_ptr<QuickReplySection>(
    parent,
    controller,
    std::move(_selector));
    result->setGeometry(geometry);
    return std::move(result);
}
    
QuickReplyMemento::~QuickReplyMemento() {}

QuickReplySection::QuickReplySection(QWidget *parent, not_null<Window::Controller *> controller)
: QuickReplySection(
    parent,
    controller,
    object_ptr<QuickReplySelector>(this, controller)){
}
    
QuickReplySection::QuickReplySection(
    QWidget *parent,
    not_null<Window::Controller *> controller,
    object_ptr<QuickReplySelector> selector)
    : Window::SectionWidget(parent, controller)
    , _selector(std::move(selector)){
    _selector->setParent(this);
        _selector->setGeometry(rect());
}
    
    QuickReplySection::~QuickReplySection() {}
    
    bool QuickReplySection::showInternal(not_null<Window::SectionMemento*> memento, const Window::SectionShow &params) {
        return true;
    }
    
    
    

} // namespace QuickReply



