#include "quick_reply/quick_reply_intro.h"

#include "auth_session.h"
#include "settings.h"
#include "core/utils.h"
#include "ui/wrap/fade_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/widgets/shadow.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_controller.h"
#include "quick_reply/quick_reply_section.h"
#include "quick_reply/quick_reply_selector.h"
#include "quick_reply/quick_reply_top_bar.h"
#include "quick_reply/text_list.h"
#include "boxes/quick_reply_box.h"
#include "boxes/confirm_box.h"
#include "storage/localstorage.h"
#include "lang/lang_keys.h"
#include "facades.h"
#include "styles/style_settings.h"
#include "styles/style_quick_reply.h"
#include "styles/style_boxes.h"

namespace QuickReply {

class QuickReplyWidget : public Ui::RpWidget {
public:
	QuickReplyWidget(QWidget* parent);
	
protected:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;

private:
	void titleClicked(TextListRow* row);
	void contentClicked(TextListRow* row);
	void refreshTitle();
	void refreshContent(QString title);

	object_ptr<Ui::IconTextButton> _import;
	object_ptr<Ui::IconTextButton> _export;
	object_ptr<Ui::LabelSimple> _tip;
	object_ptr<Ui::IconTextButton> _manager;
	object_ptr<Ui::IconTextButton> _add;
	object_ptr<Ui::IconTextButton> _remove;
	object_ptr<Ui::IconTextButton> _modify;

	not_null<Ui::ScrollArea*> _scrollTitle;
	Ui::PaddingWrap<Ui::RpWidget>* _title = nullptr;
	QString _groupRow;

	not_null<Ui::ScrollArea*> _scrollContent;
	Ui::PaddingWrap<Ui::RpWidget>* _content = nullptr;
	QString _contentRow;

	base::unique_qptr<Ui::PopupMenu> _menu;

	QPointer<ConfirmBox> _confirmBox;

};

QuickReplyWidget::QuickReplyWidget(QWidget* parent)
	: RpWidget(parent)
	, _import(this, lang(lng_quick_reply_import), st::quickReplyImport)
	, _export(this, lang(lng_quick_reply_export), st::quickReplyExport)
	, _tip(this, st::quickReplyTipLabel, lang(lng_quick_reply_tip))
	, _manager(this, lang(lng_quick_reply_manager), st::quickReplyManager)
	, _add(this, lang(lng_quick_reply_add), st::quickReplyAdd)
	, _remove(this, lang(lng_quick_reply_delete), st::quickReplyDelete)
	, _modify(this, lang(lng_quick_reply_modify), st::quickReplyModify)
	, _scrollTitle(Ui::CreateChild<Ui::ScrollArea>(this, st::quickReplyTitleScroll))
	, _scrollContent(Ui::CreateChild<Ui::ScrollArea>(this, st::quickReplyContentScroll))
{
	auto& ref = cRefQuickReplyStrings();
	_groupRow = ref.size() ? ref.first().group : QString();
	_contentRow = ref.size() ? ref.first().content.size() ? ref.first().content.first() : QString() : QString();

	_title = _scrollTitle->setOwnedWidget(
		object_ptr<Ui::PaddingWrap<Ui::RpWidget>>(
			this,
			object_ptr<TextListWidget>(_scrollTitle, st::quickReplyTitleList),
			_title ? _title->padding() : style::margins()));

	static_cast<TextListWidget*>(_title->wrapped())->setRowClickCallBack([&](TextListRow* row) {
		titleClicked(row);
	});

	_content = _scrollContent->setOwnedWidget(
		object_ptr<Ui::PaddingWrap<Ui::RpWidget>>(
			this,
			object_ptr<TextListWidget>(_scrollContent, st::quickReplyContentList),
			_content ? _content->padding() : style::margins()));

	static_cast<TextListWidget*>(_content->wrapped())->setRowClickCallBack([&](TextListRow* row) {
		contentClicked(row);
	});

	auto importCsv = [this] {
		QString fileName = QFileDialog::getOpenFileName(this,
			lang(lng_quick_reply_import_csv), "/home", tr("csv (*.csv)"));

		if (fileName.isEmpty()) return;

		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly)) {
			QString line;
			QList<QString> list;
			QString group, content;
			QuickReplyString& tmp = cRefQuickReplyStrings();
			while (!file.atEnd()) {
				line = file.readLine();
				line = line.remove('\n');
				line = line.remove('\r');
				list = line.split(',');
				if (list.size() != 2 || list.at(0).isEmpty() || list.at(1).isEmpty()) {
					continue;
				}
				group = list.at(0);
				content = list.at(1);
				if (tmp.contains({ group, {} })) {
					if (!tmp.at(tmp.indexOf({ group, {} })).content.contains(content)) {
						tmp[tmp.indexOf({ group, {} })].content.append(content);
					}
				}
				else {
					tmp.append({ group, {content} });
				}
			}
			file.close();
			Ui::show(Box<InformBox>(lang(lng_quick_reply_import_succeed)), LayerOption::KeepOther);
			refreshTitle();
		}
		else {
			Ui::show(Box<InformBox>(lang(lng_quick_reply_import_failed)), LayerOption::KeepOther);
		}		
	};

	auto exportCsv = [this] {
		QString fileName = QFileDialog::getSaveFileName(this, lang(lng_quick_reply_import_csv),
			"/home", tr("csv (*.csv)"));

		if (fileName.isEmpty()) return;

		QuickReplyString& tmp = cRefQuickReplyStrings();

		QFile csvFile(fileName);
		if (csvFile.open(QIODevice::WriteOnly)) {
			for (int i = 0; i < tmp.size(); i++) {
				for (int j = 0; j < tmp.at(i).content.size(); j++)
					csvFile.write((tmp.at(i).group + "," + tmp.at(i).content.at(j) + "\n").toUtf8());
			}
			csvFile.close();
			Ui::show(Box<InformBox>(lang(lng_quick_reply_export_succeed)), LayerOption::KeepOther);
		}
		else {
			Ui::show(Box<InformBox>(lang(lng_quick_reply_export_failed)), LayerOption::KeepOther);
		}			
	};

	auto add = [this] {
		QString group;
		if (auto row = static_cast<TextListWidget*>(_title->wrapped())->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		auto addBox = Ui::show(Box<QuickReplyBox>(group, QString()), LayerOption::KeepOther);
		addBox->titleSubmid() | rpl::start_with_next([this](QString group) {
			_groupRow = group;
			refreshTitle();
		}, lifetime());
		addBox->contentSubmit() | rpl::start_with_next([this](QString content) {
			_contentRow = content;
			refreshTitle();
		}, lifetime());
	};

	auto modify = [this] {
		QString group, content;
		if (auto row = static_cast<TextListWidget*>(_title->wrapped())->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		if (auto row = static_cast<TextListWidget*>(_content->wrapped())->getCheckedItem()) {
			content = row->toString();
		}
		auto modifyBox = Ui::show(Box<QuickReplyBox>(group, content), LayerOption::KeepOther);
		modifyBox->titleSubmid() | rpl::start_with_next([this](QString group) {
			_groupRow = group;
			refreshTitle();
			}, lifetime());
		modifyBox->contentSubmit() | rpl::start_with_next([this](QString content) {
			_contentRow = content;
			refreshContent(_groupRow);
			}, lifetime());
	};

	auto remove = [this] {
		QString group, content;
		if (auto row = static_cast<TextListWidget*>(_title->wrapped())->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		if (auto row = static_cast<TextListWidget*>(_content->wrapped())->getCheckedItem()) {
			content = row->toString();
		}
		if (group.isEmpty() || content.isEmpty()) return;

		QString text = lang(lng_quick_reply_remove_words) + "\n  " + content;
		_confirmBox = Ui::show(Box<ConfirmBox>(text, [this, &group, &content] {
			auto& ref = cRefQuickReplyStrings();
			if (ref.contains({ group, {} })) {
				if (ref[ref.indexOf({ group, {} })].content.contains(content)) {
					int index = ref[ref.indexOf({ group, {} })].content.indexOf(content);
					ref[ref.indexOf({ group, {} })].content.removeAt(index);
					if (index == ref[ref.indexOf({ group, {} })].content.size()) {
						index--;
					}						
					_contentRow = index < 0 ? QString() : ref[ref.indexOf({ group, {} })].content[index];
					refreshTitle();
					Local::writeSettings();
					Auth().settings().setQuickReplySectionClose(group);
					_confirmBox->closeBox();
				}
			}
		}), LayerOption::KeepOther);
	};

	_import->setClickedCallback(importCsv);
	_export->setClickedCallback(exportCsv);
	_add->setClickedCallback(add);
	_modify->setClickedCallback(modify);
	_remove->setClickedCallback(remove);

	auto create_group = [this] {
		auto createBox = Ui::show(Box<QuickReplyGroupBox>(), LayerOption::KeepOther);
		createBox->titleSubmid() | rpl::start_with_next([this](QString group) {
			_groupRow = group;
			refreshTitle();
			}, lifetime());
	};
	QuickReply::Action createGroup = { lang(lng_quick_reply_create_group), create_group, &st::quickReplyCreateGroup, &st::quickReplyCreateGroupOver };
		
	auto rename_group = [this] {
		QString group;
		if (auto row = static_cast<TextListWidget*>(_title->wrapped())->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		auto renameBox = Ui::show(Box<QuickReplyGroupBox>(group), LayerOption::KeepOther);
		renameBox->titleSubmid() | rpl::start_with_next([this](QString group) {
			_groupRow = group;
			refreshTitle();
		}, lifetime());
	};
	QuickReply::Action renameGroup = { lang(lng_quick_reply_rename_group), rename_group, &st::quickReplyRenameGroup, &st::quickReplyRenameGroupOver };
	
	auto remove_group = [this] {
		QString group;
		if (auto row = static_cast<TextListWidget*>(_title->wrapped())->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		if (group.isEmpty()) return;

		QString text = lng_quick_reply_remove_group_name(lt_group, group);
		_confirmBox = Ui::show(Box<ConfirmBox>(text, [this, group] {
			auto& ref = cRefQuickReplyStrings();
			if (ref.contains({ group, {} })) {
				int index = ref.indexOf({ group, {} });
				ref.removeAt(index);
				if (index == ref.size()) {
					index--;
				}
				_groupRow = index < 0 ? QString() : ref.at(index).group;
				_contentRow = index < 0 ? QString() :
					ref.at(index).content.size() > 0 ? ref.at(index).content.first() : QString();
				refreshTitle();
				Local::writeSettings();
				_confirmBox->closeBox();
			}
		}), LayerOption::KeepOther);
	};
	QuickReply::Action removeGroup = { lang(lng_quick_reply_remove_group), remove_group, &st::quickReplyRemoveGroup, &st::quickReplyRemoveGroupOver };
	
	static_cast<TextListWidget*>(_title->wrapped())->addAction(createGroup);
	static_cast<TextListWidget*>(_title->wrapped())->addAction(renameGroup);
	static_cast<TextListWidget*>(_title->wrapped())->addAction(removeGroup);
		
	auto manage = [this, createGroup, renameGroup, removeGroup] {
		if (!_menu) {
			_menu = base::make_unique_q<Ui::PopupMenu>(this, st::quickReplyPopupMenu);
		}
		auto add_action = [this](const QuickReply::Action* action) {
			_menu->addAction(action->text, action->callback, action->icon, action->iconOver);
		};
		add_action(&createGroup);
		add_action(&renameGroup);
		add_action(&removeGroup);
		_menu->popup(QCursor::pos() - QPoint(0, _menu->height()));
	};
	_manager->setClickedCallback(manage);

	refreshTitle();
}

void QuickReplyWidget::resizeEvent(QResizeEvent* e)
{	
	int left = st::quickReplyImport.margin.left();
	int top = st::quickReplyImport.margin.top();
	_import->moveToLeft(left, top); 
	left += st::quickReplyImport.width + st::quickReplyImport.margin.right() + st::quickReplyExport.margin.left();
	_export->moveToLeft(left, top);

	int minwidth = left + st::quickReplyExport.width + st::quickReplyExport.margin.right() +
		st::quickReplyTipPadding.left() + _tip->width() + st::quickReplyTipPadding.right();
	if (width() > minwidth) _tip->show();
	else _tip->hide();
	_tip->moveToRight(st::quickReplyTipPadding.right(), st::quickReplyTipPadding.top());

	top += st::quickReplyImport.height + st::quickReplyImport.margin.bottom();
	int scroll_h = height() - top - (st::quickReplyManager.margin.top() + st::quickReplyManager.height + st::quickReplyManager.margin.bottom());
	_scrollTitle->setGeometry(0, top, width() / 4, scroll_h);
	_title->wrapped()->resizeToWidth(_scrollTitle->width());
	_scrollContent->setGeometry(width() / 4 + 1, top, width() * 3 / 4 - 1, scroll_h);
	_content->wrapped()->resizeToWidth(_scrollContent->width());

	top = height() - st::quickReplyManager.height;
	int managerx = (_scrollTitle->width() - st::quickReplyManager.width) / 2;
	_manager->moveToLeft(managerx, top);
	int right = st::quickReplyModify.margin.right();
	_modify->moveToRight(right, top);
	right += st::quickReplyModify.width + st::quickReplyModify.margin.left() + st::quickReplyDelete.margin.right();
	_remove->moveToRight(right, top); 
	right += st::quickReplyDelete.width + st::quickReplyDelete.margin.left() + st::quickReplyAdd.margin.right();
	_add->moveToRight(right, top);
}

void QuickReplyWidget::paintEvent(QPaintEvent* e) {
	Ui::RpWidget::paintEvent(e);

	Painter p(this);
	p.setPen(st::quickReplyFrame);
	int top = st::quickReplyImport.margin.top() + st::quickReplyImport.height + st::quickReplyImport.margin.bottom() - 1;
	p.drawLine(0, top, width(), top);
	p.drawLine(width() / 4, top, width() / 4, height());
}

void QuickReplyWidget::titleClicked(TextListRow* row) {
	QString group = row->toString();
	_groupRow = group.left(group.lastIndexOf(' '));
	refreshContent(_groupRow);
}

void QuickReplyWidget::contentClicked(TextListRow* row) {
	_contentRow = row->toString();
}

void QuickReplyWidget::refreshTitle() {
	static_cast<TextListWidget*>(_title->wrapped())->clear();
	auto& ref = cRefQuickReplyStrings();
	for (int i = 0; i < ref.size(); i++) {
		static_cast<TextListWidget*>(_title->wrapped())->appendRow(ref.at(i).group + " (" + QString::number(ref.at(i).content.size()) + ")");
	}

	static_cast<TextListWidget*>(_title->wrapped())->setCheckedItem(ref.indexOf({ _groupRow, {} }));
}
void QuickReplyWidget::refreshContent(QString group) {
	static_cast<TextListWidget*>(_content->wrapped())->clear();
	auto& ref = cRefQuickReplyStrings();
	if (ref.contains({ group, {} })) {
		static_cast<TextListWidget*>(_content->wrapped())->appendRows(ref[ref.indexOf({ group, {} })].content);
		static_cast<TextListWidget*>(_content->wrapped())->setCheckedItem(_contentRow);
	}
}

class IntroWidget : public Ui::RpWidget {
public:
	IntroWidget(QWidget* parent);

protected:
	void resizeEvent(QResizeEvent* e) override;
private:
	QRect contentGeometry() const;

	object_ptr<Ui::RpWidget> _wrap;
	object_ptr<TopBar> _topBar;
	object_ptr<QuickReplyWidget> _inner;
};

IntroWidget::IntroWidget(QWidget* parent)
	: RpWidget(parent)
	, _wrap(this)
	, _topBar(this, st::quickReplyTopBar)
	, _inner(_wrap){
	_wrap->setAttribute(Qt::WA_OpaquePaintEvent);
	_wrap->paintRequest(
	) | rpl::start_with_next([=](QRect clip) {
		Painter p(_wrap.data());
		p.fillRect(clip, st::boxBg);
		}, _wrap->lifetime());
	_topBar->setTitle(Lang::Viewer(lng_quick_reply_management));
	auto close = _topBar->addButton(
		base::make_unique_q<Ui::IconButton>(
			_topBar,
			st::infoLayerTopBarClose));
	close->addClickHandler([] {
		Ui::hideSettingsAndLayer();
	});
	_inner->show();
}

QRect IntroWidget::contentGeometry() const {
	return rect().marginsRemoved({ 0, _topBar->height(), 0, 0 });
}

void IntroWidget::resizeEvent(QResizeEvent* e) {
	_topBar->resizeToWidth(width());
	_wrap->setGeometry(contentGeometry());
	_inner->setGeometry(_wrap->rect());
}

LayerWidget::LayerWidget(QWidget*)
	: _content(this) {
}

void LayerWidget::parentResized() {
	const auto parentSize = parentWidget()->size();
	auto windowWidth = parentSize.width();
	auto windowHeight = parentSize.height();
	const auto newWidth = windowWidth < st::quickReplyWidgetWidth ? windowWidth : st::quickReplyWidgetWidth;
	const auto newLeft = (windowWidth - newWidth) >> 1;
	auto newTop = 0;
	if (newLeft)
		newTop = snap(windowHeight / 24, st::quickReplyLayerTopMinimal, st::quickReplyLayerTopMaximal);
	
	auto newBottom = newTop;
	auto newHeight = windowHeight - newTop - newBottom;
	setGeometry(newLeft, newTop, newWidth, newHeight);
	_content->setGeometry(0, 0, newWidth, newHeight);
}

void LayerWidget::paintEvent(QPaintEvent* e) {
	Painter p(this);

	auto clip = e->rect();
	auto r = st::boxRadius;
	auto parts = RectPart::None | 0;
	if (clip.intersects({ 0, 0, width(), r })) {
		parts |= RectPart::FullTop;
	}
	if (clip.intersects({ 0, height() - r, width(), r })) {
		parts |= RectPart::FullBottom;
	}
	if (parts) {
		App::roundRect(
			p,
			rect(),
			st::boxBg,
			BoxCorners,
			nullptr,
			parts);
	}
}

}