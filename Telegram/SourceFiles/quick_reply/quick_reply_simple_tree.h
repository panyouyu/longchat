#pragma once

#include "ui/rp_widget.h"
#include "rpl/rpl.h"
#include "styles/style_widgets.h"

namespace Ui {
	class IconButton;
}

namespace QuickReply {

	
class TreeItem : public Ui::RpWidget {
public:
	TreeItem(QWidget* parent, const QString& title, const QList<QString>& content, const style::TreeItem &st = st::defaultTreeItem);

	rpl::producer<> sizeUpdate() const;
protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* e) override;
	void enterEventHook(QEvent* e) override;
	void leaveEventHook(QEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
private:
	void init_content_rect(int width);
	void init_button_state();
	void update_button_state();
	void handleMousePos(QPoint pt);
	QString _title;
	QList<QString> _content;
	QList<QRect> _rect_list;
	int _hover_index = -1;
	int _press_index = -1;
	const style::TreeItem& _st;
	object_ptr<Ui::IconButton> _button;
	rpl::event_stream<> _size_update;
};

class SimpleTree : public Ui::RpWidget {
public:
	SimpleTree(QWidget* parent);
	void load();
protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* event) override;
	
private:
	std::vector<std::unique_ptr<TreeItem>> _items;
};

}

