/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once
#include "boxes/abstract_box.h"
#include "scheme.h"
#include "treeitem.h"
#include "treemodel.h"
#include "contact/contacttreeview.h"
#include "contactdelegate.h"
#include "base/observer.h"
#include "mtproto/sender.h"
#include "window/window_controller.h"


namespace Contact {

	class FilterWidget;

	class ContactBox : public BoxContent, public RPCSender {/*, private base::Subscriber*/
		Q_OBJECT
	public:
		ContactBox(QWidget* parent, Window::Controller* controller);
		~ContactBox();
		
	protected:
		void prepare() override;

	private:
		void init();
		void updateGroupInfoData();
		void clearData();

		void showCodeError(Fn<QString()> textFactory);


		void userGroupDelDone(const MTPUserGroupReturn& result);
		bool userGroupDelFail(const RPCError& error);
		void resultHandler(int result);

	signals:
		void startChat(int64 peerId);
	private slots:
		void slotChat(int64 peerId);
		void slotSaveGroup();
		void textFilterChanged();
		void slotAddGroup();
		void slotModGroup(ContactInfo* pCI);
		void slotDelGroup(ContactInfo* pCI);
		void slotShowUserInfo(ContactInfo* pCI);
		void onCloseWait();
		
	private:
		FilterWidget* _filterWidget;
		ContactTreeView* _contactTree{ nullptr };
		QVBoxLayout* _vLayout;
		QHBoxLayout* _hLayoutStyle;


		QVector<ContactInfo*> _vecContactPData;
		QVector<ContactInfo*> _vecContactPData4Search; //qt5.10以后才支持下级查询，所以这里把下级提升为1级查询 且把parentid设置为0
		QMap<uint64, QSet<uint64>> _mapUser2Group;

		Window::Controller* _controller = { nullptr };

		mtpRequestId _allUserTagDelRequest = 0;

		object_ptr<QTimer> _closeWait;

	};

}