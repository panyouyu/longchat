#pragma once

#include "window/layer_widget.h"

namespace QuickReply {

class IntroWidget;

class LayerWidget : public Window::LayerWidget
{
public:
	LayerWidget(QWidget*);
	void parentResized() override;

protected:
	void paintEvent(QPaintEvent* e) override;
	void closeHook() override;

private:
	object_ptr<IntroWidget> _content;
};
} // namespace QuickReply
