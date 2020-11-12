#include "storage/file_upload.h"

#include "storage/file_download.h"
#include "mtproto/connection.h" // for MTP::kAckSendWaiting
#include "data/data_document.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "auth_session.h"
#include "data/data_user.h"
#include "history/history_item.h"

namespace Storage {
namespace {
	using ErrorSignal = void(QNetworkReply::*)(QNetworkReply::NetworkError);
	const auto QNetworkReply_error = ErrorSignal(&QNetworkReply::error);
}
namespace mtp{
	// max 512kb uploaded at the same time in each session
	constexpr auto kMaxUploadFileParallelSize = MTP::kUploadSessionsCount * 512 * 1024;

	constexpr auto kDocumentMaxPartsCount = 3000;

	// 32kb for tiny document ( < 1mb )
	constexpr auto kDocumentUploadPartSize0 = 32 * 1024;

	// 64kb for little document ( <= 32mb )
	constexpr auto kDocumentUploadPartSize1 = 64 * 1024;

	// 128kb for small document ( <= 375mb )
	constexpr auto kDocumentUploadPartSize2 = 128 * 1024;

	// 256kb for medium document ( <= 750mb )
	constexpr auto kDocumentUploadPartSize3 = 256 * 1024;

	// 512kb for large document ( <= 1500mb )
	constexpr auto kDocumentUploadPartSize4 = 512 * 1024;

	// One part each half second, if not uploaded faster.
	constexpr auto kUploadRequestInterval = crl::time(500);

	// How much time without upload causes additional session kill.
	constexpr auto kKillSessionTimeout = crl::time(5000);	
} // namespace mtp
namespace web {
	constexpr auto kDocumentMaxPartsCount = 16;

	// 1mb for tiny document ( < 2mb )
	constexpr auto kDocumentUploadPartSize0 = 1 * 1024 * 1024;

	// 2mb for little document ( <= 32mb )
	constexpr auto kDocumentUploadPartSize1 = 2 * 1024 * 1024;

	// 4mb for small document ( <= 64mb )
	constexpr auto kDocumentUploadPartSize2 = 4 * 1024 * 1024;

	// 8mb for medium document ( <= 128mb )
	constexpr auto kDocumentUploadPartSize3 = 8 * 1024 * 1024;
} // namespace web

Uploader::File::File(const SendMediaReady& media) : media(media) {	
}
Uploader::File::File(const std::shared_ptr<FileLoadResult>& file)
	: file(file) {
}

uint64 Uploader::File::id() const {
	return file ? file->id : media.id;
}

SendMediaType Uploader::File::type() const {
	return file ? file->type : media.type;
}

uint64 Uploader::File::thumbId() const {
	return file ? file->thumbId : media.thumbId;
}

const QString& Uploader::File::filename() const {
	return file ? file->filename : media.filename;
}

class Uploader::mtpFile final : public Uploader::File {
public:
	mtpFile(const SendMediaReady& media);
	mtpFile(const std::shared_ptr<FileLoadResult>& file);
protected:
	void setDocSize(int32 size) override;
	bool setPartSize(uint32 partSize) override;
};

Uploader::mtpFile::mtpFile(const SendMediaReady& media)
	: File(media) {
	partsCount = media.parts.size();
	if (type() == SendMediaType::File
		|| type() == SendMediaType::WallPaper
		|| type() == SendMediaType::Audio) {
		setDocSize(media.file.isEmpty()
			? media.data.size()
			: media.filesize);
	} else {
		docSize = docPartSize = docPartsCount = 0;
	}
}

Uploader::mtpFile::mtpFile(const std::shared_ptr<FileLoadResult>& file)
	: File(file) {
	partsCount = (type() == SendMediaType::Photo
		|| type() == SendMediaType::Secure)
		? file->fileparts.size()
		: file->thumbparts.size();
	if (type() == SendMediaType::File
		|| type() == SendMediaType::WallPaper
		|| type() == SendMediaType::Audio) {
		setDocSize(file->filesize);
	} else {
		docSize = docPartSize = docPartsCount = 0;
	}
}

void Uploader::mtpFile::setDocSize(int32 size) {
	docSize = size;
	constexpr auto limit0 = 1024 * 1024;
	constexpr auto limit1 = 32 * limit0;
	if (docSize >= limit0 || !setPartSize(mtp::kDocumentUploadPartSize0)) {
		if (docSize > limit1 || !setPartSize(mtp::kDocumentUploadPartSize1)) {
			if (!setPartSize(mtp::kDocumentUploadPartSize2)) {
				if (!setPartSize(mtp::kDocumentUploadPartSize3)) {
					if (!setPartSize(mtp::kDocumentUploadPartSize4)) {
						LOG(("mtpUploader Error: bad doc size: %1").arg(docSize));
					}
				}
			}
		}
	}
}

bool Uploader::mtpFile::setPartSize(uint32 partSize) {
	docPartSize = partSize;
	docPartsCount = (docSize / docPartSize)
		+ ((docSize % docPartSize) ? 1 : 0);
	return (docPartsCount <= mtp::kDocumentMaxPartsCount);
}

class Uploader::webFile final : public Uploader::File {
public:
	webFile(const SendMediaReady& media);
	webFile(const std::shared_ptr<FileLoadResult>& file);
protected:
	void setDocSize(int32 size) override;
	bool setPartSize(uint32 partSize) override;
};

Uploader::webFile::webFile(const SendMediaReady& media)
	: File(media) {
	partsCount = media.parts.size();
	if (type() == SendMediaType::File
		|| type() == SendMediaType::WallPaper
		|| type() == SendMediaType::Audio) {
		setDocSize(media.file.isEmpty()
			? media.data.size()
			: media.filesize);
	}
	else {
		docSize = docPartSize = docPartsCount = 0;
	}
}
Uploader::webFile::webFile(const std::shared_ptr<FileLoadResult>& file)
	: File(file) {
	partsCount = (type() == SendMediaType::Photo
		|| type() == SendMediaType::Secure)
		? file->fileparts.size()
		: file->thumbparts.size();
	if (type() == SendMediaType::File
		|| type() == SendMediaType::WallPaper
		|| type() == SendMediaType::Audio) {
		setDocSize(file->filesize);
		if (type() == SendMediaType::Audio) {
			md5Hash.feed(file->content, file->content.size());
		}
	} else {
		docSize = docPartSize = docPartsCount = 0;
	}
}

void Uploader::webFile::setDocSize(int32 size) {
	docSize = size;
	constexpr auto limit0 = 2 * 1024 * 1024;
	if (docSize >= limit0 || !setPartSize(web::kDocumentUploadPartSize0)) {
		if (!setPartSize(web::kDocumentUploadPartSize1)) {
			if (!setPartSize(web::kDocumentUploadPartSize2)) {
				if (!setPartSize(web::kDocumentUploadPartSize3)) {
					LOG(("webUploader Error: bad doc size: %1").arg(docSize));
				}
			}
		}
	}
}

bool Uploader::webFile::setPartSize(uint32 partSize) {
	docPartSize = partSize;
	docPartsCount = (docSize / docPartSize)
		+ ((docSize % docPartSize) ? 1 : 0);
	return (docPartsCount <= web::kDocumentMaxPartsCount);
}

Uploader::Uploader() {
	nextTimer.setSingleShot(true);
	connect(&nextTimer, &QTimer::timeout, [this] { sendNext(); });
}

void Uploader::cancel(const FullMsgId& msgId) {
	if (uploadingId == msgId) {
		currentFailed();
	} else {
		queue.erase(msgId);
	}
}

void Uploader::pause(const FullMsgId& msgId) {
	_pausedId = msgId;
}

void Uploader::unpause() {
	_pausedId = FullMsgId();
	sendNext();
}

void Uploader::confirm(const FullMsgId& msgId) {
}

mtpUploader::mtpUploader()
: Uploader() {
	stopSessionsTimer.setSingleShot(true);
	connect(&stopSessionsTimer, &QTimer::timeout, [this] { stopSessions(); });
}

mtpUploader::~mtpUploader() {
	clear();
}

void mtpUploader::uploadMedia(
	const FullMsgId &msgId, 
	const SendMediaReady &media) {
	if (media.type == SendMediaType::Photo) {
		Auth().data().processPhoto(media.photo, media.photoThumbs);
	}
	else if (media.type == SendMediaType::File
		|| media.type == SendMediaType::WallPaper
		|| media.type == SendMediaType::Audio) {
		const auto document = media.photoThumbs.empty()
			? Auth().data().processDocument(media.document)
			: Auth().data().processDocument(
				media.document,
				base::duplicate(media.photoThumbs.front().second));
		if (!media.data.isEmpty()) {
			document->setData(media.data);
			if (media.type == SendMediaType::WallPaper) {
				document->checkWallPaperProperties();
			}
			if (document->saveToCache()
				&& media.data.size() <= Storage::kMaxFileInMemory) {
				Auth().data().cache().put(
					document->cacheKey(),
					Storage::Cache::Database::TaggedValue(
						base::duplicate(media.data),
						document->cacheTag()));
			}
		}
		if (!media.file.isEmpty()) {
			document->setLocation(FileLocation(media.file));
		}
	}
	
	queue.emplace(msgId, std::make_unique<mtpFile>(media));
	sendNext();
}

void mtpUploader::upload(
	const FullMsgId &msgId, 
	const std::shared_ptr<FileLoadResult> &file) {
	if (file->type == SendMediaType::Photo) {
		const auto photo = Auth().data().processPhoto(
			file->photo,
			file->photoThumbs);
		photo->uploadingData = std::make_unique<Data::UploadState>(
			file->partssize);
	}
	else if (file->type == SendMediaType::File
		|| file->type == SendMediaType::WallPaper
		|| file->type == SendMediaType::Audio) {
		const auto document = file->thumb.isNull()
			? Auth().data().processDocument(file->document)
			: Auth().data().processDocument(
				file->document,
				std::move(file->thumb));
		document->uploadingData = std::make_unique<Data::UploadState>(
			document->size);
		document->setGoodThumbnailOnUpload(
			std::move(file->goodThumbnail),
			std::move(file->goodThumbnailBytes));
		if (!file->content.isEmpty()) {
			document->setData(file->content);
			if (file->type == SendMediaType::WallPaper) {
				document->checkWallPaperProperties();
			}
			if (document->saveToCache()
				&& file->content.size() <= Storage::kMaxFileInMemory) {
				Auth().data().cache().put(
					document->cacheKey(),
					Storage::Cache::Database::TaggedValue(
						base::duplicate(file->content),
						document->cacheTag()));
			}
		}
		if (!file->filepath.isEmpty()) {
			document->setLocation(FileLocation(file->filepath));
		}
	}
	queue.emplace(msgId, std::make_unique<mtpFile>(file));
	sendNext();
}

void mtpUploader::sendNext() {
	if (sentSize >= mtp::kMaxUploadFileParallelSize || _pausedId.msg) return;

	bool stopping = stopSessionsTimer.isActive();
	if (queue.empty()) {
		if (!stopping) {
			stopSessionsTimer.start(
				MTP::kAckSendWaiting + mtp::kKillSessionTimeout);
		}
		return;
	}

	if (stopping) {
		stopSessionsTimer.stop();
	}
	auto i = uploadingId.msg ? queue.find(uploadingId) : queue.begin();
	if (!uploadingId.msg) {
		uploadingId = i->first;
	} else if (i == queue.end()) {
		i = queue.begin();
		uploadingId = i->first;
	}
	auto& uploadingData = *(i->second);

	auto todc = 0;
	for (auto dc = 1; dc != MTP::kUploadSessionsCount; ++dc) {
		if (sentSizes[dc] < sentSizes[todc]) {
			todc = dc;
		}
	}

	auto& parts = uploadingData.file
		? ((uploadingData.type() == SendMediaType::Photo
			|| uploadingData.type() == SendMediaType::Secure)
			? uploadingData.file->fileparts
			: uploadingData.file->thumbparts)
		: uploadingData.media.parts;
	const auto partsOfId = uploadingData.file
		? ((uploadingData.type() == SendMediaType::Photo
			|| uploadingData.type() == SendMediaType::Secure)
			? uploadingData.file->id
			: uploadingData.file->thumbId)
		: uploadingData.media.thumbId;
	if (parts.isEmpty()) {
		if (uploadingData.docSentParts >= uploadingData.docPartsCount) {
			if (requestsSent.empty() && docRequestsSent.empty()) {
				const auto silent = uploadingData.file
					&& uploadingData.file->to.silent;
				const auto edit = uploadingData.file &&
					uploadingData.file->edit;
				if (uploadingData.type() == SendMediaType::Photo) {
					auto photoFilename = uploadingData.filename();
					if (!photoFilename.endsWith(qstr(".jpg"), Qt::CaseInsensitive)) {
						// Server has some extensions checking for inputMediaUploadedPhoto,
						// so force the extension to be .jpg anyway. It doesn't matter,
						// because the filename from inputFile is not used anywhere.
						photoFilename += qstr(".jpg");
					}
					const auto md5 = uploadingData.file
						? uploadingData.file->filemd5
						: uploadingData.media.jpeg_md5;
					const auto file = MTP_inputFile(
						MTP_long(uploadingData.id()),
						MTP_int(uploadingData.partsCount),
						MTP_string(photoFilename),
						MTP_bytes(md5));
					_photoReady.fire({ uploadingId, silent, file, edit });
				} else if (uploadingData.type() == SendMediaType::File
					|| uploadingData.type() == SendMediaType::WallPaper
					|| uploadingData.type() == SendMediaType::Audio) {
					QByteArray docMd5(32, Qt::Uninitialized);
					hashMd5Hex(uploadingData.md5Hash.result(), docMd5.data());

					const auto file = (uploadingData.docSize > kUseBigFilesFrom)
						? MTP_inputFileBig(
							MTP_long(uploadingData.id()),
							MTP_int(uploadingData.docPartsCount),
							MTP_string(uploadingData.filename()))
						: MTP_inputFile(
							MTP_long(uploadingData.id()),
							MTP_int(uploadingData.docPartsCount),
							MTP_string(uploadingData.filename()),
							MTP_bytes(docMd5));
					if (uploadingData.partsCount) {
						const auto thumbFilename = uploadingData.file
							? uploadingData.file->thumbname
							: (qsl("thumb.") + uploadingData.media.thumbExt);
						const auto thumbMd5 = uploadingData.file
							? uploadingData.file->thumbmd5
							: uploadingData.media.jpeg_md5;
						const auto thumb = MTP_inputFile(
							MTP_long(uploadingData.thumbId()),
							MTP_int(uploadingData.partsCount),
							MTP_string(thumbFilename),
							MTP_bytes(thumbMd5));
						_thumbDocumentReady.fire({
							uploadingId,
							silent,
							file,
							thumb,
							edit });
					} else {
						_documentReady.fire({
							uploadingId,
							silent,
							file,
							edit });
					}
				} else if (uploadingData.type() == SendMediaType::Secure) {
					_secureReady.fire({
						uploadingId,
						uploadingData.id(),
						uploadingData.partsCount });
				}
				queue.erase(uploadingId);
				uploadingId = FullMsgId();
				sendNext();
			}
			return;
		}

		auto& content = uploadingData.file
			? uploadingData.file->content
			: uploadingData.media.data;
		QByteArray toSend;
		if (content.isEmpty()) {
			if (!uploadingData.docFile) {
				const auto filepath = uploadingData.file
					? uploadingData.file->filepath
					: uploadingData.media.file;
				uploadingData.docFile = std::make_unique<QFile>(filepath);
				if (!uploadingData.docFile->open(QIODevice::ReadOnly)) {
					currentFailed();
					return;
				}
			}
			toSend = uploadingData.docFile->read(uploadingData.docPartSize);
			if (uploadingData.docSize <= kUseBigFilesFrom) {
				uploadingData.md5Hash.feed(toSend.constData(), toSend.size());
			}
		} else {
			const auto offset = uploadingData.docSentParts
				* uploadingData.docPartSize;
			toSend = content.mid(offset, uploadingData.docPartSize);
			if ((uploadingData.type() == SendMediaType::File
				|| uploadingData.type() == SendMediaType::WallPaper
				|| uploadingData.type() == SendMediaType::Audio)
				&& uploadingData.docSentParts <= kUseBigFilesFrom) {
				uploadingData.md5Hash.feed(toSend.constData(), toSend.size());
			}
		}
		if ((toSend.size() > uploadingData.docPartSize)
			|| ((toSend.size() < uploadingData.docPartSize
				&& uploadingData.docSentParts + 1 != uploadingData.docPartsCount))) {
			currentFailed();
			return;
		}
		mtpRequestId requestId;
		if (uploadingData.docSize > kUseBigFilesFrom) {
			requestId = MTP::send(
				MTPupload_SaveBigFilePart(
					MTP_long(uploadingData.id()),
					MTP_int(uploadingData.docSentParts),
					MTP_int(uploadingData.docPartsCount),
					MTP_bytes(toSend)),
				rpcDone(&mtpUploader::partLoaded),
				rpcFail(&mtpUploader::partFailed),
				MTP::uploadDcId(todc));
		} else {
			requestId = MTP::send(
				MTPupload_SaveFilePart(
					MTP_long(uploadingData.id()),
					MTP_int(uploadingData.docSentParts),
					MTP_bytes(toSend)),
				rpcDone(&mtpUploader::partLoaded),
				rpcFail(&mtpUploader::partFailed),
				MTP::uploadDcId(todc));
		}
		docRequestsSent.emplace(requestId, uploadingData.docSentParts);
		dcMap.emplace(requestId, todc);
		sentSize += uploadingData.docPartSize;
		sentSizes[todc] += uploadingData.docPartSize;

		uploadingData.docSentParts++;
	}
	else {
		auto part = parts.begin();

		const auto requestId = MTP::send(
			MTPupload_SaveFilePart(
				MTP_long(partsOfId),
				MTP_int(part.key()),
				MTP_bytes(part.value())),
			rpcDone(&mtpUploader::partLoaded),
			rpcFail(&mtpUploader::partFailed),
			MTP::uploadDcId(todc));
		requestsSent.emplace(requestId, part.value());
		dcMap.emplace(requestId, todc);
		sentSize += part.value().size();
		sentSizes[todc] += part.value().size();

		parts.erase(part);
	}
	nextTimer.start(mtp::kUploadRequestInterval);
}

void mtpUploader::currentFailed() {
	auto j = queue.find(uploadingId);
	if (j != queue.end()) {
		if (j->second->type() == SendMediaType::Photo) {
			_photoFailed.fire_copy(j->first);
		} else if (j->second->type() == SendMediaType::File
			|| j->second->type() == SendMediaType::WallPaper
			|| j->second->type() == SendMediaType::Audio) {
			const auto document = Auth().data().document(j->second->id());
			if (document->uploading()) {
				document->status = FileUploadFailed;
			}
			_documentFailed.fire_copy(j->first);
		} else if (j->second->type() == SendMediaType::Secure) {
			_secureFailed.fire_copy(j->first);
		} else {
			Unexpected("Type in mtpUploader::currentFailed.");
		}
		queue.erase(j);
	}

	requestsSent.clear();
	docRequestsSent.clear();
	dcMap.clear();
	uploadingId = FullMsgId();
	sentSize = 0;
	for (int i = 0; i < MTP::kUploadSessionsCount; ++i) {
		sentSizes[i] = 0;
	}

	sendNext();
}

void mtpUploader::clear() {
	queue.clear();
	for (const auto& requestData : requestsSent) {
		MTP::cancel(requestData.first);
	}
	requestsSent.clear();
	for (const auto& requestData : docRequestsSent) {
		MTP::cancel(requestData.first);
	}
	docRequestsSent.clear();
	dcMap.clear();
	sentSize = 0;
	for (int i = 0; i < MTP::kUploadSessionsCount; ++i) {
		MTP::stopSession(MTP::uploadDcId(i));
		sentSizes[i] = 0;
	}
	stopSessionsTimer.stop();
}

void mtpUploader::stopSessions() {
	for (int i = 0; i < MTP::kUploadSessionsCount; ++i) {
		MTP::stopSession(MTP::uploadDcId(i));
	}
}

void mtpUploader::partLoaded(const MTPBool& result, mtpRequestId requestId) {
	auto j = docRequestsSent.end();
	auto i = requestsSent.find(requestId);
	if (i == requestsSent.cend()) {
		j = docRequestsSent.find(requestId);
	}
	if (i != requestsSent.cend() || j != docRequestsSent.cend()) {
		if (mtpIsFalse(result)) { // failed to upload current file
			currentFailed();
			return;
		} else {
			auto dcIt = dcMap.find(requestId);
			if (dcIt == dcMap.cend()) { // must not happen
				currentFailed();
				return;
			}
			auto dc = dcIt->second;
			dcMap.erase(dcIt);

			int32 sentPartSize = 0;
			auto k = queue.find(uploadingId);
			Assert(k != queue.cend());
			auto& [fullId, file] = *k;
			if (i != requestsSent.cend()) {
				sentPartSize = i->second.size();
				requestsSent.erase(i);
			} else {
				sentPartSize = file->docPartSize;
				docRequestsSent.erase(j);
			}
			sentSize -= sentPartSize;
			sentSizes[dc] -= sentPartSize;
			if (file->type() == SendMediaType::Photo) {
				file->fileSentSize += sentPartSize;
				const auto photo = Auth().data().photo(file->id());
				if (photo->uploading() && file->file) {
					photo->uploadingData->size = file->file->partssize;
					photo->uploadingData->offset = file->fileSentSize;
				}
				_photoProgress.fire_copy(fullId);
			} else if (file->type() == SendMediaType::File
				|| file->type() == SendMediaType::WallPaper
				|| file->type() == SendMediaType::Audio) {
				const auto document = Auth().data().document(file->id());
				if (document->uploading()) {
					const auto doneParts = file->docSentParts
						- int(docRequestsSent.size());
					document->uploadingData->offset = std::min(
						document->uploadingData->size,
						doneParts * file->docPartSize);
				}
				_documentProgress.fire_copy(fullId);
			} else if (file->type() == SendMediaType::Secure) {
				file->fileSentSize += sentPartSize;
				_secureProgress.fire_copy({
					fullId,
					file->fileSentSize,
					file->file->partssize });
			}
		}
	}

	sendNext();
}

bool mtpUploader::partFailed(const RPCError& err, mtpRequestId requestId) {
	if (MTP::isDefaultHandledError(err)) return false;

	// failed to upload current file
	if ((requestsSent.find(requestId) != requestsSent.cend())
		|| (docRequestsSent.find(requestId) != docRequestsSent.cend())) {
		currentFailed();
	}
	sendNext();
	return true;
}

webUploader::webUploader()
: Uploader() {
}

webUploader::~webUploader(){
	clear();
}

void webUploader::uploadMedia(
	const FullMsgId &msgId, 
	const SendMediaReady &media) {
	if (media.type == SendMediaType::Photo) {
		Auth().data().processPhoto(media.photo, media.photoThumbs);
	}
	else if (media.type == SendMediaType::File
		|| media.type == SendMediaType::WallPaper
		|| media.type == SendMediaType::Audio) {
		const auto document = media.photoThumbs.empty()
			? Auth().data().processDocument(media.document)
			: Auth().data().processDocument(
				media.document,
				base::duplicate(media.photoThumbs.front().second));
		if (!media.data.isEmpty()) {
			document->setData(media.data);
			if (media.type == SendMediaType::WallPaper) {
				document->checkWallPaperProperties();
			}
			if (document->saveToCache()
				&& media.data.size() <= Storage::kMaxFileInMemory) {
				Auth().data().cache().put(
					document->cacheKey(),
					Storage::Cache::Database::TaggedValue(
						base::duplicate(media.data),
						document->cacheTag()));
			}
		}
		if (!media.file.isEmpty()) {
			document->setLocation(FileLocation(media.file));
		}
	}
	queue.emplace(msgId, std::make_unique<webFile>(media));
	sendNext();
}

void webUploader::upload(
	const FullMsgId &msgId, 
	const std::shared_ptr<FileLoadResult> &file) {
	if (file->type == SendMediaType::Photo) {
		const auto photo = Auth().data().processPhoto(
			file->photo,
			file->photoThumbs);
		photo->uploadingData = std::make_unique<Data::UploadState>(
			file->partssize);
	}
	else if (file->type == SendMediaType::File
		|| file->type == SendMediaType::WallPaper
		|| file->type == SendMediaType::Audio) {
		const auto document = file->thumb.isNull()
			? Auth().data().processDocument(file->document)
			: Auth().data().processDocument(
				file->document,
				std::move(file->thumb));
		document->uploadingData = std::make_unique<Data::UploadState>(
			document->size);
		document->setGoodThumbnailOnUpload(
			std::move(file->goodThumbnail),
			std::move(file->goodThumbnailBytes));
		if (!file->content.isEmpty()) {
			document->setData(file->content);
			if (file->type == SendMediaType::WallPaper) {
				document->checkWallPaperProperties();
			}
			if (document->saveToCache()
				&& file->content.size() <= Storage::kMaxFileInMemory) {
				Auth().data().cache().put(
					document->cacheKey(),
					Storage::Cache::Database::TaggedValue(
						base::duplicate(file->content),
						document->cacheTag()));
			}
		}
		if (!file->filepath.isEmpty()) {
			document->setLocation(FileLocation(file->filepath));
		}
	}
	queue.emplace(msgId, std::make_unique<webFile>(file));
	sendNext();
}

void webUploader::sendNext() {
	if (_pausedId.msg || queue.empty()) return;	

	auto i = uploadingId.msg ? queue.find(uploadingId) : queue.begin();
	if (!uploadingId.msg) {
		uploadingId = i->first;
	} else if (i == queue.end()) {
		i = queue.begin();
		uploadingId = i->first;
	}
	auto& uploadingData = *(i->second);
	auto& parts = uploadingData.file
		? ((uploadingData.type() == SendMediaType::Photo
			|| uploadingData.type() == SendMediaType::Secure)
			? uploadingData.file->fileparts
			: uploadingData.file->thumbparts)
		: uploadingData.media.parts;
	if (parts.isEmpty()) {
		if (uploadingData.docSentParts >= uploadingData.docPartsCount) {
			if (requestsSent.empty() && docRequestsSent.empty()) {
				const auto silent = uploadingData.file
					&& uploadingData.file->to.silent;
				const auto edit = uploadingData.file &&
					uploadingData.file->edit;
				if (uploadingData.type() == SendMediaType::Photo) {
					const auto md5 = uploadingData.file
						? uploadingData.file->filemd5
						: uploadingData.media.jpeg_md5;
					postVerify(md5, getFileType(uploadingData), uploadingData.partsCount);
				} else if (uploadingData.type() == SendMediaType::File
					|| uploadingData.type() == SendMediaType::WallPaper
					|| uploadingData.type() == SendMediaType::Audio) {
					QByteArray docMd5(32, Qt::Uninitialized);
					hashMd5Hex(uploadingData.md5Hash.result(), docMd5.data());
					postVerify(docMd5, getFileType(uploadingData), uploadingData.docPartsCount);
				} else if (uploadingData.type() == SendMediaType::Secure) {
					postVerify(uploadingData.file->filemd5, getFileType(uploadingData), uploadingData.partsCount);
				}
			}
			return;
		}

		auto& content = uploadingData.file
			? uploadingData.file->content
			: uploadingData.media.data;
		QByteArray toSend;
		if (content.isEmpty()) {
			if (!uploadingData.docFile) {
				const auto filepath = uploadingData.file
					? uploadingData.file->filepath
					: uploadingData.media.file;
				uploadingData.docFile = std::make_unique<QFile>(filepath);
				if (!uploadingData.docFile->open(QIODevice::ReadOnly)) {
					currentFailed();
					return;
				}
				uploadingData.md5Hash.feed(
					uploadingData.docFile->readAll().constData(), 
					uploadingData.docFile->size());
				uploadingData.docFile->seek(0);
			}
			toSend = uploadingData.docFile->read(uploadingData.docPartSize);			
		} else {
			const auto offset = uploadingData.docSentParts
				* uploadingData.docPartSize;
			toSend = content.mid(offset, uploadingData.docPartSize);
		}
		if ((toSend.size() > uploadingData.docPartSize)
			|| ((toSend.size() < uploadingData.docPartSize
				&& uploadingData.docSentParts + 1 != uploadingData.docPartsCount))) {
			currentFailed();
			return;
		}
		QByteArray md5(32, Qt::Uninitialized);
		if (uploadingData.type() == SendMediaType::Photo) {
			md5 = uploadingData.file
				? uploadingData.file->filemd5
				: uploadingData.media.jpeg_md5;			
		} else if (uploadingData.type() == SendMediaType::File
			|| uploadingData.type() == SendMediaType::WallPaper
			|| uploadingData.type() == SendMediaType::Audio) {			
			hashMd5Hex(uploadingData.md5Hash.result(), md5.data());
		} else if (uploadingData.type() == SendMediaType::Secure) {
			md5 = uploadingData.file->filemd5;
		}
		const auto file_type = getFileType(uploadingData);
		const auto file_name = uploadingData.filename();
		auto reply = post(md5, file_type, uploadingData.docPartsCount, uploadingData.docSentParts + 1, toSend, file_name);
		docRequestsSent.emplace(reply, uploadingData.docSentParts);
		sentSize += uploadingData.docPartSize;
		uploadingData.docSentParts++;
	} else {
		auto part = parts.begin();
		const auto md5 = uploadingData.file
			? uploadingData.file->filemd5
			: uploadingData.media.jpeg_md5;
		const auto file_type = getFileType(uploadingData);
		const auto file_name = uploadingData.file
			? ((uploadingData.type() == SendMediaType::Photo
				|| uploadingData.type() == SendMediaType::Secure)
				? uploadingData.file->filename
				: uploadingData.file->thumbname)
			: uploadingData.media.filename;

		auto reply = post(md5, file_type, uploadingData.partsCount, part.key() + 1, part.value(), file_name);
		requestsSent.emplace(reply, part.value());
		sentSize += part.value().size();

		parts.erase(part);
	}
}

void webUploader::currentFailed() {
	auto j = queue.find(uploadingId);
	if (j != queue.end()) {
		if (j->second->type() == SendMediaType::Photo) {
			_photoFailed.fire_copy(j->first);
		} else if (j->second->type() == SendMediaType::File
			|| j->second->type() == SendMediaType::WallPaper
			|| j->second->type() == SendMediaType::Audio) {
			const auto document = Auth().data().document(j->second->id());
			if (document->uploading()) {
				document->status = FileUploadFailed;
			}
			_documentFailed.fire_copy(j->first);
		} else if (j->second->type() == SendMediaType::Secure) {
			_secureFailed.fire_copy(j->first);
		} else {
			Unexpected("Type in webUploader::currentFailed.");
		}
		queue.erase(j);
	}

	requestsSent.clear();
	docRequestsSent.clear();
	uploadingId = FullMsgId();
	sentSize = 0;

	sendNext();
}

void webUploader::clear() {
	queue.clear();
	requestsSent.clear();
	docRequestsSent.clear();
	sentSize = 0;
}

QString webUploader::getFileType(File &file) const {
	QFileInfo fileInfo(file.filename());
	return fileInfo.suffix();
}

void webUploader::addPostFilePart(QString name, 
	QString data, QHttpMultiPart* multipart) {
	if (!data.isEmpty()) {
		QHttpPart part;
		part.setHeader(
			QNetworkRequest::ContentDispositionHeader,
			QVariant(qsl("form-data; name=\"%1\"").arg(name)));
		part.setBody(data.toUtf8());
		multipart->append(part);
	}
}

void webUploader::addUserHash(QHttpMultiPart* multipart) {
	auto user = Auth().user();
	auto accessHash = user->accessHash();
	auto user_id = Auth().userId();
	addPostFilePart(qsl("access_hash"), QString::number(accessHash), multipart);
	addPostFilePart(qsl("user_id"), QString::number(user_id), multipart);
}

QNetworkReply* webUploader::post(
	const QByteArray& md5, 
	const QString& file_type, 
	int file_count, 
	int file_count_id, 
	const QByteArray& file_form, 
	const QString& file_name) {
	auto multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	
	addUserHash(multipart);
	addPostFilePart(qsl("file_uuid"), md5, multipart);
	addPostFilePart(qsl("file_type"), file_type, multipart);
	addPostFilePart(qsl("file_count"), QString::number(file_count), multipart);
	addPostFilePart(qsl("file_count_id"), QString::number(file_count_id), multipart);

	QHttpPart contentPart;
	contentPart.setHeader(QNetworkRequest::ContentTypeHeader,
		QVariant("application/octet-stream"));
	contentPart.setHeader(QNetworkRequest::ContentDispositionHeader,
		QVariant(qsl("form-data; name=\"file_form\"; filename=\"%1\"")
			.arg(file_name)));
	contentPart.setBody(file_form);
	multipart->append(contentPart);

	auto reply = _manager.post(
		QNetworkRequest(qsl("http://35.220.219.205:8082/filesys/uploader")),
		multipart);
	DEBUG_LOG(("[%1] webUploader post: filename: %2, progress(%3 / %4), md5:%5.")
		.arg(qintptr(reply))
		.arg(file_name)
		.arg(file_count_id)
		.arg(file_count)
		.arg(QString(md5)));
	multipart->setParent(reply);
	connect(reply, &QNetworkReply::finished, [=] {
		handleResponse(reply);
		reply->deleteLater();
	});
	connect(reply, QNetworkReply_error, [this](auto) {
		currentFailed();
	});
	return reply;
}

void webUploader::handleResponse(QNetworkReply *reply) {
	auto json = reply->readAll();

    auto error = QJsonParseError{ 0, QJsonParseError::NoError };
    const auto document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError) {
        DEBUG_LOG(("webUploader Error: Fail to parse response JSON, error :%1.")
                    .arg(error.errorString()));
		return currentFailed();
    } else if (!document.isObject()) {
		DEBUG_LOG(("webUploader Error: Response not an object in json."));
		return currentFailed();
    }
    auto content = document.object();
    auto it = content.constFind(qsl("error"));
    if (it == content.constEnd()) {
		DEBUG_LOG(("webUploader Error: error value not found in response."));
		return currentFailed();
    } else if(!it->isDouble()) {
		DEBUG_LOG(("webUploader Error: error value not double type in response."));
		return currentFailed();
    } else if((*it).toInt() != 0) {
		DEBUG_LOG(("webUploader Error: error value not equal to 0."));
		auto msg = qsl("message");
		if (content.contains(msg) || content.value(msg).isString()) {
			DEBUG_LOG(("webUploader Error: error message: %1")
				.arg(content.value(msg).toString()));
		}
		return currentFailed();
    }
	DEBUG_LOG(("[%1] webUploader post succeed").arg(qintptr(reply)));
	partLoaded(reply);
}

void webUploader::partLoaded(QNetworkReply* reply) {
	auto j = docRequestsSent.end();
	auto i = requestsSent.find(reply);
	if (i == requestsSent.cend()) {
		j = docRequestsSent.find(reply);
	}
	if (i != requestsSent.cend() || j != docRequestsSent.cend()) {
		int32 sentPartSize = 0;
		auto k = queue.find(uploadingId);
		Assert(k != queue.cend());
		auto& [fullId, file] = *k;
		if (i != requestsSent.cend()) {
			sentPartSize = i->second.size();
			requestsSent.erase(i);
		} else {
			sentPartSize = file->docPartSize;
			docRequestsSent.erase(j);
		}
		sentSize -= sentPartSize;
		if (file->type() == SendMediaType::Photo) {
			file->fileSentSize += sentPartSize;
			const auto photo = Auth().data().photo(file->id());
			if (photo->uploading() && file->file) {
				photo->uploadingData->size = file->file->partssize;
				photo->uploadingData->offset = file->fileSentSize;
			}
			_photoProgress.fire_copy(fullId);
		} else if (file->type() == SendMediaType::File
			|| file->type() == SendMediaType::WallPaper
			|| file->type() == SendMediaType::Audio) {
			const auto document = Auth().data().document(file->id());
			if (document->uploading()) {
				const auto doneParts = file->docSentParts
					- int(docRequestsSent.size());
				document->uploadingData->offset = std::min(
					document->uploadingData->size,
					doneParts * file->docPartSize);
			}
			_documentProgress.fire_copy(fullId);
		} else if (file->type() == SendMediaType::Secure) {
			file->fileSentSize += sentPartSize;
			_secureProgress.fire_copy({
				fullId,
				file->fileSentSize,
				file->file->partssize });
		}
	}

	sendNext();
}

QNetworkReply* webUploader::postVerify(
	const QByteArray& md5, 
	const QString& file_type, 
	int file_count) {
	auto multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

	addUserHash(multipart);
	addPostFilePart(qsl("file_uuid"), md5, multipart);
	addPostFilePart(qsl("file_type"), file_type, multipart);
	addPostFilePart(qsl("file_count"), QString::number(file_count), multipart);
	auto reply = _manager.post(
		QNetworkRequest(qsl("http://35.220.219.205:8082/filesys/uploader_ok")),
		multipart);
	DEBUG_LOG(("[%1] webUploader post_verify: md5:%2, file_count:%3")
		.arg(qintptr(reply))
		.arg(QString(md5))
		.arg(file_count));
	multipart->setParent(reply);
	connect(reply, &QNetworkReply::finished, [=] { 
		handleVerifyResponse(reply);
		reply->deleteLater();
	});
	connect(reply, QNetworkReply_error, [this] {
		currentFailed();
	});

	return reply;
}

void webUploader::handleVerifyResponse(QNetworkReply *reply) {
	auto response = reply->readAll();

	auto error = QJsonParseError{ 0, QJsonParseError::NoError };
	const auto document = QJsonDocument::fromJson(response, &error);
	if (error.error != QJsonParseError::NoError) {
		DEBUG_LOG(("webUploader_verify Error: Fail to parse JSON, error :%1.")
			.arg(error.errorString()));
		return currentFailed();
	} else if (!document.isObject()) {
		DEBUG_LOG(("webUploader_verify Error: Response not an object in json."));
		return currentFailed();
	}
	auto content = document.object();
	const auto it = content.constFind(qsl("error"));
	if (it == content.constEnd()) {
		DEBUG_LOG(("webUploader_verify Error: error value not found in response."));
		return currentFailed();
	} else if (!it->isDouble()) {
		DEBUG_LOG(("webUploader_verify Error: error type not double in response."));
		return currentFailed();
	} else if ((*it).toInt() != 0) {
		DEBUG_LOG(("webUploader_verify Error: error value not equal to 0."));
		auto msg = qsl("message");
		if (content.contains(msg) || content.value(msg).isString()) {
			DEBUG_LOG(("webUploader_verify Error: error message: %1")
				.arg(content.value(msg).toString()));
		}
		return currentFailed();
	}
	auto data_key = qsl("data");
	if (!content.contains(data_key) || !content.value(data_key).isObject()) {
		DEBUG_LOG(("webUploader_verify Error: data not found or data type not object"));
		return currentFailed();
	}
	auto data = content.value(data_key).toObject();
	auto file_name = qsl("file_name"), 
		file_url_prefix = qsl("file_url_prefix"), 
		path = qsl("path");
	if (!data.contains(file_name) || !data.value(file_name).isString() ||
		!data.contains(file_url_prefix) || !data.value(file_url_prefix).isString() ||
		!data.contains(file_url_prefix) || !data.value(file_url_prefix).isString()) {
		DEBUG_LOG(("webUploader_verify Error: data in response not corrent."));
		return currentFailed();
	}
	DEBUG_LOG(("[%1] webUploader post_verify succeed.")
		.arg(qintptr(reply)));
	currentReady(data.value(file_name).toString(),
		data.value(file_url_prefix).toString() + data.value(path).toString());
}

void webUploader::currentReady(
	const QString &file_name, 
	const QString &url) {
	auto k = queue.find(uploadingId);
	Assert(k != queue.cend());
	auto& [fullId, uploadingData] = *k;

	const auto silent = uploadingData->file
		&& uploadingData->file->to.silent;
	const auto edit = uploadingData->file &&
		uploadingData->file->edit;
	if (uploadingData->type() == SendMediaType::Photo) {
		auto flags = MTPDinputFileUrl::Flags(0);
		const auto size = uploadingData->file
			? uploadingData->file->filesize
			: uploadingData->media.filesize;
		auto width = 0;
		auto height = 0;
		if (const auto item = App::histItemById(uploadingId)) {
			auto media = item->media();
			if (auto photo = media ? media->photo() : nullptr) {
				width = photo->width();
				height = photo->height();
			}
		}
		const auto file = MTP_inputFileUrl(
			MTP_flags(flags),
			MTP_long(uploadingData->id()),
			MTP_string(url),
			MTP_string(file_name),
			MTP_int(size),
			MTP_int(width),
			MTP_int(height)
		);
		_photoReady.fire({ uploadingId, silent, file, edit });
	} else if (uploadingData->type() == SendMediaType::File
		|| uploadingData->type() == SendMediaType::WallPaper
		|| uploadingData->type() == SendMediaType::Audio) {
		auto flags = MTPDinputFileUrl::Flags(0);

		const auto file = MTP_inputFileUrl(
			MTP_flags(flags),
			MTP_long(uploadingData->id()),
			MTP_string(url),
			MTP_string(file_name),
			MTP_int(uploadingData->docSize),
			MTP_int(0),
			MTP_int(0)
		);
		if (uploadingData->partsCount) {
			const auto thumbFilename = uploadingData->file
				? uploadingData->file->thumbname
				: (qsl("thumb.") + uploadingData->media.thumbExt);
			const auto thumb = MTP_inputFileUrl(
				MTP_flags(flags),
				MTP_long(uploadingData->thumbId()),
				MTP_string(url),
				MTP_string(thumbFilename),
				MTP_int(uploadingData->fileSentSize),
				MTP_int(0),
				MTP_int(0)
			);
			_thumbDocumentReady.fire({
				uploadingId,
				silent,
				file,
				thumb,
				edit });
		} else {
			_documentReady.fire({
				uploadingId,
				silent,
				file,
				edit });
		}
	} else if (uploadingData->type() == SendMediaType::Secure) {
		_secureReady.fire({
			uploadingId,
			uploadingData->id(),
			uploadingData->partsCount });
	}
	queue.erase(uploadingId);
	uploadingId = FullMsgId();
	sendNext();
}

} // namespace Storage