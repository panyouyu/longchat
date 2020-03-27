#pragma once

#include "window/layer_widget.h"

#include "auth_session.h"
#include "window/section_widget.h"
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

	void setCloseHook(Fn<void()> callback) {
		_callback = callback;
	}
protected:
	void paintEvent(QPaintEvent* e) override;
	virtual void closeHook() {
		if (const auto callback = base::take(_callback)) {
			// only for thirdSection of QuickReply update
			if (Auth().settings().thirdSectionQuickReplyEnabled())
				callback();
		}
	}

private:
	object_ptr<IntroWidget> _content;
	Fn<void()> _callback;
};
}