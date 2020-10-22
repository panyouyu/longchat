#include "boxes/qrcode_box.h"

#include "qclipboard.h"

#include "lang/lang_keys.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "data/data_channel.h"
#include "auth_session.h"
#include "ui/toast/toast.h"
#include "qr/qr_generate.h"
#include "ui/effects/radial_animation.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"

namespace {

[[nodiscard]] QImage PeerImage(not_null<PeerData*> peer) {
	const auto size = QSize(st::qrCodeCenterSize, st::qrCodeCenterSize);
	auto result = QImage(
		size,
		QImage::Format_ARGB32_Premultiplied);
	result.fill(Qt::transparent);
	result.setDevicePixelRatio(cRetinaFactor());
	{
		auto p = Painter(&result);
		auto hq = PainterHighQualityEnabler(p);
		p.setBrush(st::activeButtonBg);
		p.setPen(Qt::NoPen);
		peer->paintUserpicRounded(p, 0, 0, st::qrCodeCenterSize);
	}
	return result;
}

[[nodiscard]] QImage QrExact(const Qr::Data& data, int pixel) {
	return Qr::Generate(data, pixel, st::windowFg->c);
}

[[nodiscard]] QImage LongChatQr(const Qr::Data& data, int pixel, int max = 0) {
	Expects(data.size > 0);

	if (max > 0 && data.size * pixel > max) {
		pixel = std::max(max / data.size, 1);
	}
	const auto qr = QrExact(data, pixel);
	auto result = QImage(qr.size(), QImage::Format_ARGB32_Premultiplied);
	result.fill(st::windowBg->c);
	{
		auto p = QPainter(&result);
		p.drawImage(QRect(QPoint(), qr.size()), qr);
	}
	return result;
}

[[nodiscard]] not_null<Ui::RpWidget*> PrepareQrWidget(
	not_null<QWidget*> parent,
	not_null<PeerData*> peer,
	rpl::producer<QByteArray> codes) {
	struct State {
		explicit State(Fn<void()> callback)
			: waiting(callback, st::defaultInfiniteRadialAnimation) {
		}

		QImage previous;
		QImage qr;
		QImage center;
		Ui::Animations::Simple shown;
		Ui::InfiniteRadialAnimation waiting;
	};
	auto qrs = std::move(
		codes
	) | rpl::map([](const QByteArray& code) {
		return Qr::Encode(code, Qr::Redundancy::Quartile);
	});
	auto palettes = rpl::single(
		rpl::empty_value()
	) | rpl::then(
		style::PaletteChanged()
	);
	auto result = Ui::CreateChild<Ui::RpWidget>(parent.get());
	const auto state = result->lifetime().make_state<State>(
		[=] { result->update(); });
	state->waiting.start();
	result->resize(st::qrCodeMaxSize, st::qrCodeMaxSize);
	rpl::combine(
		std::move(qrs),
		rpl::duplicate(palettes)
	) | rpl::map([](const Qr::Data& code, const auto&) {
		return LongChatQr(code, st::qrCodeSize, st::qrCodeMaxSize);
	}) | rpl::start_with_next([=](QImage&& image) {
		state->previous = std::move(state->qr);
		state->qr = std::move(image);
		state->waiting.stop();
		state->shown.stop();
		state->shown.start(
			[=] { result->update(); },
			0.,
			1.,
			st::fadeWrapDuration);
		}, result->lifetime());
	std::move(
		palettes
	) | rpl::map([=] {
		return PeerImage(peer);
	}) | rpl::start_with_next([=](QImage&& image) {
		state->center = std::move(image);
		}, result->lifetime());
	result->paintRequest(
	) | rpl::start_with_next([=](QRect clip) {
		auto p = QPainter(result);
		const auto shown = state->qr.isNull() ? 0. : state->shown.value(1.);
		if (!state->qr.isNull()) {
			const auto size = state->qr.size();
			const auto qr = QRect(
				(result->width() - size.width()) / 2,
				(result->height() - size.height()) / 2,
				size.width(),
				size.height());
			if (shown == 1.) {
				state->previous = QImage();
			}
			else if (!state->previous.isNull()) {
				p.drawImage(qr, state->previous);
			}
			p.setOpacity(shown);
			p.drawImage(qr, state->qr);
			p.setOpacity(1.);
		}
		const auto rect = QRect(
			(result->width() - st::qrCodeCenterSize) / 2,
			(result->height() - st::qrCodeCenterSize) / 2,
			st::qrCodeCenterSize,
			st::qrCodeCenterSize);
		p.drawImage(rect, state->center);
		if (!anim::Disabled() && state->waiting.animating()) {
			auto hq = PainterHighQualityEnabler(p);
			const auto line = st::radialLine;
			const auto radial = state->waiting.computeState();
			auto pen = st::activeButtonBg->p;
			pen.setWidth(line);
			pen.setCapStyle(Qt::RoundCap);
			p.setOpacity(radial.shown * (1. - shown));
			p.setPen(pen);
			p.drawArc(
				rect.marginsAdded({ line, line, line, line }),
				radial.arcFrom,
				radial.arcLength);
			p.setOpacity(1.);
		}
	}, result->lifetime());
	return result;
}
}

QRCodeBox::QRCodeBox(QWidget*, not_null<PeerData*> peer)
	: BoxContent()
	, _peer(peer) {
	boxClosing(
	) | rpl::start_with_next([=] {
		if (_requestId) {
			MTP::cancel(_requestId);
		}
	}, lifetime());
}

void QRCodeBox::prepare() {
	setTitle(_peer->isSelf() 
		? langFactory(lng_setting_self_qrcode)
		: langFactory(lng_setting_title_qrcode));
	addButton(langFactory(lng_qrcode_copy), [this] { copy(); });
	addButton(langFactory(lng_qrcode_save_as), [this] { saveAs(); });
	addButton(langFactory(lng_qrcode_refresh), [this] { refresh(); });
	addTopButton(st::boxTitleClose, [=] {closeBox(); });

	_code = PrepareQrWidget(this, _peer, _qrCodes.changes());

	_status.value(
	) | rpl::start_with_next([=](auto status) {
		auto height = st::qrCodePicTop + st::qrCodeUserPicSize;
		height += st::qrCodePicCodeInterval + st::qrCodeMaxSize;
		if (!status.isEmpty()) {
			height += st::qrCodeStatusMargin.top() + st::qrCodeStatusFont->height + st::qrCodeStatusMargin.bottom();
		}
		_code->moveToLeft(
			(st::qrCodeBoxWidth - st::qrCodeMaxSize) >> 1,
			st::qrCodePicTop + st::qrCodeUserPicSize + st::qrCodePicCodeInterval);
		setDimensions(st::qrCodeBoxWidth, height);
	}, lifetime());
	
	init();
}

void QRCodeBox::paintEvent(QPaintEvent* e) {
	BoxContent::paintEvent(e);

	Painter p(this);
	auto left = st::qrCodeLeft;
	auto top = st::qrCodePicTop;
	// user pic
	_peer->paintUserpicRounded(p, left, top, st::qrCodeUserPicSize);
	// user name
	p.setPen(st::dialogsNameFg);
	auto name = Text(st::qrCodeNameStyle, _peer->name);
	auto nameTop = top + ((st::qrCodeUserPicSize - name.minHeight()) >> 1);
	auto nameLeft = left + st::qrCodeUserPicSize + st::qrCodePicNameInterval;
	auto nameMaxWidth = width() - left - nameLeft;
	name.drawElided(p, nameLeft, nameTop, nameMaxWidth);
	// code
	top += st::qrCodeUserPicSize + st::qrCodePicCodeInterval + st::qrCodeMaxSize;
	top += st::qrCodeStatusMargin.top();
	// status
	if (!_status.current().isEmpty()) {
		Text text = Text(st::msgNameStyle, _status.current());
		text.drawElided(p, (st::qrCodeBoxWidth - st::qrCodeMaxSize) >> 1, top, st::qrCodeMaxSize, 1, style::al_center);
	}
}

void QRCodeBox::keyPressEvent(QKeyEvent* e) {
	auto m = e->modifiers();
	if ((m & Qt::CTRL) && (e->key() == Qt::Key_C)) {
		copy();
	}
}

void QRCodeBox::init() {
	if (_requestId) {
		return;
	}
	_requestId = MTP::send(MTPaccount_GetQRCode(
		MTP_flags(MTPaccount_getQRCode::Flags(0)),
		MTP_int(_peer->bareId()),
		MTP_string(qsl())),
		rpcDone(&QRCodeBox::onDone),
		rpcFail(&QRCodeBox::onFail));
}

QImage QRCodeBox::qrCodeImage() {
	if (_requestId) {
		return QImage();
	}
	return _code->grab().toImage();
}

void QRCodeBox::copy() {
	auto img = qrCodeImage();
	if (!img.isNull()) {
		const auto clipboard = QApplication::clipboard();
		clipboard->setImage(img);
		Ui::Toast::Show(lang(lng_qrcode_copy_finished));
	}
}

void QRCodeBox::saveAs() {
	auto img = qrCodeImage();
	if (img.isNull()) {
		return;
	}

	QString filename = QFileDialog::getSaveFileName(this, 
		lang(lng_qrcode_save_code), 
		lng_qrcode_code_name(lt_name, _peer->name), 
		qsl("Images (*.png *.xpm *.jpg)"));

	if (filename.length() > 0) {
		img.save(filename);
	}
}

void QRCodeBox::refresh() {
	if (_requestId) {
		return;
	}
	int type = 0;
	long hash = 0;
	_peer->input.match([&type, &hash](const MTPDinputPeerSelf &) {
		type = 1;
		hash = Auth().user()->accessHash();
	}, [&type, &hash](const MTPDinputPeerUser &user) {
		type = 2;
		hash = user.vaccess_hash.v;
	}, [&type](const MTPDinputPeerChat &chat) {
		type = 3;
	}, [&type, &hash](const MTPDinputPeerChannel &channel) {
		type = 4;
		hash = channel.vaccess_hash.v;
	}, [](const auto&) {
		Unexpected("unsupport peer type.");
	});

	_requestId = MTP::send(MTPaccount_RefreshQRCode(
		MTP_flags(MTPaccount_refreshQRCode::Flags(0)),
		MTP_int(_peer->bareId()),
		MTP_int(type),
		MTP_long(hash)),
		rpcDone(&QRCodeBox::onDone),
		rpcFail(&QRCodeBox::onFail));
}

void QRCodeBox::onDone(const MTPQRCode &data) {
	_requestId = 0;
	data.match([=](const MTPDqRCode& info) {
		Expects(!qba(info.vurl).isEmpty());
		_qrCodes = qba(info.vurl);
		auto dateTime = QDateTime::fromTime_t(info.vuntil.v);
		_status = _peer->isSelf()
			? lang(lng_qrcode_self_status)
			: lng_qrcode_chat_status(lt_date, langDayOfMonthFull(dateTime.date()), 
									 lt_time, dateTime.time().toString(cTimeFormat()));
	});
}

bool QRCodeBox::onFail(const RPCError& error) {
	if (MTP::isDefaultHandledError(error)) return false;

	_requestId = 0;
	return true;
}
