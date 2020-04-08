#include "quick_reply/quick_reply_selector.h"

#include "auth_session.h"
#include "app.h"
#include "mainwindow.h"
#include "window/window_controller.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/scroll_area.h"
#include "quick_reply/quick_reply_simple_tree.h"
#include "quick_reply/quick_reply_top_bar.h"
#include "quick_reply/quick_reply_intro.h"
#include "lang/lang_keys.h"
#include "styles/style_quick_reply.h"
#include "styles/style_boxes.h"

namespace QuickReply {

Selector::Selector(QWidget *parent, not_null<Window::Controller *> controller)
    : RpWidget(parent)
	, _controller(controller)
	, _topBar(this, st::quickReplyTopBar)
	, _scroll(Ui::CreateChild<Ui::ScrollArea>(this, st::quickReplyTitleScroll))
	, _open(this, lang(lng_quick_reply_open)){
	_topBar->setTitle(Lang::Viewer(lng_quick_reply));
	auto close = _topBar->addButton(
		base::make_unique_q<Ui::IconButton>(
			_topBar,
			st::infoLayerTopBarClose));
	close->addClickHandler([this] {
		_controller->closeThirdSection();
	});
	_open->setClickedCallback([this] {
		App::wnd()->showSpecialLayer(Box<QuickReply::LayerWidget>(), anim::type::instant);
	});
	Auth().settings().thirdSectionQuickReplyUpdate() |
		rpl::start_with_next([this] {
			_inner->load();
			updateControlsGeometry();
		}, lifetime());
	_inner = _scroll->setOwnedWidget(object_ptr<SimpleTree>(this));
	_inner->move(0, 0);
	updateControlsGeometry();
}

Selector::~Selector() {}

void Selector::resizeEvent(QResizeEvent* event) {
	updateControlsGeometry();
}

void Selector::paintEvent(QPaintEvent* event) {
	Ui::RpWidget::paintEvent(event);
	Painter p(this);
	p.fillRect(scrollRect(), st::boxBg);
}

QRect Selector::scrollRect() const {
	return rect().marginsRemoved({ 0, _topBar->height(), 0, _open->height()});
}

void Selector::updateControlsGeometry() {
	_topBar->resizeToWidth(width());
	_open->resizeToWidth(width());
	_topBar->moveToLeft(0, 0);
	_scroll->setGeometry(scrollRect());
	_inner->resizeToWidth(_scroll->width());
	auto scrollTop = _scroll->scrollTop();
	_inner->setVisibleTopBottom(
		scrollTop,
		scrollTop + _scroll->scrollHeight());
	
	_open->moveToLeft(0, height() - _open->height());
}

} // namespace QuickReply


