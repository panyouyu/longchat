#include "guest/guest_section.h"

#include "guest/guest_selector.h"
#include "window/window_controller.h"

namespace Guest {

Memento::Memento(object_ptr<Selector> selector)
	: _selector(std::move(selector)) {
}

object_ptr<Window::SectionWidget> Memento::createWidget(
	QWidget* parent,
	not_null<Window::Controller*> controller,
	Window::Column column,
	const QRect& geometry) {
	auto result = object_ptr<Section>(
		parent,
		controller,
		std::move(_selector));
	result->setGeometry(geometry);
	return std::move(result);
}

Section::Section(QWidget* parent, not_null<Window::Controller*> controller, Dialogs::Key key)
	: Section(
		parent,
		controller,
		object_ptr<Selector>(this, controller, key)) {
}

Section::Section(
	QWidget* parent,
	not_null<Window::Controller*> controller,
	object_ptr<Selector> selector)
	: Window::SectionWidget(parent, controller)
	, _selector(std::move(selector)) {
	_selector->setParent(this);
}

bool Section::showInternal(not_null<Window::SectionMemento*> memento, const Window::SectionShow& params) {
	return true;
}

void Section::resizeEvent(QResizeEvent* event) {
	if (_selector) _selector->setGeometry(rect());
}

} // namespace Guest