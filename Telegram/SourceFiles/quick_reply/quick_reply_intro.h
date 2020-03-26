#pragma once

#include "window/layer_widget.h"
#include "base/basic_types.h"

namespace Ui {
class VerticalLayout;
class FadeShadow;
class FlatLabel;
template <typename Widget>
class FadeWrap;
} // namespace Ui

namespace QuickReply {

class IntroWidget;

class LayerWidget : public Window::LayerWidget
{
public:
	LayerWidget(QWidget*);

	void parentResized() override;

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	object_ptr<IntroWidget> _content;
};
}