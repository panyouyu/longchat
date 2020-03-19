#pragma once

#include "ui/rp_widget.h"

namespace Window {
    class Controller;
}

namespace QuickReply {
    
class QuickReplySelector : public Ui::RpWidget {
public:
    QuickReplySelector(QWidget *parent, not_null<Window::Controller*> controller);
    ~QuickReplySelector();
};
    
} // namespace QuickReply
