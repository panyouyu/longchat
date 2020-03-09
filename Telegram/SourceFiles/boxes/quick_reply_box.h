#pragma once
#include "boxes/abstract_box.h"
#include "ui/rp_widget.h"

namespace Ui {
	class LabelSimple;
	class InputField;
	class DropdownMenu;
}

class QuickReplyBox : public BoxContent
{
public:
	QuickReplyBox(QWidget*, QString groupType, QString content = QString());

	rpl::producer<QString> titleSubmid() const;
	rpl::producer<QString> contentSubmit() const;

protected:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void prepare() override;
	void setInnerFocus() override;

private:
	void showDropDownList();
	void submit();
	void change();

	QString _groupType;
	QString _content;
	object_ptr<Ui::LabelSimple> _groupLabel;
	object_ptr<Ui::InputField> _filter;
	object_ptr<Ui::IconButton> _dropDown;	
	object_ptr<Ui::LabelSimple> _contentLabel;
	object_ptr<Ui::InputField> _input;
	object_ptr<Ui::DropdownMenu> _dropDownList;

	rpl::event_stream<QString> _titleSubmit;
	rpl::event_stream<QString> _contentSubmit;
};

class QuickReplyGroupBox : public BoxContent {
public:
	QuickReplyGroupBox(QWidget*, QString groupName = QString());
	rpl::producer<QString> titleSubmid() const;
protected:
	void prepare() override;
	void setInnerFocus() override;
	void resizeEvent(QResizeEvent* e) override;
	
private:
	void submit();
	void changed();

	QString _groupName;
	object_ptr<Ui::InputField> _input;

	rpl::event_stream<QString> _titleSubmit;
};