#pragma once

#include "boxes/abstract_box.h"
#include "mtproto/sender.h"
#include "base/binary_guard.h"

class QRCodeBox : public BoxContent, public RPCSender {
public:
	QRCodeBox(QWidget*, not_null<PeerData*> peer);
protected:
	void prepare() override;

	void paintEvent(QPaintEvent *e) override;
	void keyPressEvent(QKeyEvent* e) override;

private:
	void init();
	QImage qrCodeImage();
	void copy();
	void saveAs();
	void refresh();

	void onDone(const MTPQRCode &data);
	bool onFail(const RPCError &error);

	not_null<PeerData*> _peer;
	Ui::RpWidget* _code;
	mtpRequestId _requestId = 0;
	
	rpl::variable<QByteArray> _qrCodes;
	rpl::variable<QString> _status;
};