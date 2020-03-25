#include "quick_reply/quick_reply_simple_tree.h"

#include "ui/widgets/buttons.h"
#include "styles/style_quick_reply.h"

namespace QuickReply {

TreeItem::TreeItem(QWidget* parent, const style::TreeItem& st)
	: RpWidget(parent)
	, _st(st)	
	, _button(this, st.topLeft) {
	setAttribute(Qt::WA_OpaquePaintEvent);
}

int TreeItem::resizeGetHeight(int newWidth) {
	int result = 0;
	result += _st.titlemargins.top();
	result += _st.titlefont->height;
	result += _st.titlemargins.bottom();

	int max_width = newWidth - _st.contentmargins.left() - _st.contentmargins.right();
	int signal_height = _st.contentfont->height;
	for (auto item = _content.cbegin(); item != _content.cend(); item++) {
		int num = int(_st.contentfont->width(*item) / max_width);
		result += (num + 1) * signal_height;
		result += (num > 0 ? _st.contentfont->descent : 0);
	}
	result += _content.size() * (_st.contentmargins.top() + _st.contentmargins.bottom());
	return result > _st.topLeft.height ? result : _st.topLeft.height;
}

void TreeItem::paintEvent(QPaintEvent* event) {
	Ui::RpWidget::paintEvent(event);
	Painter p(this);
	p.fillRect(rect(), st::boxBg);

	int left = 0,top = 0;
	p.setFont(_st.titlefont);
	p.setPen(_st.titlecolor);
	left = _st.titlemargins.left();
	top += _st.titlemargins.top();
	auto max_width = width() - _st.titlemargins.left() - _st.titlemargins.right();
	auto signel_height = _st.titlefont->height;
	p.drawText(QRect(left, top, max_width, signel_height), Qt::AlignLeft|Qt::AlignVCenter , _title);	
	top += signel_height;
	top += _st.titlemargins.bottom();

	left = _st.contentmargins.left();
	max_width = width() - _st.contentmargins.left() - _st.contentmargins.right();	
	signel_height = _st.contentfont->height;

	p.setFont(_st.contentfont);
	p.setPen(_st.contentcolor);
	QTextOption option(Qt::AlignLeft | Qt::AlignTop);
	option.setWrapMode(QTextOption::WordWrap);
	for (auto item = _content.cbegin(); item != _content.cend(); item++) {
		top += _st.contentmargins.top();
		int num = int(_st.contentfont->width(*item) / max_width);
		auto actural_h = (num + 1) * signel_height + (num > 0 ? _st.contentfont->descent : 0);
		p.drawText(QRect(left, top, max_width, actural_h), *item, option);
		top += actural_h;
		top += _st.contentmargins.bottom();		
	}
}

void TreeItem::updateButtonActive() {

}


SimpleTree::SimpleTree(QWidget* parent)
	: RpWidget(parent){
	setAttribute(Qt::WA_OpaquePaintEvent);
	_items.push_back(std::make_unique<TreeItem>(this));
	_items.push_back(std::make_unique<TreeItem>(this));
}

int SimpleTree::resizeGetHeight(int newWidth) {
	int left = 0, top = 0;
	for (auto item = _items.cbegin(); item != _items.cend(); item++) {
		item->get()->resizeToWidth(newWidth);
		item->get()->moveToLeft(left, top);
		top += item->get()->height();
	}
	return top;
}

void SimpleTree::paintEvent(QPaintEvent* event) {
	Ui::RpWidget::paintEvent(event);
	Painter p(this);
	p.fillRect(rect(), st::boxBg);
}

}