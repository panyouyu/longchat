#include "boxes/add_label_box.h"

#include "auth_session.h"
#include "apiwrap.h"
#include "data/data_user.h"
#include "ui/widgets/input_fields.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"

namespace {
	auto kLabelMaxLength = 20;
}

AddLabelBox::AddLabelBox(QWidget*, UserData* user)
	: _user(user)
	, _label(this, st::defaultInputField, langFactory(lng_guest_add_placeholder))
	, _desc(this, st::defaultInputField, langFactory(lng_guest_add_description)){
}

void AddLabelBox::setInnerFocus() {
	_label->setFocus();
}

void AddLabelBox::prepare()
{
	setTitle(langFactory(lng_guest_add_label));	

	addButton(langFactory(lng_guest_add), [this] { submit(); });
	addButton(langFactory(lng_guest_cancel), [this] { closeBox(); });

	connect(_label, &Ui::InputField::changed, [=] { change(); });
	connect(_label, &Ui::InputField::submitted, [=] { submit(); });
	connect(_desc, &Ui::InputField::submitted, [=] { submit(); });

	_label->setFocus();
	auto top = st::addLabelMargin.top();
	_label->resize(st::boxWideWidth - st::addLabelMargin.left() - st::addLabelMargin.right(), _label->height());
	_label->moveToLeft(st::addLabelMargin.left(), top); top += _label->height() + st::addLabelInterval;
	_desc->resize(st::boxWideWidth - st::addLabelMargin.left() - st::addLabelMargin.right(), _desc->height());
	_desc->moveToLeft(st::addLabelMargin.left(), top); top += _desc->height() + st::addLabelMargin.bottom();
	setDimensions(st::boxWideWidth, top);
}

void AddLabelBox::change() {
	if (_label->getLastText().size() > kLabelMaxLength) {
		_label->showError();
		_label->setFocus();
	}
}

void AddLabelBox::submit()
{
	QString label = TextUtilities::PrepareForSending(_label->getLastText());
	QString desc = TextUtilities::PrepareForSending(_desc->getLastText());
	if (label.isEmpty() 
		|| _user->labels().contains({ label, qsl(""), 0 })
		|| _label->getLastText().size() > kLabelMaxLength) {
		_label->showError();
		return;
	}
	if (AuthSession::Exists()) {
		Auth().api().uploadPeerLabel(_user, { label, desc, Auth().userId()});
	}
	closeBox();
}

