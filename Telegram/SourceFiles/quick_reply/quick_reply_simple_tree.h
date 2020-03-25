#pragma once

#include "ui/rp_widget.h"
#include "styles/style_widgets.h"

namespace Ui {
	class IconButton;
}

namespace QuickReply {

	
class TreeItem : public Ui::RpWidget {
public:
	TreeItem(QWidget* parent, const style::TreeItem &st = st::defaultTreeItem);

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* event) override;

private:
	void updateButtonActive();

	const style::TreeItem& _st;
	object_ptr<Ui::IconButton> _button;
	QString _title;
	QList<QString> _content;
};

class SimpleTree : public Ui::RpWidget {
public:
	SimpleTree(QWidget* parent);

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* event) override;
	
private:
	std::vector<std::unique_ptr<TreeItem>> _items;
};

}

