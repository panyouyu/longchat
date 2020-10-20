#pragma once

#include "boxes/abstract_box.h"
#include "mtproto/rpc_sender.h"

namespace Ui {
	class InputField;
}

class BlockUserBox : public BoxContent, public RPCSender {
public:
	BlockUserBox(QWidget*, not_null<UserData*> user);

protected:
	void prepare() override;

	void paintEvent(QPaintEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;

	void setInnerFocus() override;

private:
	void submit();
	void onBlockDone(const MTPBool &result);
	bool onBlockFail(const RPCError &error);

	not_null<UserData*> _user;
	object_ptr<Ui::InputField> _userName;
	object_ptr<Ui::InputField> _reason;

	mtpRequestId _blockId = 0;
};