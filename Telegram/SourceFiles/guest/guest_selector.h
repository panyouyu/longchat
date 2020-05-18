#pragma once

#include "ui/rp_widget.h"
#include "dialogs/dialogs_key.h"

namespace Window {
	class Controller;
}

namespace Ui {
	class ScrollArea;
	class InputField;
	class LeftFlatButton;
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
	QRect intervalRect() const;
	void updateControlsGeometry();
	Window::Controller* _controller;
	object_ptr<Ui::ScrollArea> _scroll;
	QPointer<InfoWidget> _info;
	object_ptr<PropertyWidget> _property;
	object_ptr<Ui::LeftFlatButton> _url;
	object_ptr<Ui::LeftFlatButton> _block;
	QPointer<ConfirmBox> _confirmBox;
};

} // namespace Guest