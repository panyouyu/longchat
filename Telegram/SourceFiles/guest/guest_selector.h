#pragma once

#include "ui/rp_widget.h"
#include "dialogs/dialogs_key.h"

namespace Window {
	class Controller;
}

namespace Ui {
	class ScrollArea;
	class InputField;
	class FlatButton;
}

namespace QuickReply {
	class TopBar;
}

class ConfirmBox;

namespace Guest {

class InfoWidget;
class PropertyWidget;

class Selector : public Ui::RpWidget {
public:
	Selector(QWidget* parent, not_null<Window::Controller*> controller, Dialogs::Key key);
	~Selector();
protected:
	void resizeEvent(QResizeEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
private:
	void load(Dialogs::Key key);
	void loadInfo(not_null<UserData*> user);
	void loadLabels(not_null<UserData*> user);
	void updateBlock(not_null<UserData*> user);
	QRect scrollRect() const;
	void updateControlsGeometry();
	Window::Controller* _controller;
	object_ptr<QuickReply::TopBar> _topBar;
	object_ptr<Ui::ScrollArea> _scroll;
	QPointer<InfoWidget> _info;
	object_ptr<PropertyWidget> _property;
	object_ptr<Ui::FlatButton> _block;
	QPointer<ConfirmBox> _confirmBox;
};

} // namespace Guest