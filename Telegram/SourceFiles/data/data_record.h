#pragma once

#include "data/data_types.h"

class HistoryRecord;

class RecordData {
public:
	explicit RecordData(not_null<HistoryItem*> item,
		const MTPDmessageMediaRecord& data);
	~RecordData() = default;
	
	const Text &title() const {
		return _title;
	}
	const std::vector<QString> &content() const {
		return _content;
	}
	const QVector<MTPMessage>& msgs() const {
		return _msg;
	}
	TextForMimeData clipboardText() const;

private:
	friend inline bool operator == (const RecordData &a, const RecordData &b) {
		return (a._originer == b._originer) &&
			(a._forwarder == b._forwarder) &&
			(a._msgId == b._msgId);
	}
	friend inline bool operator != (const RecordData& a, const RecordData& b) {
		return !(a == b);
	}
	//friend inline bool operator < (const RecordData& a, const RecordData& b) {
	//	return a._originer < b._originer;
	//}

	PeerId _originer;
	PeerId _forwarder = 0;
	std::vector<int> _msgId;
	std::vector<QString> _content;
	Text _title;	
	QVector<MTPMessage> _msg;
};

class RecordClickHandler : public LeftButtonClickHandler {
public:
	RecordClickHandler(
		not_null<RecordData*> record,
		FullMsgId context = FullMsgId()) 
	: _record(record)
	, _context(context) {}
protected:
	void onClickImpl() const;
private:
	not_null<RecordData*> _record;
	FullMsgId _context;
};