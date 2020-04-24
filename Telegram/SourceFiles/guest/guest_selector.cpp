#include "guest/guest_selector.h"

#include "auth_session.h"
#include "apiwrap.h"
#include "app.h"
#include "facades.h"
#include "window/window_controller.h"
#include "boxes/confirm_box.h"
#include "boxes/add_label_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/input_fields.h"
#include "ui/image/image_prepare.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "observer_peer.h"
#include "quick_reply/quick_reply_top_bar.h"
#include "lang/lang_keys.h"
#include "styles/style_guest.h"

namespace Guest {

class PropertyLabel : public Ui::RoundButton {
public:
	PropertyLabel(QWidget* parent,
		not_null<Window::Controller*> controller,
		const QString& text,
		const style::RoundButton& st);
	~PropertyLabel();
protected:
	void enterEventHook(QEvent* e) {
		_close->setVisible(true);
	}
	void leaveEventHook(QEvent* e) {
		_close->setVisible(false);
	}
private:
	not_null<Window::Controller*> _controller;
	object_ptr<Ui::IconButton> _close;
	const QString _text;
	QPointer<ConfirmBox> _confirmBox;
};

PropertyLabel::PropertyLabel(QWidget* parent, not_null<Window::Controller*> controller, const QString& text, const style::RoundButton& st)
	: RoundButton(parent, [&text] { return text; }, st)
	, _controller(controller)
	, _close(this, st::guestRemoveProperty)
	, _text(text) {
	sizeValue() | rpl::start_with_next([this](const QSize size) {
		_close->moveToRight(0, 0);
	}, lifetime());
	_close->setVisible(false);
	_close->setClickedCallback([this] {
		const auto nowActivePeer = _controller->activeChatCurrent().peer();
		if (nowActivePeer && nowActivePeer->isUser() && !nowActivePeer->isSelf() && AuthSession::Exists()) {
			if (_text == lang(lng_guest_normal_guest)) return;
			QString text = lang(lng_guest_remove_label_tip) + "\n  " + _text;
			_confirmBox = Ui::show(Box<ConfirmBox>(text, [this, nowActivePeer] {
				Auth().api().removePeerLabel(nowActivePeer, _text);
				_confirmBox->closeBox();
				_confirmBox.clear();
			}), LayerOption::KeepOther);
		}
	});
	_confirmBox.clear();
	show();
}

PropertyLabel::~PropertyLabel() {
	if (!_confirmBox.isNull()) {
		_confirmBox->closeBox();
	}
}

class InfoWidget : public Ui::RpWidget {
public:
	InfoWidget(QWidget* parent);

	void clear();
	void append(QString& title, QStringList& content);

protected:
	int resizeGetHeight(int newWidth);
private:
	std::vector<object_ptr<Ui::InputField>> _field;
};

InfoWidget::InfoWidget(QWidget* parent)
	: RpWidget(parent) {
}

void InfoWidget::clear() {
	for (auto i = _field.begin(), e = _field.end(); i != e; ++i) {
		(*i).destroy();
	}
	_field.clear();
	int height = resizeGetHeight(width());
	resize(width(), height);
}

void InfoWidget::append(QString& title, QStringList& content) {
	auto title_ptr = object_ptr<Ui::InputField>(this, st::guestTitleField, Ui::InputField::Mode::MultiLine, nullptr, QString());
	title_ptr->rawTextEdit()->setReadOnly(true);
	connect(title_ptr, &Ui::InputField::resized, [this] {
		int height = resizeGetHeight(width());
		resize(width(), height);
	});
	title_ptr->insertTag(title);

	auto content_ptr = object_ptr<Ui::InputField>(this, st::guestContentField, Ui::InputField::Mode::MultiLine, nullptr, QString());
	content_ptr->rawTextEdit()->setReadOnly(true);
	QString splice_content;
	for (auto i = content.cbegin(), e = content.cend(); i != e; ++i) {
		splice_content += (*i) + "\n";
	}
	if (splice_content.endsWith("\n")) {
		splice_content = splice_content.left(splice_content.size() - 1);
	}
	connect(content_ptr, &Ui::InputField::resized, [this] {
		int height = resizeGetHeight(width());
		resize(width(), height);
	});
	content_ptr->insertTag(splice_content);

	_field.push_back(std::move(title_ptr));
	_field.push_back(std::move(content_ptr));
}

int InfoWidget::resizeGetHeight(int newWidth) {
	if (newWidth == 0) return 0;

	int left = 0, top = 0;
	for (auto i = _field.cbegin(), e = _field.cend(); i != e; ++i) {
		(*i)->resize(newWidth, (*i)->height());
		(*i)->moveToLeft(left, top);
		top += (*i)->height();
	}
	return top;
}

class PropertyWidget : public Ui::RpWidget {
public:
	PropertyWidget(QWidget* parent, not_null<Window::Controller*> controller);

	void clear();
	void addPropertyLabel(const QString& label);
	void addPropertyLabels(const QStringList& labels);

	rpl::producer<> sizeChanged() const {
		return _sizeChanged.events();
	}
protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* e) override;

private:
	not_null<Window::Controller*> _controller;
	std::vector<object_ptr<Ui::RippleButton>> _property;
	rpl::event_stream<> _sizeChanged;
};

PropertyWidget::PropertyWidget(QWidget* parent, not_null<Window::Controller*> controller)
	: RpWidget(parent)
	, _controller(controller) {
	setAttribute(Qt::WA_OpaquePaintEvent);
	auto add = object_ptr<Ui::IconButton>(this, st::guestAddProperty);
	add->setClickedCallback([this] {
		const auto nowActivePeer = _controller->activeChatCurrent().peer();
		if (nowActivePeer && nowActivePeer->isUser() && !nowActivePeer->isSelf() && AuthSession::Exists()) {
			Ui::show(Box<AddLabelBox>(nowActivePeer->asUser()), LayerOption::KeepOther);
		}
	});
	_property.push_back(std::move(add));
}

void PropertyWidget::clear() {
	Expects(_property.size() > 0);

	for (int i = 0, n = _property.size() - 1; i < n; ++i) {
		_property[i].destroy();
	}
	_property.erase(_property.begin(), _property.end() - 1);

	_sizeChanged.fire({});
}

void PropertyWidget::addPropertyLabel(const QString& label) {
	Expects(_property.size() > 0);

	_property.insert(_property.end() - 1, object_ptr<PropertyLabel>(this, _controller, label, st::guestProperty));

	_sizeChanged.fire({});
}

void PropertyWidget::addPropertyLabels(const QStringList& labels) {
	Expects(_property.size() > 0);

	for (auto i = labels.begin(), e = labels.end(); i != e; ++i) {
		_property.insert(_property.end() - 1, object_ptr<PropertyLabel>(this, _controller, *i, st::guestProperty));
	}
	_sizeChanged.fire({});
}

int PropertyWidget::resizeGetHeight(int newWidth) {
	int left = 0, top = 0, lastLineHeight = 0;

	top += st::subTitleMargins.top() + st::subTitleFont->height + st::subTitleMargins.bottom();

	for (auto i = _property.cbegin(), e = _property.cend(); i != e;) {
		if (left + st::propertyMargins.left() + (*i)->width() + st::propertyMargins.right() <= newWidth) {
			left += st::propertyMargins.left();
			(*i)->moveToLeft(left, top + st::propertyMargins.top());
			left += (*i)->width() + st::propertyMargins.right();
			lastLineHeight = st::propertyMargins.top() + (*i)->height() + st::propertyMargins.bottom();
			++i;
		} else {
			left = 0;
			top += lastLineHeight;
			if (left + st::propertyMargins.left() + (*i)->width() + st::propertyMargins.right() > newWidth) {
				(*i)->resize(newWidth - st::propertyMargins.left() - st::propertyMargins.right(), (*i)->height());
				(*i)->update();
			}
		}
	}
	top += lastLineHeight;
	return top;	
}

void PropertyWidget::paintEvent(QPaintEvent* e) {
	Painter p(this);
	p.fillRect(rect(), st::boxBg);	
	p.setFont(st::subTitleFont);
	p.drawTextLeft(st::subTitleMargins.left(), st::subTitleMargins.top(), width(), lang(lng_guest_property));
	if (_property.size() > 0) {
		App::roundRect(p, _property.back()->geometry(), st::activeButtonBg, ImageRoundRadius::Small);
	}
}

Selector::Selector(QWidget* parent, not_null<Window::Controller*> controller, Dialogs::Key key)
	: RpWidget(parent)
	, _controller(controller)
	, _topBar(this, st::guestTopBar)
	, _scroll(this, st::guestScroll)
	, _property(this, controller)
	, _block(this, lang(lng_guest_pull_black_guest), st::shieldButton) {
	setAttribute(Qt::WA_OpaquePaintEvent);
	_topBar->setTitle(Lang::Viewer(lng_guest_topbar));
	auto close = _topBar->addButton(
		base::make_unique_q<Ui::IconButton>(
			_topBar,
			st::infoLayerTopBarClose));
	close->addClickHandler([this] {
		_controller->closeThirdSection();
	});

	_info = _scroll->setOwnedWidget(object_ptr<InfoWidget>(this));	
	_confirmBox.clear();

	_property->sizeChanged() | rpl::start_with_next([this] {
		updateControlsGeometry();
	}, lifetime());	

	_controller->activeChatValue() | rpl::start_with_next([this](Dialogs::Key key) {
		if (auto peer = key.peer()) {
			if (peer->isSelf())
				_controller->closeThirdSection();
		}
		load(key);
	}, lifetime());

	Notify::PeerUpdateViewer(
		Notify::PeerUpdate::Flag::UserInfoChanged
	) | rpl::map([](const Notify::PeerUpdate& update) {
		return update.peer->asUser();
	}) | rpl::filter([](UserData* user) {
		return user != nullptr && !user->isSelf();
	}) | rpl::start_with_next([=](not_null<UserData*> user) {
		loadInfo(user);
	}, lifetime());

	Notify::PeerUpdateViewer(
		Notify::PeerUpdate::Flag::UserLabelChanged
	) | rpl::map([](const Notify::PeerUpdate& update) {
		return update.peer->asUser();
	}) | rpl::filter([](UserData* user) {
		return user != nullptr && !user->isSelf();
	}) | rpl::start_with_next([=](not_null<UserData*> user) {
		loadLabels(user);
	}, lifetime());

	Notify::PeerUpdateViewer(
		Notify::PeerUpdate::Flag::UserShieldBlack
	) | rpl::map([](const Notify::PeerUpdate& update) {
		return update.peer->asUser();
	}) | rpl::filter([](UserData* user) {
		return user != nullptr && !user->isSelf();
	}) | rpl::start_with_next([=](not_null<UserData*> user) {
		updateBlock(user);
	}, lifetime());

	load(key);
}

Selector::~Selector() {
	if (!_confirmBox.isNull()) {
		_confirmBox->closeBox();
	}
}

void Selector::resizeEvent(QResizeEvent* event) {
	updateControlsGeometry();
}

void Selector::paintEvent(QPaintEvent* event) {
	Painter p(this);
	p.fillRect(scrollRect(), st::boxBg);
}

void Selector::load(Dialogs::Key key) {	
	if (auto peer = key.peer()) {
		UserData* user = peer->asUser();
		if (user != nullptr && !peer->isSelf()) {
			updateBlock(user);
			loadInfo(user);
			loadLabels(user);
		}
	}
}

void Selector::loadInfo(not_null<UserData*> user) {
	_info->clear();
}

void Selector::loadLabels(not_null<UserData*> user) {
	_property->clear();
	auto labels = QList<QString>::fromVector(user->labels());
	if (labels.isEmpty()) {
		labels.append(lang(lng_guest_normal_guest));
	}
	_property->addPropertyLabels(labels);
}

void Selector::updateBlock(not_null<UserData*> user) {
	bool shieldBlack = user->isShieldBlack();
	auto pull_black = [this] {
		const auto nowActivePeer = _controller->activeChatCurrent().peer();
		if (nowActivePeer && nowActivePeer->isUser() && !nowActivePeer->isSelf() && AuthSession::Exists()) {
			QString text = lng_guest_pull_black_tip(lt_user, textcmdLink(1, App::peerName(nowActivePeer)));;
			_confirmBox = Ui::show(Box<ConfirmBox>(text, [this, nowActivePeer] {
				Auth().api().pullBlackUser(nowActivePeer);
				_confirmBox->closeBox();
				_confirmBox.clear();
				}), LayerOption::KeepOther);
		}
	};
	auto pull_blacked = [] { return; };
	if (shieldBlack) {
		_block->setClickedCallback(pull_blacked);
	} else {
		_block->setClickedCallback(pull_black);
	}
	_block->setText(shieldBlack ? lang(lng_guest_pull_blacked_guest) : lang(lng_guest_pull_black_guest));
}

QRect Selector::scrollRect() const {
	int top = _topBar->height();
	int bottom = _property->height() + _block->height();
	return rect().marginsRemoved({ 0, top, 0, bottom });
}

void Selector::updateControlsGeometry() {
	if (width() == 0) return;
	int top = 0;
	_topBar->resizeToWidth(width());
	_property->resizeToWidth(width());	
	_topBar->moveToLeft(0, top); top += _topBar->height();
	_scroll->setGeometry(scrollRect());
	if (_info) {
		_info->resizeToWidth(_scroll->width());
		auto scrollTop = _scroll->scrollTop();
		_info->setVisibleTopBottom(
			scrollTop,
			scrollTop + _scroll->scrollHeight());
	}
	top += _scroll->scrollHeight();
	_property->moveToLeft(0, top); top += _property->height();
	_block->resize(width(), _block->height());
	_block->moveToLeft(0, top);	
}

} // namespace Guest
