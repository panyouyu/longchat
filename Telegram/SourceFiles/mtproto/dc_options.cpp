/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "mtproto/dc_options.h"

#include "storage/serialize_common.h"

namespace MTP {
namespace {

const char *(PublicRSAKeys[]) = { "\
-----BEGIN RSA PUBLIC KEY-----\n\
MIIBCgKCAQEAvKLEOWTzt9Hn3/9Kdp/RdHcEhzmd8xXeLSpHIIzaXTLJDw8BhJy1\n\
jR/iqeG8Je5yrtVabqMSkA6ltIpgylH///FojMsX1BHu4EPYOXQgB0qOi6kr08iX\n\
ZIH9/iOPQOWDsL+Lt8gDG0xBy+sPe/2ZHdzKMjX6O9B4sOsxjFrk5qDoWDrioJor\n\
AJ7eFAfPpOBf2w73ohXudSrJE0lbQ8pCWNpMY8cB9i8r+WBitcvouLDAvmtnTX7a\n\
khoDzmKgpJBYliAY4qA73v7u5UIepE8QgV0jCOhxJCPubP8dg+/PlLLVKyxU5Cdi\n\
QtZj2EMy4s9xlNKzX8XezE0MHEa6bQpnFwIDAQAB\n\
-----END RSA PUBLIC KEY-----", "\
-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvKLEOWTzt9Hn3/9Kdp/R\n\
dHcEhzmd8xXeLSpHIIzaXTLJDw8BhJy1jR/iqeG8Je5yrtVabqMSkA6ltIpgylH/\n\
//FojMsX1BHu4EPYOXQgB0qOi6kr08iXZIH9/iOPQOWDsL+Lt8gDG0xBy+sPe/2Z\n\
HdzKMjX6O9B4sOsxjFrk5qDoWDrioJorAJ7eFAfPpOBf2w73ohXudSrJE0lbQ8pC\n\
WNpMY8cB9i8r+WBitcvouLDAvmtnTX7akhoDzmKgpJBYliAY4qA73v7u5UIepE8Q\n\
gV0jCOhxJCPubP8dg+/PlLLVKyxU5CdiQtZj2EMy4s9xlNKzX8XezE0MHEa6bQpn\n\
FwIDAQAB\n\
-----END PUBLIC KEY-----", "\
-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvKLEOWTzt9Hn3/9Kdp/R\n\
dHcEhzmd8xXeLSpHIIzaXTLJDw8BhJy1jR/iqeG8Je5yrtVabqMSkA6ltIpgylH/\n\
//FojMsX1BHu4EPYOXQgB0qOi6kr08iXZIH9/iOPQOWDsL+Lt8gDG0xBy+sPe/2Z\n\
HdzKMjX6O9B4sOsxjFrk5qDoWDrioJorAJ7eFAfPpOBf2w73ohXudSrJE0lbQ8pC\n\
WNpMY8cB9i8r+WBitcvouLDAvmtnTX7akhoDzmKgpJBYliAY4qA73v7u5UIepE8Q\n\
gV0jCOhxJCPubP8dg+/PlLLVKyxU5CdiQtZj2EMy4s9xlNKzX8XezE0MHEa6bQpn\n\
FwIDAQAB\n\
-----END PUBLIC KEY-----", "\
-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvKLEOWTzt9Hn3/9Kdp/R\n\
dHcEhzmd8xXeLSpHIIzaXTLJDw8BhJy1jR/iqeG8Je5yrtVabqMSkA6ltIpgylH/\n\
//FojMsX1BHu4EPYOXQgB0qOi6kr08iXZIH9/iOPQOWDsL+Lt8gDG0xBy+sPe/2Z\n\
HdzKMjX6O9B4sOsxjFrk5qDoWDrioJorAJ7eFAfPpOBf2w73ohXudSrJE0lbQ8pC\n\
WNpMY8cB9i8r+WBitcvouLDAvmtnTX7akhoDzmKgpJBYliAY4qA73v7u5UIepE8Q\n\
gV0jCOhxJCPubP8dg+/PlLLVKyxU5CdiQtZj2EMy4s9xlNKzX8XezE0MHEa6bQpn\n\
FwIDAQAB\n\
-----END PUBLIC KEY-----", "\
-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvKLEOWTzt9Hn3/9Kdp/R\n\
dHcEhzmd8xXeLSpHIIzaXTLJDw8BhJy1jR/iqeG8Je5yrtVabqMSkA6ltIpgylH/\n\
//FojMsX1BHu4EPYOXQgB0qOi6kr08iXZIH9/iOPQOWDsL+Lt8gDG0xBy+sPe/2Z\n\
HdzKMjX6O9B4sOsxjFrk5qDoWDrioJorAJ7eFAfPpOBf2w73ohXudSrJE0lbQ8pC\n\
WNpMY8cB9i8r+WBitcvouLDAvmtnTX7akhoDzmKgpJBYliAY4qA73v7u5UIepE8Q\n\
gV0jCOhxJCPubP8dg+/PlLLVKyxU5CdiQtZj2EMy4s9xlNKzX8XezE0MHEa6bQpn\n\
FwIDAQAB\n\
-----END PUBLIC KEY-----" };

} // namespace

class DcOptions::WriteLocker {
public:
	WriteLocker(not_null<DcOptions*> that)
	: _that(that)
	, _lock(&_that->_useThroughLockers) {
	}
	~WriteLocker() {
		_that->computeCdnDcIds();
	}

private:
	not_null<DcOptions*> _that;
	QWriteLocker _lock;

};

class DcOptions::ReadLocker {
public:
	ReadLocker(not_null<const DcOptions*> that)
	: _lock(&that->_useThroughLockers) {
	}

private:
	QReadLocker _lock;

};

void DcOptions::readBuiltInPublicKeys() {
	for (const auto key : PublicRSAKeys) {
		const auto keyBytes = bytes::make_span(key, strlen(key));
		auto parsed = internal::RSAPublicKey(keyBytes);
		if (parsed.isValid()) {
			_publicKeys.emplace(parsed.getFingerPrint(), std::move(parsed));
		} else {
			LOG(("MTP Error: could not read this public RSA key:"));
			LOG((key));
		}
	}
}

void DcOptions::constructFromBuiltIn() {
	WriteLocker lock(this);
	_data.clear();

	readBuiltInPublicKeys();

	auto bdcs = builtInDcs();
	for (auto i = 0, l = builtInDcsCount(); i != l; ++i) {
		const auto flags = Flag::f_static | 0;
		const auto bdc = bdcs[i];
		applyOneGuarded(bdc.id, flags, bdc.ip, bdc.port, {});
		DEBUG_LOG(("MTP Info: adding built in DC %1 connect option: "
			"%2:%3").arg(bdc.id).arg(bdc.ip).arg(bdc.port));
	}

	auto bdcsipv6 = builtInDcsIPv6();
	for (auto i = 0, l = builtInDcsCountIPv6(); i != l; ++i) {
		const auto flags = Flag::f_static | Flag::f_ipv6;
		const auto bdc = bdcsipv6[i];
		applyOneGuarded(bdc.id, flags, bdc.ip, bdc.port, {});
		DEBUG_LOG(("MTP Info: adding built in DC %1 IPv6 connect option: "
			"%2:%3").arg(bdc.id).arg(bdc.ip).arg(bdc.port));
	}
}

void DcOptions::processFromList(
		const QVector<MTPDcOption> &options,
		bool overwrite) {
	if (options.empty() || _immutable) {
		return;
	}

	auto data = [&] {
		if (overwrite) {
			return std::map<DcId, std::vector<Endpoint>>();
		}
		ReadLocker lock(this);
		return _data;
	}();
	for (auto &mtpOption : options) {
		if (mtpOption.type() != mtpc_dcOption) {
			LOG(("Wrong type in DcOptions: %1").arg(mtpOption.type()));
			continue;
		}

		auto &option = mtpOption.c_dcOption();
		auto dcId = option.vid.v;
		auto flags = option.vflags.v;
		auto ip = std::string(
			option.vip_address.v.constData(),
			option.vip_address.v.size());
		auto port = option.vport.v;
		auto secret = option.has_secret()
			? bytes::make_vector(option.vsecret.v)
			: bytes::vector();
		ApplyOneOption(data, dcId, flags, ip, port, secret);
	}

	auto difference = [&] {
		WriteLocker lock(this);
		auto result = CountOptionsDifference(_data, data);
		if (!result.empty()) {
			_data = std::move(data);
		}
		return result;
	}();
	if (!difference.empty()) {
		_changed.notify(std::move(difference));
	}
}

void DcOptions::setFromList(const MTPVector<MTPDcOption> &options) {
	processFromList(options.v, true);
}

void DcOptions::addFromList(const MTPVector<MTPDcOption> &options) {
	processFromList(options.v, false);
}

void DcOptions::addFromOther(DcOptions &&options) {
	if (this == &options || _immutable) {
		return;
	}

	auto idsChanged = std::vector<DcId>();
	{
		ReadLocker lock(&options);
		if (options._data.empty()) {
			return;
		}

		idsChanged.reserve(options._data.size());
		{
			WriteLocker lock(this);
			const auto changed = [&](const std::vector<Endpoint> &list) {
				auto result = false;
				for (const auto &endpoint : list) {
					const auto dcId = endpoint.id;
					const auto flags = endpoint.flags;
					const auto &ip = endpoint.ip;
					const auto port = endpoint.port;
					const auto &secret = endpoint.secret;
					if (applyOneGuarded(dcId, flags, ip, port, secret)) {
						result = true;
					}
				}
				return result;
			};
			for (const auto &item : base::take(options._data)) {
				if (changed(item.second)) {
					idsChanged.push_back(item.first);
				}
			}
			for (auto &item : options._cdnPublicKeys) {
				for (auto &entry : item.second) {
					_cdnPublicKeys[item.first].insert(std::move(entry));
				}
			}
		}
	}

	if (!idsChanged.empty()) {
		_changed.notify(std::move(idsChanged));
	}
}

void DcOptions::constructAddOne(
		int id,
		Flags flags,
		const std::string &ip,
		int port,
		const bytes::vector &secret) {
	WriteLocker lock(this);
	applyOneGuarded(BareDcId(id), flags, ip, port, secret);
}

bool DcOptions::applyOneGuarded(
		DcId dcId,
		Flags flags,
		const std::string &ip,
		int port,
		const bytes::vector &secret) {
	return ApplyOneOption(_data, dcId, flags, ip, port, secret);
}

bool DcOptions::ApplyOneOption(
		std::map<DcId, std::vector<Endpoint>> &data,
		DcId dcId,
		Flags flags,
		const std::string &ip,
		int port,
		const bytes::vector &secret) {
	auto i = data.find(dcId);
	if (i != data.cend()) {
		for (auto &endpoint : i->second) {
			if (endpoint.ip == ip && endpoint.port == port) {
				return false;
			}
		}
		i->second.push_back(Endpoint(dcId, flags, ip, port, secret));
	} else {
		data.emplace(dcId, std::vector<Endpoint>(
			1,
			Endpoint(dcId, flags, ip, port, secret)));
	}
	return true;
}

auto DcOptions::CountOptionsDifference(
		const std::map<DcId, std::vector<Endpoint>> &a,
		const std::map<DcId, std::vector<Endpoint>> &b) -> Ids {
	auto result = Ids();
	const auto find = [](
			const std::vector<Endpoint> &where,
			const Endpoint &what) {
		for (const auto &endpoint : where) {
			if (endpoint.ip == what.ip && endpoint.port == what.port) {
				return true;
			}
		}
		return false;
	};
	const auto equal = [&](
			const std::vector<Endpoint> &m,
			const std::vector<Endpoint> &n) {
		if (m.size() != n.size()) {
			return false;
		}
		for (const auto &endpoint : m) {
			if (!find(n, endpoint)) {
				return false;
			}
		}
		return true;
	};

	auto i = begin(a);
	auto j = begin(b);
	const auto max = std::numeric_limits<DcId>::max();
	while (i != end(a) || j != end(b)) {
		const auto aId = (i == end(a)) ? max : i->first;
		const auto bId = (j == end(b)) ? max : j->first;
		if (aId < bId) {
			result.push_back(aId);
			++i;
		} else if (bId < aId) {
			result.push_back(bId);
			++j;
		} else {
			if (!equal(i->second, j->second)) {
				result.push_back(aId);
			}
			++i;
			++j;
		}
	}
	return result;
}

QByteArray DcOptions::serialize() const {
	if (_immutable) {
		// Don't write the overriden options to our settings.
		return DcOptions().serialize();
	}

	ReadLocker lock(this);

	auto size = sizeof(qint32);

	// Dc options.
	auto optionsCount = 0;
	size += sizeof(qint32);
	for (const auto &item : _data) {
		if (isTemporaryDcId(item.first)) {
			continue;
		}
		for (const auto &endpoint : item.second) {
			++optionsCount;
			// id + flags + port
			size += sizeof(qint32) + sizeof(qint32) + sizeof(qint32);
			size += sizeof(qint32) + endpoint.ip.size();
			size += sizeof(qint32) + endpoint.secret.size();
		}
	}

	// CDN public keys.
	auto count = 0;
	for (auto &keysInDc : _cdnPublicKeys) {
		count += keysInDc.second.size();
	}
	struct SerializedPublicKey {
		DcId dcId;
		bytes::vector n;
		bytes::vector e;
	};
	std::vector<SerializedPublicKey> publicKeys;
	publicKeys.reserve(count);
	size += sizeof(qint32);
	for (const auto &keysInDc : _cdnPublicKeys) {
		for (const auto &entry : keysInDc.second) {
			publicKeys.push_back({
				keysInDc.first,
				entry.second.getN(),
				entry.second.getE()
			});
			size += sizeof(qint32)
				+ Serialize::bytesSize(publicKeys.back().n)
				+ Serialize::bytesSize(publicKeys.back().e);
		}
	}

	constexpr auto kVersion = 1;

	auto result = QByteArray();
	result.reserve(size);
	{
		QDataStream stream(&result, QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << qint32(-kVersion);

		// Dc options.
		stream << qint32(optionsCount);
		for (const auto &item : _data) {
			if (isTemporaryDcId(item.first)) {
				continue;
			}
			for (const auto &endpoint : item.second) {
				stream << qint32(endpoint.id)
					<< qint32(endpoint.flags)
					<< qint32(endpoint.port)
					<< qint32(endpoint.ip.size());
				stream.writeRawData(endpoint.ip.data(), endpoint.ip.size());
				stream << qint32(endpoint.secret.size());
				stream.writeRawData(
					reinterpret_cast<const char*>(endpoint.secret.data()),
					endpoint.secret.size());
			}
		}

		// CDN public keys.
		stream << qint32(publicKeys.size());
		for (auto &key : publicKeys) {
			stream << qint32(key.dcId)
				<< Serialize::bytes(key.n)
				<< Serialize::bytes(key.e);
		}
	}
	return result;
}

void DcOptions::constructFromSerialized(const QByteArray &serialized) {
	QDataStream stream(serialized);
	stream.setVersion(QDataStream::Qt_5_1);

	auto minusVersion = qint32(0);
	stream >> minusVersion;
	const auto version = (minusVersion < 0) ? (-minusVersion) : 0;

	auto count = qint32(0);
	if (version > 0) {
		stream >> count;
	} else {
		count = minusVersion;
	}
	if (stream.status() != QDataStream::Ok) {
		LOG(("MTP Error: Bad data for DcOptions::constructFromSerialized()"));
		return;
	}

	WriteLocker lock(this);
	_data.clear();
	for (auto i = 0; i != count; ++i) {
		qint32 id = 0, flags = 0, port = 0, ipSize = 0;
		stream >> id >> flags >> port >> ipSize;

		// https://stackoverflow.com/questions/1076714/max-length-for-client-ip-address
		constexpr auto kMaxIpSize = 45;
		if (ipSize <= 0 || ipSize > kMaxIpSize) {
			LOG(("MTP Error: Bad data inside DcOptions::constructFromSerialized()"));
			return;
		}

		auto ip = std::string(ipSize, ' ');
		stream.readRawData(ip.data(), ipSize);

		constexpr auto kMaxSecretSize = 32;
		auto secret = bytes::vector();
		if (version > 0) {
			auto secretSize = qint32(0);
			stream >> secretSize;
			if (secretSize < 0 || secretSize > kMaxSecretSize) {
				LOG(("MTP Error: Bad data inside DcOptions::constructFromSerialized()"));
				return;
			} else if (secretSize > 0) {
				secret.resize(secretSize);
				stream.readRawData(
					reinterpret_cast<char*>(secret.data()),
					secretSize);
			}
		}

		if (stream.status() != QDataStream::Ok) {
			LOG(("MTP Error: Bad data inside DcOptions::constructFromSerialized()"));
			return;
		}

		applyOneGuarded(
			DcId(id),
			Flags::from_raw(flags),
			ip,
			port,
			secret);
	}

	// Read CDN config
	if (!stream.atEnd()) {
		auto count = qint32(0);
		stream >> count;
		if (stream.status() != QDataStream::Ok) {
			LOG(("MTP Error: Bad data for CDN config in DcOptions::constructFromSerialized()"));
			return;
		}

		for (auto i = 0; i != count; ++i) {
			qint32 dcId = 0;
			bytes::vector n, e;
			stream >> dcId >> Serialize::bytes(n) >> Serialize::bytes(e);
			if (stream.status() != QDataStream::Ok) {
				LOG(("MTP Error: Bad data for CDN config inside DcOptions::constructFromSerialized()"));
				return;
			}

			auto key = internal::RSAPublicKey(n, e);
			if (key.isValid()) {
				_cdnPublicKeys[dcId].emplace(key.getFingerPrint(), std::move(key));
			} else {
				LOG(("MTP Error: Could not read valid CDN public key."));
			}
		}
	}
}

DcOptions::Ids DcOptions::configEnumDcIds() const {
	auto result = Ids();
	{
		ReadLocker lock(this);
		result.reserve(_data.size());
		for (auto &item : _data) {
			const auto dcId = item.first;
			Assert(!item.second.empty());
			if (!isCdnDc(item.second.front().flags)
				&& !isTemporaryDcId(dcId)) {
				result.push_back(dcId);
			}
		}
	}
	ranges::sort(result);
	return result;
}

DcType DcOptions::dcType(ShiftedDcId shiftedDcId) const {
	if (isTemporaryDcId(shiftedDcId)) {
		return DcType::Temporary;
	}
	ReadLocker lock(this);
	if (_cdnDcIds.find(BareDcId(shiftedDcId)) != _cdnDcIds.cend()) {
		return DcType::Cdn;
	}
	if (isDownloadDcId(shiftedDcId)) {
		return DcType::MediaDownload;
	}
	return DcType::Regular;
}

void DcOptions::setCDNConfig(const MTPDcdnConfig &config) {
	WriteLocker lock(this);
	_cdnPublicKeys.clear();
	for_const (auto &publicKey, config.vpublic_keys.v) {
		Expects(publicKey.type() == mtpc_cdnPublicKey);
		const auto &keyData = publicKey.c_cdnPublicKey();
		const auto keyBytes = bytes::make_span(keyData.vpublic_key.v);
		auto key = internal::RSAPublicKey(keyBytes);
		if (key.isValid()) {
			_cdnPublicKeys[keyData.vdc_id.v].emplace(
				key.getFingerPrint(),
				std::move(key));
		} else {
			LOG(("MTP Error: could not read this public RSA key:"));
			LOG((qs(keyData.vpublic_key)));
		}
	}
}

bool DcOptions::hasCDNKeysForDc(DcId dcId) const {
	ReadLocker lock(this);
	return _cdnPublicKeys.find(dcId) != _cdnPublicKeys.cend();
}

bool DcOptions::getDcRSAKey(DcId dcId, const QVector<MTPlong> &fingerprints, internal::RSAPublicKey *result) const {
	auto findKey = [&fingerprints, &result](const std::map<uint64, internal::RSAPublicKey> &keys) {
		for_const (auto &fingerprint, fingerprints) {
			auto it = keys.find(static_cast<uint64>(fingerprint.v));
			if (it != keys.cend()) {
				*result = it->second;
				return true;
			}
		}
		return false;
	};
	{
		ReadLocker lock(this);
		auto it = _cdnPublicKeys.find(dcId);
		if (it != _cdnPublicKeys.cend()) {
			return findKey(it->second);
		}
	}
	return findKey(_publicKeys);
}

auto DcOptions::lookup(
		DcId dcId,
		DcType type,
		bool throughProxy) const -> Variants {
	using Flag = Flag;
	auto result = Variants();
	{
		ReadLocker lock(this);
		const auto i = _data.find(dcId);
		if (i == end(_data)) {
			return result;
		}
		for (const auto &endpoint : i->second) {
			const auto flags = endpoint.flags;
			if (type == DcType::Cdn && !(flags & Flag::f_cdn)) {
				continue;
			} else if (type != DcType::MediaDownload
				&& (flags & Flag::f_media_only)) {
				continue;
			}
			const auto address = (flags & Flag::f_ipv6)
				? Variants::IPv6
				: Variants::IPv4;
			result.data[address][Variants::Tcp].push_back(endpoint);
			if (!(flags & (Flag::f_tcpo_only | Flag::f_secret))) {
				result.data[address][Variants::Http].push_back(endpoint);
			}
		}
		if (type == DcType::MediaDownload) {
			FilterIfHasWithFlag(result, Flag::f_media_only);
		}
		if (throughProxy) {
			FilterIfHasWithFlag(result, Flag::f_static);
		}
	}
	return result;
}

void DcOptions::FilterIfHasWithFlag(Variants &variants, Flag flag) {
	const auto is = [&](const Endpoint &endpoint) {
		return (endpoint.flags & flag) != 0;
	};
	const auto has = [&](const std::vector<Endpoint> &list) {
		return ranges::find_if(list, is) != end(list);
	};
	for (auto &byAddress : variants.data) {
		for (auto &list : byAddress) {
			if (has(list)) {
				list = ranges::view::all(
					list
				) | ranges::view::filter(
					is
				) | ranges::to_vector;
			}
		}
	}
}

void DcOptions::computeCdnDcIds() {
	_cdnDcIds.clear();
	for (auto &item : _data) {
		Assert(!item.second.empty());
		if (item.second.front().flags & Flag::f_cdn) {
			_cdnDcIds.insert(BareDcId(item.first));
		}
	}
}

bool DcOptions::loadFromFile(const QString &path) {
	QVector<MTPDcOption> options;

	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		LOG(("MTP Error: could not read '%1'").arg(path));
		return false;
	}
	QTextStream stream(&f);
	stream.setCodec("UTF-8");
	while (!stream.atEnd()) {
		auto line = stream.readLine();
		auto components = line.split(QRegularExpression(R"(\s)"), QString::SkipEmptyParts);
		if (components.isEmpty() || components[0].startsWith('#')) {
			continue;
		}

		auto error = [line] {
			LOG(("MTP Error: in .tdesktop-endpoints expected 'dcId host port [tcpo_only] [media_only]', got '%1'").arg(line));
			return false;
		};
		if (components.size() < 3) {
			return error();
		}
		auto dcId = components[0].toInt();
		auto ip = components[1];
		auto port = components[2].toInt();
		auto host = QHostAddress();
		if (dcId <= 0 || dcId >= kDcShift || !host.setAddress(ip) || port <= 0) {
			return error();
		}
		auto flags = Flags(0);
		if (host.protocol() == QAbstractSocket::IPv6Protocol) {
			flags |= Flag::f_ipv6;
		}
		for (auto &option : components.mid(3)) {
			if (option.startsWith('#')) {
				break;
			} else if (option == qstr("tcpo_only")) {
				flags |= Flag::f_tcpo_only;
			} else if (option == qstr("media_only")) {
				flags |= Flag::f_media_only;
			} else {
				return error();
			}
		}
		options.push_back(MTP_dcOption(
			MTP_flags(flags),
			MTP_int(dcId),
			MTP_string(ip),
			MTP_int(port),
			MTPbytes()));
	}
	if (options.isEmpty()) {
		LOG(("MTP Error: in .tdesktop-endpoints expected at least one endpoint being provided."));
		return false;
	}

	_immutable = false;
	setFromList(MTP_vector<MTPDcOption>(options));
	_immutable = true;

	return true;
}

bool DcOptions::writeToFile(const QString &path) const {
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly)) {
		return false;
	}
	QTextStream stream(&f);
	stream.setCodec("UTF-8");

	ReadLocker lock(this);
	for (const auto &item : _data) {
		for (const auto &option : item.second) {
			stream
				<< option.id
				<< ' '
				<< QString::fromStdString(option.ip)
				<< ' ' << option.port;
			if (option.flags & Flag::f_tcpo_only) {
				stream << " tcpo_only";
			}
			if (option.flags & Flag::f_media_only) {
				stream << " media_only";
			}
			stream << '\n';
		}
	}
	return true;
}

} // namespace MTP
