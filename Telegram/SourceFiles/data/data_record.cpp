#include "data/data_record.h"

#include "auth_session.h"
#include "app.h"
#include "mainwindow.h"
#include "mainwidget.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "data/data_chat.h"
#include "ui/text_options.h"
#include "lang/lang_keys.h"
#include "history/history.h"
#include "history/history_item.h"
#include "window/window_controller.h"

RecordData::RecordData(not_null<HistoryItem*> item, const MTPDmessageMediaRecord& data) {
	Ensures(data.vmessages.type() == mtpc_messages_messages ||
		data.vmessages.type() == mtpc_messages_channelMessages);
	_forwarder = data.vforwarder_id.v;
	_originer = data.voriginer_id.v;	

	_msgId.reserve(data.vmsg_id.v.size());
	for (auto& id : data.vmsg_id.v) {
		_msgId.push_back(id.v);
	}

	auto& session = item->history()->owner();
	auto progcessMessage = [&](const MTPMessage& message)->QString {
		return message.match([&](const MTPDmessage& msg)->QString {
			QString from = session.user(msg.vfrom_id.v)->name + qsl(": ");
			if (msg.vmessage.v.size()) {
				return from + QString(msg.vmessage.v);
			}
			else if (msg.has_media()) {
				return msg.vmedia.match([&](const MTPDmessageMediaEmpty&)->QString {
					return from + lang(lng_message_empty);
					}, [&](const MTPDmessageMediaPhoto&)->QString {
						return from + lang(lng_in_dlg_photo);
					}, [&](const MTPDmessageMediaGeo&)->QString {
						return from + lang(lng_maps_point);
					}, [&](const MTPDmessageMediaContact&)->QString {
						return from + lang(lng_in_dlg_contact);
					}, [&](const MTPDmessageMediaUnsupported&)->QString {
						return from + lang(lng_message_unsupported);
					}, [&](const MTPDmessageMediaDocument& media)->QString {
						return media.vdocument.match([&](const MTPDdocument& document)->QString {
							return from + document.vmime_type.v;
							}, [&](const MTPDdocumentEmpty&)->QString {
								return from;
							});
					}, [&](const MTPDmessageMediaWebPage&)->QString {
						return from + lang(lng_web_page);
					}, [&](const MTPDmessageMediaVenue&)->QString {
						return from + lang(lng_maps_point);
					}, [&](const MTPDmessageMediaGame&)->QString {
						return from + lang(lng_game_tag);
					}, [&](const MTPDmessageMediaPhoto&)->QString {
						return from + lang(lng_in_dlg_photo);
					}, [&](const MTPDmessageMediaInvoice& media)->QString {
						return from + media.vtitle.v;
					}, [&](const MTPDmessageMediaGeoLive&)->QString {
						return from + lang(lng_maps_point);
					}, [&](const MTPDmessageMediaPoll&)->QString {
						return from + lang(lng_polls_anonymous);
					}, [&](const MTPDmessageMediaTlv& media)->QString {
						auto& tlvs = media.vtlv.c_tlvs().vtlvs.v;
						for (auto& tlv : tlvs) {
							switch (tlv.c_tlv().vid.v) {
							case mtpc_messageMediaRecord:
								return from + lang(lng_message_record); break;
							}
						}
						return from;
					});
			}
			return from;
			}, [](const auto&)->QString {
				return lang(lng_message_unsupported);
			});
	};

	data.vmessages.match([&](const MTPDmessages_messages& messages) {
		session.processChats(messages.vchats);
		session.processUsers(messages.vusers);

		_msg.reserve(messages.vmessages.v.size());
		for (auto &message : messages.vmessages.v) {
			_content.push_back(progcessMessage(message));
		}
		for (auto i = messages.vmessages.v.end(), e = messages.vmessages.v.begin(); i != e;) {
			auto &message = *(--i);
			if (message.type() == mtpc_message) {
				auto flags = message.c_message().vflags.v;
				auto msg_id = message.c_message().vid.v;
				auto from = message.c_message().vfrom_id.v;
				auto date = message.c_message().vdate.v;
				auto text = message.c_message().vmessage.v;
				auto media = message.c_message().vmedia;
				_msg.push_back(
					MTP_message(
						MTP_flags(flags),
						MTP_int(msg_id),
						MTP_int(from),
						MTP_peerChannel(MTP_int(peerToChannel(PeerData::kRecordDialogId))),
						MTPMessageFwdHeader(),
						MTPint(),
						MTPint(),
						MTP_int(date),
						MTP_string(QString(text)),
						media,
						MTPReplyMarkup(),
						message.c_message().ventities,
						MTPint(),
						MTPint(),
						MTPstring(),
						MTPlong())
				);
			}
		}
	}, [](const auto&) {});
	
	auto title = !data.is_user() ?
		lang(lng_record_chat_title) :
		lng_record_user_title(lt_originer,
			_originer == Auth().userId() ? 
				lang(lng_profile_me) :
				session.peer(_originer)->name, 
			lt_forwarder, 
			_forwarder == Auth().userId() ? 
				lang(lng_profile_me) :
				session.peer(_forwarder)->name);
	_title.setText(st::msgNameStyle, title, Ui::NameTextOptions());
}

TextForMimeData RecordData::clipboardText() const {
	auto text = _title.toString() + '\n';
	for (size_t i = 0, e = _content.size(); i < e; ++i) {
		text += _content.at(i) + '\n';
	}
	return TextForMimeData::Simple(text);
}

void RecordClickHandler::onClickImpl() const {
	if (_record->msgs().size() == 0) return;

	auto& session = Auth().data();
	session.processChat(MTP_channelForbidden(
		MTP_flags(0),
		MTP_int(peerToChannel(PeerData::kRecordDialogId)),
		MTP_long(0),
		MTP_string(_record->title().toString()),
		MTP_int(unixtime())));

	auto history = session.history(PeerData::kRecordDialogId);
	if (!history->isEmpty()) {
		history->clear(History::ClearType::DeleteChat);
	}
	if (!history->isEmpty()) {
		return;
	}
	history->addNewerSlice(QVector<MTPMessage>());
	history->addOlderSlice(_record->msgs());

	App::wnd()->controller()->showPeerHistory(
		PeerData::kRecordDialogId,
		Window::SectionShow::Way::Forward);
	
	App::main()->removeDialog(Dialogs::Key(history));
}
