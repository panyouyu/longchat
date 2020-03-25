#pragma once

#include "ui/rp_widget.h"
#include "styles/style_info.h"
namespace Ui {
	class FlatLabel;
}
namespace QuickReply {

class TopBar : public Ui::RpWidget {
public:
	TopBar(QWidget* parent, const style::InfoTopBar& st);

	void setTitle(rpl::producer<QString>&& title);

	template <typename ButtonWidget>
	ButtonWidget* addButton(base::unique_qptr<ButtonWidget> button) {
		auto result = button.get();
		pushButton(std::move(button));
		return result;
	}

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent* e) override;

private:
	void updateControlsGeometry(int newWidth);
	Ui::RpWidget* pushButton(base::unique_qptr<Ui::RpWidget> button);
	void removeButton(not_null<Ui::RpWidget*> button);

	const style::InfoTopBar& _st;
	std::vector<base::unique_qptr<Ui::RpWidget>> _buttons;
	QPointer<Ui::FlatLabel> _title;
};

} //namespace QuickReply