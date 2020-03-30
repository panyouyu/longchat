#include "quick_reply/text_list.h"

#include "ui/widgets/popup_menu.h"
#include "ui/effects/ripple_animation.h"
#include "ui/text_options.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_dialogs.h"
#include "styles/style_quick_reply.h"

namespace QuickReply {

TextListRow::TextListRow(QString text) 
{
	_text.setText(st::defaultTextStyle, text, Ui::NameTextOptions());	
}

TextListRow::~TextListRow() = default;

template <typename UpdateCallback>
void TextListRow::addRipple(const style::RippleAnimation& ripple, QSize size, QPoint point, UpdateCallback updateCallback) {
	if (!_ripple) {
		auto mask = Ui::RippleAnimation::rectMask(size);
		_ripple = std::make_unique<Ui::RippleAnimation>(ripple, std::move(mask), std::move(updateCallback));
	}
	_ripple->add(point);
}

void TextListRow::stopLastRipple() {
	if (_ripple) {
		_ripple->lastStop();
	}
}

void TextListRow::paintRipple(Painter& p, int x, int y, int outerWidth) {
	if (_ripple) {
		_ripple->paint(p, x, y, outerWidth);
		if (_ripple->empty()) {
			_ripple.reset();
		}
	}
}

TextListWidget::TextListWidget(QWidget*, const style::TextList& st)
	: _st(st)
{
}

void TextListWidget::clear()
{
	_rows.clear();
	setContexted(Selected());
	resizeToWidth(width());
}

void TextListWidget::appendRow(QString row) {
	appendRow(std::make_unique<TextListRow>(row));
}

void TextListWidget::appendRow(std::unique_ptr<TextListRow> row)
{
	Expects(row != nullptr);
	_rows.push_back(std::move(row));
	resizeToWidth(width());
}

void TextListWidget::appendRows(QList<QString> rows)
{
	for (QList<QString>::iterator row = rows.begin(); row != rows.end(); row++) {
		_rows.push_back(std::make_unique<TextListRow>(*row));
	}
	resizeToWidth(width());
}

void TextListWidget::setRowClickCallBack(Fn<void(TextListRow*)> rowClickCallBack)
{
	_rowClickCallBack = rowClickCallBack;
}

TextListRow* TextListWidget::getCheckedItem()
{
	return getRow(_contexted.index);
}

void TextListWidget::setCheckedItem(TextListRow* row)
{
	RowIndex index = row == nullptr ? RowIndex() : findRowIndex(row, RowIndex());
	auto contexted = index.value < 0
		? _rows.size() == 0 ? Selected() : Selected(0, false)
		: Selected(index, false);
	if (contexted != _contexted) {
		setContexted(contexted);
		if (_rowClickCallBack) {
			_rowClickCallBack(getCheckedItem());
		}
	}
}

void TextListWidget::setCheckedItem(QString& string)
{
	setCheckedItem(findRow(string));
}

void TextListWidget::setCheckedItem(int index)
{
	setCheckedItem(getRow(RowIndex(index)));
}

void TextListWidget::addAction(Action action)
{
	_actions.push_back(std::make_unique<Action>(action));
}

void TextListWidget::setSelected(Selected selected)
{
	updateRow(_selected.index);
	if (_selected != selected) {
		_selected = selected;
		updateRow(_selected.index);
		setCursor(_selected.action ? style::cur_pointer : style::cur_default);
	}
}

void TextListWidget::setPressed(Selected pressed)
{
	if (auto row = getRow(_pressed.index)) {
		row->stopLastRipple();
		row->stopLastActionRipple();
	}
	_pressed = pressed;
}

void TextListWidget::setContexted(Selected contexted)
{
	updateRow(_contexted.index);
	if (_contexted != contexted) {
		_contexted = contexted;
		updateRow(_contexted.index);
	}
}

void TextListWidget::moveUp(Selected selcted)
{
	RowIndex index = selcted.index;
	if (index.value <= 0 || index.value >= _rows.size() || _rows.size() < 2) return;
	swap(_rows[index.value], _rows[index.value - 1]);
	setContexted(Selected(RowIndex(index.value - 1), false));
	updateRow(index);
	updateRow(RowIndex(index.value - 1));
}

void TextListWidget::moveDown(Selected selcted)
{
	RowIndex index = _contexted.index;
	if (index.value < 0 || index.value > _rows.size() - 1 || _rows.size() < 2) return;
	swap(_rows[index.value], _rows[index.value + 1]);
	setContexted(Selected(RowIndex(index.value + 1), false));
	updateRow(index);
	updateRow(RowIndex(index.value + 1));
}

void TextListWidget::selectByMouse(QPoint globalPosition)
{
	_mouseSelection = true;
	_lastMousePosition = globalPosition;
	const auto point = mapFromGlobal(globalPosition);
	auto in = parentWidget()->rect().contains(parentWidget()->mapFromGlobal(globalPosition));
	auto selected = Selected();
	auto rowsPointY = point.y();
	selected.index.value = (in && rowsPointY >= 0 && rowsPointY < RowsCount() * _st.item.height) ? (rowsPointY / _st.item.height) : -1;
	if (selected.index.value >= 0) {
		auto row = getRow(selected.index);		
		if (getActionRect(row, selected.index).contains(point)) {
			selected.action = true;
		}
	}

	setSelected(selected);
}

void TextListWidget::updateRow(not_null<TextListRow*> row, RowIndex hint)
{
	updateRow(findRowIndex(row, hint));
}

void TextListWidget::updateRow(RowIndex index)
{
	if (index.value < 0) {
		return;
	}
	auto row = getRow(index);
	update(0, getRowTop(index), width(), _st.item.height);
}

int TextListWidget::getRowTop(RowIndex index) const {
	if (index.value >= 0) {
		return index.value * _st.item.height;
	}
	return -1;
}

TextListRow* TextListWidget::getRow(RowIndex index)
{
	if (index.value >= 0 && index.value < _rows.size()) {		
		return _rows[index.value].get();
	}
	return nullptr;
}

TextListWidget::RowIndex TextListWidget::findRowIndex(not_null<TextListRow*> row, RowIndex hint)
{
	auto result = hint;
	if (getRow(result) == row) {
		return result;
	}

	auto count = RowsCount();
	for (result.value = 0; result.value != count; ++result.value) {
		if (getRow(result) == row) {
			return result;
		}
	}
	result.value = -1;
	return result;
}

TextListRow* TextListWidget::findRow(QString& string)
{
	TextListRow* result = nullptr;
	auto count = RowsCount();
	for (int i = 0; i != count; ++i) {
		result = getRow(RowIndex(i));
		if (result->toString() == string) {
			return result;
		}
	}
	result = nullptr;
	return result;
}

QRect TextListWidget::getActionRect(not_null<TextListRow*> row, RowIndex index) const
{
	return QRect();
}

void TextListWidget::paintRow(Painter& p, crl::time ms, RowIndex index)
{
	auto row = getRow(index);
	Assert(row != nullptr);
	
	auto& bg = (_contexted.index == index)
		? _st.item.textBgOver
		: (_selected.index == index)
		? _st.item.textBgHover
		: _st.item.textBg;
	p.fillRect(0, 0, width(), _st.item.height, bg);
	row->paintRipple(p, 0, 0, width());

	auto& text = row->text();
	auto textx = _st.item.textPosition.x();
	auto textw = width() - textx;

	auto& fg = (_contexted.index == index)
		? _st.item.textFgOver
		: (_selected.index == index)
		? _st.item.textFgHover
		: _st.item.textFg;
	p.setPen(fg);
	text.drawLeftElided(p, textx, _st.item.textPosition.y(), textw, width());
}

void TextListWidget::handleMouseMove(QPoint globalPosition)
{
	if (!_lastMousePosition) {
		_lastMousePosition = globalPosition;
		return;
	}
	else if (!_mouseSelection
		&& *_lastMousePosition == globalPosition) {
		return;
	}
	selectByMouse(globalPosition);
}

void TextListWidget::mousePressReleased(Qt::MouseButton button)
{
	updateRow(_pressed.index);
	updateRow(_selected.index);

	auto pressed = _pressed;
	setPressed(Selected());
	setContexted(pressed);
	if (button == Qt::LeftButton && pressed == _selected) {
		if (auto row = getRow(pressed.index)) {
			if (_rowClickCallBack)
				_rowClickCallBack(row);
		}
	}
}

int TextListWidget::resizeGetHeight(int newWidth)
{
	return _rows.size() * _st.item.height;
}

void TextListWidget::paintEvent(QPaintEvent* e)
{
	Painter p(this);

	auto clip = e->rect();
	p.fillRect(clip, _st.listBg);

	auto ms = crl::now();
	auto yFrom = clip.y();
	auto yTo = clip.y() + clip.height();
	auto count = RowsCount();
	if (count > 0) {
		auto from = floorclamp(yFrom, _st.item.height, 0, count);
		auto to = ceilclamp(yTo, _st.item.height, 0, count);
		p.translate(0, from * _st.item.height);
		for (auto index = from; index != to; ++index) {
			paintRow(p, ms, RowIndex(index));
			p.translate(0, _st.item.height);
		}
	}
}

void TextListWidget::enterEventHook(QEvent* e)
{
	setMouseTracking(true);
}

void TextListWidget::leaveEventHook(QEvent* e)
{
	setMouseTracking(false);
	if (_mouseSelection) {
		setSelected(Selected());
		_mouseSelection = false;
		_lastMousePosition = std::nullopt;
	}
}

void TextListWidget::mouseMoveEvent(QMouseEvent* e)
{
	handleMouseMove(e->globalPos());
}

void TextListWidget::mousePressEvent(QMouseEvent* e)
{
	_pressButton = e->button();
	selectByMouse(e->globalPos());
	setPressed(_selected);
	if (auto row = getRow(_selected.index)) {
		auto updateCallback = [this, row, hint = _selected.index]{
			updateRow(row, hint);
		};
		auto size = QSize(width(), _st.item.height);
		auto point = mapFromGlobal(QCursor::pos()) - QPoint(0, getRowTop(_selected.index));
		auto& ripple = _selected == _contexted ? _st.item.rippleOver : _st.item.ripple;
		row->addRipple(ripple, size, point, std::move(updateCallback));
	}
	if (anim::Disabled()) {
		mousePressReleased(e->button());
	}
}

void TextListWidget::mouseReleaseEvent(QMouseEvent* e)
{
	mousePressReleased(e->button());
}

void TextListWidget::contextMenuEvent(QContextMenuEvent* e)
{
	if (_contextMenu) {
		_contextMenu->deleteLater();
		_contextMenu = nullptr;
	}
	setContexted(Selected());
	if (e->reason() == QContextMenuEvent::Mouse) {
		handleMouseMove(e->globalPos());
	}
	setPressed(_selected);
	setContexted(_selected);
	if (_pressButton != Qt::LeftButton) {
		mousePressReleased(_pressButton);
	}

	_contextMenu = base::make_unique_q<Ui::PopupMenu>(this, st::quickReplyPopupMenu);
	auto addAction = [this](Action* action) {
		_contextMenu->addAction(action->text, action->callback, action->icon, action->iconOver);
	};
	for (int i = 0; i < _actions.size(); i++) {
		addAction(_actions.at(i).get());
	}

	if (const auto row = getRow(_contexted.index)) {		
		if (_contexted.index.value != 0) {
			_contextMenu->addAction(lang(lng_quick_reply_move_up), 
				[this] { moveUp(_contexted); },
				&st::quickReplyMoveUp, 
				&st::quickReplyMoveUpOver);
		}
		if (_contexted.index.value != _rows.size() - 1) {
			_contextMenu->addAction(lang(lng_quick_reply_move_down), 
				[this] { moveDown(_contexted); },
				&st::quickReplyMoveDown, 
				&st::quickReplyMoveDownOver);
		}
	}
	if (_contextMenu) {
		_contextMenu->setDestroyedCallback(crl::guard(
			this,
			[this] {
				handleMouseMove(QCursor::pos());
			}));
		_contextMenu->popup(e->globalPos());
		e->accept();
	}
}

} // namespace QuickReply