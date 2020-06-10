#include "history/media/history_media_record.h"

#include "data/data_session.h"
#include "layout.h"
#include "history/view/history_view_cursor_state.h"
#include "history/view/history_view_element.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "styles/style_history.h"

namespace {
using TextState = HistoryView::TextState;
constexpr size_t kMaxMessageSize = 3;
} // namespace



HistoryRecord::HistoryRecord(
	not_null<Element*> element, 
	not_null<RecordData*> data)
	: HistoryMedia(element)
	, _data(data) {
	_open = std::make_shared<RecordClickHandler>(_data, element->data()->fullId());	
}

HistoryRecord::~HistoryRecord() {
}

void HistoryRecord::draw(Painter& p, const QRect& r, 
	TextSelection selection, crl::time ms) const {
	if (width() < st::msgPadding.left() + st::msgPadding.right() + 1) return;
	auto paintx = 0, painty = 0, paintw = width();

	auto outbg = _parent->hasOutLayout();
	bool selected = (selection == FullSelection);

	accumulate_min(paintw, maxWidth());
	
	painty += st::historyRecordTitle.top();
	_data->title().drawLeftElided(p, st::historyRecordTitle.left(), painty,
		paintw - st::historyRecordTitle.left() - st::historyRecordTitle.right(), paintw);
	painty += _data->title().minHeight() + st::historyRecordTitle.bottom();

	QFontMetrics fm = p.fontMetrics();
	QString strElidedText;

	painty += st::historyRecordContent.top();
	p.setFont(st::historyRecordContentFont);
	p.setPen(st::historyRecordContentColor);
	for (size_t i = 0, e = std::min(_data->content().size(), kMaxMessageSize); i < e; ++i) {
		strElidedText = fm.elidedText(_data->content().at(i), Qt::ElideRight,
			paintw - st::historyRecordContent.left() - st::historyRecordContent.right(),
			Qt::TextShowMnemonic);
		p.drawTextLeft(st::historyRecordContent.left(), painty,
			paintw - st::historyRecordContent.left() - st::historyRecordContent.right(),
			strElidedText, st::historyRecordContentFont->width(_data->content().at(i)));
		painty += st::historyRecordContentFont->height;
	}
	painty += st::historyRecordContent.bottom();

	p.setPen(st::messageRecordMediaLine);
	p.drawLine(st::historyRecordTail.left(), painty, paintw - st::historyRecordTail.right(), painty);
	painty += st::historyRecordLine + st::historyRecordTail.top();
	p.setPen(st::historyRecordContentColor);
	p.setFont(st::historyRecordTailFont);
	p.drawTextLeft(st::historyRecordTail.left(), painty,
		paintw - st::historyRecordTail.left() - st::historyRecordTail.right(),
		lng_message_record_num(lt_num, QString::number(_data->content().size())), 
		st::historyRecordContentFont->width(lng_message_record_num(lt_num, QString::number(_data->content().size()))));
}

TextState HistoryRecord::textState(QPoint point, 
	StateRequest request) const {
	auto result = TextState(_parent);
	if (width() < st::msgPadding.left() + st::msgPadding.right() + 1) {
		return result;
	}
	auto paintx = 0, painty = 0, paintw = width(), painth = height();
	paintw -= st::historyRecordTitle.left() + st::historyRecordTitle.right();

	auto title_rect = QRect(st::historyRecordTitle.left(), st::historyRecordTitle.top(),
		paintw - st::historyRecordTitle.left() - st::historyRecordTitle.right(), _data->title().minHeight());
	if (title_rect.contains(point)) {
		point -= {st::historyRecordTitle.left(), st::historyRecordTitle.top()};
		return TextState(_parent,
			_data->title().getState(point, title_rect.width(), request.forText()));
	}

	auto content_rect = QRect(st::historyRecordContent.left(), 
		st::historyRecordTitle.top() + _data->title().minHeight() + st::historyRecordTitle.bottom() + st::historyRecordContent.top(),
		paintw - st::historyRecordContent.left() - st::historyRecordContent.right(),
		st::historyRecordContentFont->height * std::min(_data->content().size(), kMaxMessageSize));
	if (content_rect.contains(point)) {
		result.link = _open;
	}

	return result;
}

QSize HistoryRecord::countOptimalSize() {
	auto max_size = std::min(_data->content().size(), kMaxMessageSize);
	int height = st::historyRecordTitle.top() + _data->title().minHeight() + st::historyRecordTitle.bottom() +
		st::historyRecordContent.top() + st::historyRecordContentFont->height * max_size + st::historyRecordContent.bottom() +
		st::historyRecordLine +
		st::historyRecordTail.top() + st::historyRecordContentFont->height + st::historyRecordTail.bottom();
	return { st::historyRecordMaxWidth, height };
}

