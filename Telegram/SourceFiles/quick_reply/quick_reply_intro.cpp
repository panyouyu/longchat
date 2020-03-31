#include "quick_reply/quick_reply_intro.h"

#include "settings.h"
#include "auth_session.h"
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
	~QuickReplyWidget() {
		Local::writeSettings();
	}
	void importFromFile(QString& file);
protected:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;

private:	
	void titleClicked(TextListRow* row);
	void titleSwap(int index1, int index2);
	void contentClicked(TextListRow* row);
	void contentSwap(int index1, int index2);
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
	QPointer<TextListWidget> _title;
	QString _groupRow;

	not_null<Ui::ScrollArea*> _scrollContent;
	QPointer<TextListWidget> _content;
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

	_title = _scrollTitle->setOwnedWidget(object_ptr<TextListWidget>(_scrollTitle, st::quickReplyTitleList));
	_title->setRowClickCallBack([this](TextListRow* row) {	titleClicked(row); });
	_title->setRowSwapUpCallBack([this](int index1, int index2) { titleSwap(index1, index2); });

	_content = _scrollContent->setOwnedWidget(object_ptr<TextListWidget>(_scrollContent, st::quickReplyContentList));
	_content->setRowClickCallBack([this](TextListRow* row) { contentClicked(row); });
	_content->setRowSwapUpCallBack([this](int index1, int index2) { contentSwap(index1, index2); });

	auto importCsv = [this] {
		QString fileName = QFileDialog::getOpenFileName(this,
			lang(lng_quick_reply_import_csv), "/home", tr("csv (*.csv)"));

		if (fileName.isEmpty()) return;

		importFromFile(fileName);
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
					csvFile.write(QString(tmp.at(i).group + "," + tmp.at(i).content.at(j) + "\n").toLocal8Bit());
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
		if (auto row = _title->getCheckedItem()) {
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
		if (auto row = _title->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		if (auto row = _content->getCheckedItem()) {
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
		if (auto row = _title->getCheckedItem()) {
			group = row->toString();
			group = group.left(group.lastIndexOf(' '));
		}
		if (auto row = _content->getCheckedItem()) {
			content = row->toString();
		}
		if (group.isEmpty() || content.isEmpty()) return;

		QString text = lang(lng_quick_reply_remove_words) + "\n  " + content;
		_confirmBox = Ui::show(Box<ConfirmBox>(text, [this, group, content] {
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
		if (auto row = _title->getCheckedItem()) {
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
		if (auto row = _title->getCheckedItem()) {
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
				Auth().settings().setQuickReplySectionClose(group);
				_confirmBox->closeBox();
			}
		}), LayerOption::KeepOther);
	};
	QuickReply::Action removeGroup = { lang(lng_quick_reply_remove_group), remove_group, &st::quickReplyRemoveGroup, &st::quickReplyRemoveGroupOver };
	
	_title->addAction(createGroup);
	_title->addAction(renameGroup);
	_title->addAction(removeGroup);
		
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

void QuickReplyWidget::importFromFile(QString& fileName) {
	QFile file(fileName);
	if (file.open(QIODevice::ReadOnly)) {
		QString line;
		QList<QString> list;
		QString group, content;
		QuickReplyString& tmp = cRefQuickReplyStrings();
		while (!file.atEnd()) {
			line = QString::fromLocal8Bit(file.readLine());
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
	_title->resizeToWidth(_scrollTitle->width());
	_scrollContent->setGeometry(width() / 4 + 1, top, width() * 3 / 4 - 1, scroll_h);
	_content->resizeToWidth(_scrollContent->width());

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

void QuickReplyWidget::titleSwap(int index1, int index2) {
	auto& ref = cRefQuickReplyStrings();
	auto count = ref.size();
	if (count > index1 && count > index2 && index1 >= 0 && index2 >= 0) {
		std::swap(ref[index1], ref[index2]);
	}
}

void QuickReplyWidget::contentClicked(TextListRow* row) {
	_contentRow = row->toString();
}

void QuickReplyWidget::contentSwap(int index1, int index2) {
	auto& ref = cRefQuickReplyStrings();
	if (ref.contains({ _groupRow, {} })) {
		auto& content = ref[ref.indexOf({ _groupRow, {} })].content;
		auto count = content.size();
		if (count > index1 && count > index2 && index1 >= 0 && index2 >= 0) {
			std::swap(content[index1], content[index2]);
		}
	}	
}

void QuickReplyWidget::refreshTitle() {
	_title->clear();
	auto& ref = cRefQuickReplyStrings();
	for (int i = 0; i < ref.size(); i++) {
		_title->appendRow(ref.at(i).group + " (" + QString::number(ref.at(i).content.size()) + ")");
	}

	_title->setCheckedItem(ref.indexOf({ _groupRow, {} }));
	refreshContent(_groupRow);
}
void QuickReplyWidget::refreshContent(QString group) {
	_content->clear();
	if (group.isEmpty()) return;
	auto& ref = cRefQuickReplyStrings();
	if (ref.contains({ group, {} })) {
		_content->appendRows(ref[ref.indexOf({ group, {} })].content);
		_content->setCheckedItem(_contentRow);
	}
}

class IntroWidget : public Ui::RpWidget {
public:
	IntroWidget(QWidget* parent);

protected:
	void resizeEvent(QResizeEvent* e) override;
	void dragEnterEvent(QDragEnterEvent* e) override;
	void dropEvent(QDropEvent* e) override;
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
	setAcceptDrops(true);
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

const auto protocol = qsl("file:///");
const auto fileType = qsl(".csv");

void IntroWidget::dragEnterEvent(QDragEnterEvent* e) {
	if (e->mimeData()->hasText() && 
		e->mimeData()->text().startsWith(protocol) &&
		e->mimeData()->text().endsWith(fileType))
		e->acceptProposedAction();
}

void IntroWidget::dropEvent(QDropEvent* e) {
	if (rect().contains(e->pos())) {
		auto fileName = e->mimeData()->text().mid(protocol.size());
		_inner->importFromFile(fileName);
	}
}

LayerWidget::LayerWidget(QWidget*)
	: _content(this) {}

void LayerWidget::parentResized() {
	const auto parentSize = parentWidget()->size();
	auto windowWidth = parentSize.width();
	auto windowHeight = parentSize.height();
	const auto newWidth = windowWidth < st::quickReplyWidgetMaxWidth ? windowWidth : st::quickReplyWidgetMaxWidth;
	const auto newHeight = windowHeight < st::quickReplyWidgetMaxHeight ? windowHeight : st::quickReplyWidgetMaxHeight;
	const auto newLeft = (windowWidth - newWidth) >> 1;
	const auto newTop = (windowHeight - newHeight) >> 1;
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

void LayerWidget::closeHook() {
	Auth().settings().setThirdSectionQuickReplyUpdate();
}

} // namespace QuickReply