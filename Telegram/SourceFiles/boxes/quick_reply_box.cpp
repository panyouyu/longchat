#include "boxes/quick_reply_box.h"

#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/input_fields.h"
#include "ui/widgets/dropdown_menu.h"
#include "settings.h"
#include "lang/lang_keys.h"
#include "storage/localstorage.h"
#include "styles/style_boxes.h"
#include "styles/style_quick_reply.h"

QuickReplyBox::QuickReplyBox(QWidget*, QString groupType, QString content)
	: _groupType(groupType)
	, _content(content)
	, _groupLabel(this, st::quickReplyLabel, lang(lng_quick_reply_group))
	, _filter(this, st::quickReplyGroupInput, langFactory(lng_quick_reply_group_name), groupType)
	, _dropDown(this, st::dropDownButton)
	, _contentLabel(this, st::quickReplyLabel, lang(lng_quick_reply_content))
	, _input(this, st::quickReplyInput,	Ui::InputField::Mode::MultiLine,
		langFactory(lng_quick_reply_input_words), content)
	, _dropDownList(this, st::dropDownList) {
	_dropDown->addClickHandler([this] { showDropDownList(); });
	_dropDownList->setMaxHeight(st::dropDownListMaxHeight);	
	_dropDownList->setHiddenCallback([this] {
		_dropDown->setForceRippled(false);
		});
	_dropDownList->setShowStartCallback([this] {
		_dropDown->setForceRippled(true);
		});
	_dropDownList->setHideStartCallback([this] {
		_dropDown->setForceRippled(false);
		});
	_dropDownList->hideFast();

	connect(_filter, &Ui::InputField::changed, this, [this] { change(); });
	connect(_filter, &Ui::InputField::submitted, this, [this] { _input->setFocus(); });
	connect(_input, &Ui::InputField::changed, this, [this] { change(); });
	connect(_input, &Ui::InputField::submitted, this, [this] { submit(); });

	const auto addAction = [=](
		const QString& text,
		Fn<void()> callback) {
			return _dropDownList->addAction(text, std::move(callback));
	};
	QString text = QString();
	auto& ref = cRefQuickReplyStrings();
	if (ref.isEmpty()) {
		addAction(text, [this, text] { _filter->setText(text); });
	}
	for (int i = 0; i < ref.size(); i++) {
		text = ref.at(i).group;
		addAction(text, [this, text] { _filter->setText(text); });
	}
}

rpl::producer<QString> QuickReplyBox::titleSubmid() const
{	
	return _titleSubmit.events_starting_with_copy(_filter->getLastText());
}

rpl::producer<QString> QuickReplyBox::contentSubmit() const
{
	return _contentSubmit.events_starting_with_copy(_input->getLastText());
}

void QuickReplyBox::resizeEvent(QResizeEvent* e) {
	int top = st::quickReplygroupLabelMargin.top();

	_groupLabel->moveToLeft(st::quickReplygroupLabelMargin.left(), top);
	top += _groupLabel->height() + st::quickReplygroupLabelMargin.bottom() + st::quickReplygroupComboMargin.top();
	
	_dropDown->moveToRight(st::quickReplygroupComboMargin.right(), top + (_filter->height() - _dropDown->height()) / 2);
	
	_filter->resize(width() - st::quickReplygroupComboMargin.left() - st::quickReplygroupComboMargin.right(), _filter->height());
	_filter->moveToLeft(st::quickReplygroupComboMargin.left(), top);

	_dropDownList->resizeToWidth(_filter->width());
	
	top += _filter->height() + st::quickReplygroupComboMargin.bottom() + st::quickReplycontentLabelMargin.top();

	int x = st::quickReplygroupComboMargin.left();
	int y = _filter->y() + _filter->height();
	_dropDownList->moveToLeft(x, y);

	_contentLabel->moveToLeft(st::quickReplycontentLabelMargin.left(), top);	
	top += _contentLabel->height() + st::quickReplycontentLabelMargin.bottom() + st::quickReplyInputMargin.top();

	_input->resize(width() - st::quickReplyInputMargin.left() - st::quickReplyInputMargin.right(), _input->height());
	_input->moveToLeft(st::quickReplyInputMargin.left(), top);
}

void QuickReplyBox::paintEvent(QPaintEvent* e)
{
	BoxContent::paintEvent(e);
	QPainter p(this);
	p.setPen(QPen(st::defaultQuickReplyBox.quickReplyFrameColor));

	int x = st::quickReplyInputMargin.left();
	int y = height() - st::quickReplyInput.heightMax - st::quickReplyInputMargin.bottom();
	int w = width() - st::quickReplyInputMargin.left() - st::quickReplyInputMargin.right();
	int h = st::quickReplyInput.heightMax;
	p.drawRect(QRect(x, y, w, h).adjusted(-8, -8, 8, 8));

	x = st::quickReplygroupComboMargin.left();
	y = st::quickReplygroupLabelMargin.top() + _groupLabel->height() + st::quickReplygroupLabelMargin.bottom()
		+ st::quickReplygroupComboMargin.top();
	w = width() - st::quickReplygroupComboMargin.left() - st::quickReplygroupComboMargin.right();
	h = st::quickReplyGroupInput.heightMax;
	p.drawRect(QRect(x, y, w, h).adjusted(-8, -8, 8, 8));
}

void QuickReplyBox::prepare()
{
	setTitle(_content.isEmpty() ? langFactory(lng_quick_reply_add_words) : langFactory(lng_quick_reply_modify_words));
	addButton(_content.isEmpty() ? langFactory(lng_quick_reply_add) : langFactory(lng_quick_reply_save), [this] {submit(); });
	addButton(langFactory(lng_quick_reply_cancel), [this] { closeBox(); });

	int width = st::quickReplyAddWordsWidth;
	int height = st::quickReplygroupLabelMargin.top() + _groupLabel->height() + st::quickReplygroupLabelMargin.bottom()
		+ st::quickReplygroupComboMargin.top() + _filter->height() + st::quickReplygroupComboMargin.bottom()
		+ st::quickReplycontentLabelMargin.top() + _contentLabel->height() + st::quickReplycontentLabelMargin.bottom()
		+ st::quickReplyInputMargin.top() + _input->height() + st::quickReplyInputMargin.bottom();
	setDimensions(width, height);
}

void QuickReplyBox::setInnerFocus()
{
	_input->setFocusFast();
}

void QuickReplyBox::showDropDownList()
{
	if (_dropDownList->isHidden()) {
		_dropDownList->showAnimated(Ui::PanelAnimation::Origin::TopRight);
		return;
	}

	if (!_dropDownList->isHiding()) {
		_dropDownList->hideAnimated(Ui::InnerDropdown::HideOption::IgnoreShow);
		return;
	}
}

void QuickReplyBox::submit() {
	if (_filter->empty()) {
		_filter->showError();
		_filter->setFocus();
		return;
	}
	if (_input->empty() || _input->getLastText() == _content) {
		_input->showError();
		_input->setFocus();
		return;
	}

	QString group = _filter->getLastText();
	QString str = _input->getLastText();
	auto& ref = cRefQuickReplyStrings();
	if (ref.contains({ group, {} })) {
		if (_content.isEmpty()) {
			if (!ref[ref.indexOf({ group, {} })].content.contains(str)) {
				ref[ref.indexOf({ group, {} })].content.append(str);
			}
		}
		else {
			if (ref[ref.indexOf({ group, {} })].content.contains(_content)) {
				ref[ref.indexOf({ group, {} })].content[ref[ref.indexOf({ group, {} })].content.indexOf(_content)] = str;
			}
		}
	}
	else {
		ref.append({ group, { str } });
		_titleSubmit.fire_copy(group);
	}
	_contentSubmit.fire_copy(str);
	Local::writeSettings();
	closeBox();
}

void QuickReplyBox::change() {
	if (_filter->empty()) {
		_filter->showError();
		_filter->setFocus();
		return;
	}
	if (_input->empty() || _input->getLastText() == _content) {
		_input->showError();
		_input->setFocus();
		return;
	}
}

QuickReplyGroupBox::QuickReplyGroupBox(QWidget*, QString groupName)
	: _groupName(groupName)
	, _input(this, st::defaultInputField, groupName.isEmpty() ? 
		langFactory(lng_quick_reply_create_group) : langFactory(lng_quick_reply_rename_group_name), groupName)
{
	connect(_input, &Ui::InputField::changed, [this] { changed(); });
	connect(_input, &Ui::InputField::submitted, [this] { submit(); });
	_input->showError();
}

rpl::producer<QString> QuickReplyGroupBox::titleSubmid() const
{
	return _titleSubmit.events_starting_with_copy(_input->getLastText());
}

void QuickReplyGroupBox::prepare()
{
	setTitle(_groupName.isEmpty() ? 
		langFactory(lng_quick_reply_create_group_name) : langFactory(lng_quick_reply_rename_group));
	addButton(_groupName.isEmpty() ? 
		langFactory(lng_quick_reply_add) : langFactory(lng_quick_reply_save), [this] { submit(); });
	addButton(langFactory(lng_quick_reply_cancel), [=] { closeBox(); });
	setDimensions(st::quickReplyGroupInputWidth, 
		st::quickReplyRenameGroupMargin.top() + _input->height() + st::quickReplyRenameGroupMargin.bottom());
}

void QuickReplyGroupBox::setInnerFocus()
{
	_input->setFocusFast();
}

void QuickReplyGroupBox::resizeEvent(QResizeEvent* e) {
	_input->resize(width() - st::quickReplyRenameGroupMargin.left() - st::quickReplyRenameGroupMargin.right(), _input->height());
	_input->moveToLeft(st::quickReplyRenameGroupMargin.left(), st::quickReplyRenameGroupMargin.top());
}

void QuickReplyGroupBox::submit() {
	QString groupName = _input->getLastText();
	auto& ref = cRefQuickReplyStrings();
	if (_input->empty() || ref.contains({ groupName, {} })) {
		_input->showError();
		return;
	}
	
	if (ref.contains({ _groupName, {} })) {
		ref[ref.indexOf({ _groupName, {} })].group = groupName;
	}
	else {
		ref.append({ groupName, {} });
	}
	_titleSubmit.fire_copy(groupName);
	Local::writeSettings();
	closeBox();
}

void QuickReplyGroupBox::changed() {
	auto& ref = cRefQuickReplyStrings();
	if (_input->empty() || ref.contains({ _input->getLastText(), {} })) {
		_input->showError();
	}
}