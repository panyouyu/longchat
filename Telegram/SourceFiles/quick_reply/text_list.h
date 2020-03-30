#pragma once

#include "ui/rp_widget.h"
#include "styles/style_widgets.h"

namespace Ui {
	class RippleAnimation;
	class PopupMenu;
}

namespace QuickReply {

class TextListRow
{
public:
	TextListRow(QString text = QString());
	virtual ~TextListRow();

	const Text& text() const {
		return _text;
	}

	const QString toString() const {
		return _text.toString();
	}

	virtual QSize actionSize() const {
		return QSize();
	}
	virtual QMargins actionMargins() const {
		return QMargins();
	}
	virtual void addActionRipple(QPoint point, Fn<void()> updateCallback) {
	}
	virtual void stopLastActionRipple() {
	}

	template <typename UpdateCallback>
void addRipple(
	const style::RippleAnimation& st,
	QSize size,
	QPoint point,
	UpdateCallback updateCallback);
void stopLastRipple();
void paintRipple(Painter& p, int x, int y, int outerWidth);

private:
	Text _text;
	std::unique_ptr<Ui::RippleAnimation> _ripple;
};

struct Action 
{
	QString text;
	Fn<void()> callback;
	const style::icon* icon;
	const style::icon* iconOver;
};

class TextListWidget : public Ui::RpWidget
{
public:
	TextListWidget(QWidget*, const style::TextList& st = st::defaultTextList);
	~TextListWidget() {}

	void clear();
	void appendRow(QString row);
	void appendRow(std::unique_ptr<TextListRow> row);
	void appendRows(QList<QString> rows);
	void setRowClickCallBack(Fn<void(TextListRow*)> rowClickCallBack);
	TextListRow* getCheckedItem();
	void setCheckedItem(TextListRow* row);
	void setCheckedItem(QString& string);
	void setCheckedItem(int index);
	void addAction(Action action);	
protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* e) override;
	void enterEventHook(QEvent* e) override;
	void leaveEventHook(QEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void contextMenuEvent(QContextMenuEvent* e) override;

private:
	int RowsCount() const {
		return _rows.size();
	}

	struct RowIndex {
		RowIndex() {
		}
		explicit RowIndex(int value) : value(value) {
		}
		int value = -1;
	};
	friend inline bool operator==(RowIndex a, RowIndex b) {
		return (a.value == b.value);
	}
	friend inline bool operator!=(RowIndex a, RowIndex b) {
		return !(a == b);
	}
	struct Selected {
		Selected() {
		}
		Selected(RowIndex index, bool action) : index(index), action(action) {
		}
		Selected(int index, bool action) : index(index), action(action) {
		}
		RowIndex index;
		bool action = false;
	};
	friend inline bool operator==(Selected a, Selected b) {
		return (a.index == b.index) && (a.action == b.action);
	}
	friend inline bool operator!=(Selected a, Selected b) {
		return !(a == b);
	}

	void setSelected(Selected selected);
	void setPressed(Selected pressed);
	void setContexted(Selected contexted);
	void moveUp(Selected selcted);
	void moveDown(Selected selcted);

	void selectByMouse(QPoint globalPosition);
	void updateRow(not_null<TextListRow*> row, RowIndex hint);
	void updateRow(RowIndex index);
	int getRowTop(RowIndex row) const;
	TextListRow* getRow(RowIndex element);
	RowIndex findRowIndex(not_null<TextListRow*> row, RowIndex hint = RowIndex());
	TextListRow* findRow(QString& string);
	QRect getActionRect(not_null<TextListRow*> row, RowIndex index) const;
	void paintRow(Painter& p, crl::time ms, RowIndex index);

	void handleMouseMove(QPoint globalPosition);
	void mousePressReleased(Qt::MouseButton button);

	style::TextList _st;

	Selected _selected;
	Selected _pressed;
	Selected _contexted;

	Fn<void(TextListRow*)> _rowClickCallBack;

	bool _mouseSelection = false;
	std::optional<QPoint> _lastMousePosition;
	Qt::MouseButton _pressButton = Qt::LeftButton;

	std::vector<std::unique_ptr<TextListRow>> _rows;
	std::vector<std::unique_ptr<Action>> _actions;

	base::unique_qptr<Ui::PopupMenu> _contextMenu;
};
} // namespace QuickReply