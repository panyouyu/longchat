#pragma once

#include "boxes/abstract_box.h"
#include "data/data_user.h"

namespace Ui {
	class InputField;
}

class AddLabelBox : public BoxContent {
public:
	AddLabelBox(QWidget*, UserData* user);

protected:
	void prepare() override;

private:
	void submit();
	not_null<UserData*> _user;
	object_ptr<Ui::InputField> _label;
};