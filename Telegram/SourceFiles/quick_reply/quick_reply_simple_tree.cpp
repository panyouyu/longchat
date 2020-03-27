#include "quick_reply/quick_reply_simple_tree.h"

#include "auth_session.h"
#include "mainwidget.h"
#include "ui/widgets/buttons.h"
#include "styles/style_quick_reply.h"

namespace QuickReply {

TreeItem::TreeItem(QWidget* parent, const QString& title, const QList<QString>& content, const style::TreeItem& st)
	: RpWidget(parent)
	, _title(title)
	, _content(content)
	, _st(st)	
	, _button(this, st.topLeft) {
	init_button_state();
	_button->setClickedCallback([this] {
		update_button_state();
		_size_update.fire({});
	});
}

rpl::producer<> TreeItem::sizeUpdate() const {
	return _size_update.events();
}

int TreeItem::resizeGetHeight(int newWidth) {
	int result = 0;
	result += _st.titlemargins.top();
	result += _st.titlefont->height;
	result += _st.titlemargins.bottom();

	init_content_rect(newWidth);
	if (Auth().settings().quickReplySectionOpened(_title)) {
		int count = std::max(_content.size(), _rect_list.size());
		result += (_st.contentmargins.top() + _st.contentmargins.bottom()) * count;
		for (int i = 0; i != count; ++i) {
			result += _rect_list.at(i).height();
		}
	}
	return result;
}

void TreeItem::paintEvent(QPaintEvent* e) {
	Ui::RpWidget::paintEvent(e);
	Painter p(this);
	p.fillRect(rect(), st::boxBg);

	p.setFont(_st.titlefont);
	p.setPen(_st.titlecolor);
	QRect title_rect = QRect(_st.titlemargins.left(), _st.titlemargins.top(), width() - _st.titlemargins.left() - _st.titlemargins.right(), _st.titlefont->height);
	p.drawText(title_rect, style::al_left, _title);

	if (Auth().settings().quickReplySectionOpened(_title)) {
		p.setFont(_st.contentfont);
		p.setPen(_st.contentcolor);
		QTextOption option(style::al_left);
		option.setWrapMode(QTextOption::WordWrap);
		for (int i = 0, count = std::max(_content.size(), _rect_list.size()); i < count; i++) {
			p.drawText(_rect_list.at(i), _content.at(i), option);
		}
	}	
}

void TreeItem::enterEventHook(QEvent* e) {
	setMouseTracking(true);
	_press_index = _hover_index = -1;
}

void TreeItem::leaveEventHook(QEvent* e) {
	setMouseTracking(false);
	_press_index = _hover_index = -1;
}

void TreeItem::mousePressEvent(QMouseEvent* e)
{	
	_press_index = _hover_index;
}

void TreeItem::mouseReleaseEvent(QMouseEvent* e)
{
	if ((_press_index == _hover_index) &&
		(_hover_index >= 0) && 
		(_rect_list.size() > _hover_index) && 
		(_content.size() > _hover_index)) {
		if (App::main())
			App::main()->setInputMsg(_content.at(_hover_index));
	}
}

void TreeItem::mouseMoveEvent(QMouseEvent* e) {
	handleMousePos(e->pos());
}

void TreeItem::init_content_rect(int newWidth) {
	if (Auth().settings().quickReplySectionOpened(_title)) {
		_rect_list.clear();
		auto left = _st.contentmargins.left();
		auto top = _st.titlemargins.top() + _st.titlefont->height + _st.titlemargins.bottom();

		auto width = newWidth - _st.contentmargins.left() - _st.contentmargins.right();
		auto signel_height = _st.contentfont->height;
		for (auto item = _content.cbegin(), end = _content.cend(); item != end; ++item) {
			top += _st.contentmargins.top();
			int num = int(_st.contentfont->width(*item) / width);
			auto actural_h = (num + 1) * signel_height + (num > 0 ? _st.contentfont->descent : 0);
			_rect_list.append({ left, top, width, actural_h });
			top += actural_h;
			top += _st.contentmargins.bottom();
		}
	}	
}

void TreeItem::init_button_state() {
	if (Auth().settings().quickReplySectionOpened(_title)) {
		_button->setIconOverride(&st::quickReplyTreeItemBottomIcon, &st::quickReplyTreeItemBottomOver);		
	}
	else {
		_button->setIconOverride(&st::quickReplyTreeItemRightIcon, &st::quickReplyTreeItemRightIconOver);
	}
	_button->forceRippled();
}

void TreeItem::update_button_state()
{
	if (Auth().settings().quickReplySectionOpened(_title)) {
		Auth().settings().setQuickReplySectionClose(_title);
		_button->setIconOverride(&st::quickReplyTreeItemRightIcon, &st::quickReplyTreeItemRightIconOver);
	}
	else {
		Auth().settings().setQuickReplySectionOpen(_title);
		_button->setIconOverride(&st::quickReplyTreeItemBottomIcon, &st::quickReplyTreeItemBottomOver);
	}
	_button->forceRippled();
}

void TreeItem::handleMousePos(QPoint pt) {
	_hover_index = [=] {
		for (int i = 0, e = _rect_list.size(); i != e; ++i) {
			if (_rect_list.at(i).contains(pt))
				return i;
		}
		return -1;
	}();

	setCursor(_hover_index >= 0 ? style::cur_pointer : style::cur_default);
}

SimpleTree::SimpleTree(QWidget* parent)
	: RpWidget(parent){
	load();
}

void SimpleTree::load() {
	_items.clear();
	auto &list = cRefQuickReplyStrings();
	for (auto i = list.cbegin(), e = list.cend(); i != e; ++i) {
		_items.push_back(std::make_unique<TreeItem>(this, i->group, i->content));
	}
	for (auto i = _items.cbegin(), e = _items.cend(); i != e; ++i) {
		i->get()->show();
		i->get()->sizeUpdate() | rpl::start_with_next([this] { resize(width(), resizeGetHeight(width())); }, lifetime());
	}
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