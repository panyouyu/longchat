#pragma once

#include "boxes/abstract_box.h"
#include "mtproto/sender.h"
#include "styles/style_widgets.h"

namespace Ui {
class InputField;
}

class HandleFriendRequestBox : public BoxContent, public RPCSender{
public:
	HandleFriendRequestBox(QWidget*, not_null<UserData*> user);

protected:
	void prepare() override;

	void paintEvent(QPaintEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;

	void setInnerFocus() override;

private:
	void accept();
	void refuse();

	void onAcceptDone(const MTPcontacts_HandleFriendRequestResponse&response);
	bool onAcceptFail(const RPCError &error);

	void onRefuseDone(const MTPcontacts_HandleFriendRequestResponse& response);
	bool onRefuseFail(const RPCError &error);

	not_null<UserData*> _user;
	object_ptr<Ui::InputField> _first;
	object_ptr<Ui::InputField> _verify_info;
	mtpRequestId _acceptRequest = 0;
	mtpRequestId _refuseRequest = 0;
	QPointer<BoxContent> _confirmRefuse = nullptr;
};