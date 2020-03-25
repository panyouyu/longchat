#include "quick_reply/quick_reply_top_bar.h"

#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"

namespace QuickReply {

TopBar::TopBar(QWidget* parent, const style::InfoTopBar& st)
	: RpWidget(parent)
	, _st(st) {
	setAttribute(Qt::WA_OpaquePaintEvent);
}

void TopBar::setTitle(rpl::producer<QString>&& title) {
	if (_title) {
		delete _title;
	}
	_title = Ui::CreateChild<Ui::FlatLabel>(
		this,
		std::move(title),
		_st.title);
	updateControlsGeometry(width());
}

Ui::RpWidget* TopBar::pushButton(base::unique_qptr<Ui::RpWidget> button) {
	auto wrapped = std::move(button);
	auto weak = wrapped.get();
	_buttons.push_back(std::move(wrapped));
	weak->widthValue(
	) | rpl::start_with_next([this] {
		updateControlsGeometry(width());
		}, lifetime());
	return weak;
}

void TopBar::removeButton(not_null<Ui::RpWidget*> button) {
	_buttons.erase(
		std::remove(_buttons.begin(), _buttons.end(), button),
		_buttons.end());
}

int TopBar::resizeGetHeight(int newWidth) {
	updateControlsGeometry(newWidth);
	return _st.height;
}

void TopBar::updateControlsGeometry(int newWidth) {
	auto right = 0;
	for (auto& button : _buttons) {
		if (!button) continue;
		button->moveToRight(right, 0, newWidth);
		right += button->width();
	}
	if (_title) {
		_title->moveToLeft(
			_st.titlePosition.x(),
			_st.titlePosition.y(),
			newWidth);
	}
}

void TopBar::paintEvent(QPaintEvent* e) {
	Painter p(this);
	p.fillRect(e->rect(), _st.bg);
}

} // namespace QuickReply