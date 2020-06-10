#pragma once

#include "history/media/history_media.h"

#include "data/data_record.h"

class HistoryRecord : public HistoryMedia {
public:
	HistoryRecord(not_null<Element*> element,
		not_null<RecordData*> data);
	~HistoryRecord();

	void draw(
		Painter& p,
		const QRect& r,
		TextSelection selection,
		crl::time ms) const override;
	 TextState textState(
		QPoint point,
		StateRequest request) const override;
	bool toggleSelectionByHandlerClick(
		const ClickHandlerPtr& p) const override {
		return p == _open;
	}
	bool dragItemByHandler(
		const ClickHandlerPtr& p) const override {
		return p == _open;
	}
	bool needsBubble() const override {
		return true;
	}
	bool customInfoLayout() const override {
		return false;
	}
private:
	QSize countOptimalSize() override;

	using RecordClickHandlerPtr = std::shared_ptr<RecordClickHandler>;
	RecordClickHandlerPtr _open;
	not_null<RecordData*> _data;
};