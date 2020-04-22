#include "boxes/add_label_box.h"

#include "auth_session.h"
#include "apiwrap.h"
#include "ui/widgets/input_fields.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"

AddLabelBox::AddLabelBox(QWidget*, UserData* user)
	: _user(user)
	, _label(this, st::defaultInputField, langFactory(lng_guest_add_placeholder)) {
}

void AddLabelBox::prepare()
{
	setTitle(langFactory(lng_guest_add_label));	

	addButton(langFactory(lng_guest_add), [this] { submit(); });
	addButton(langFactory(lng_guest_cancel), [this] { closeBox(); });

	connect(_label, &Ui::InputField::submitted, [=] { submit(); });

	_label->setFocus();
	_label->resize(st::boxWideWidth - st::addLabelMargin.left() - st::addLabelMargin.right(), _label->height());
	_label->moveToLeft(st::addLabelMargin.left(), st::addLabelMargin.top());
	setDimensions(st::boxWideWidth, st::addLabelMargin.top() + _label->height() + st::addLabelMargin.bottom());
}

void AddLabelBox::submit()
{
	QString label = TextUtilities::PrepareForSending(_label->getLastText());
	if (label.isEmpty() || _user->labels().contains(label)) {
		_label->showError();
		return;
	}
	if (AuthSession::Exists()) {
		Auth().api().uploadPeerLabel(_user, label);
	}
	closeBox();
}

