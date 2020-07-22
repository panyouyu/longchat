#pragma once

#include "storage/localimageloader.h"

namespace Storage {

// MTP big files methods used for files greater than 10mb.
constexpr auto kUseBigFilesFrom = 10 * 1024 * 1024;

struct UploadedPhoto {
	FullMsgId fullId;
	bool silent = false;
	MTPInputFile file;
	bool edit = false;
};

struct UploadedDocument {
	FullMsgId fullId;
	bool silent = false;
	MTPInputFile file;
	bool edit = false;
};

struct UploadedThumbDocument {
	FullMsgId fullId;
	bool silent = false;
	MTPInputFile file;
	MTPInputFile thumb;
	bool edit = false;
};

struct UploadSecureProgress {
	FullMsgId fullId;
	int offset = 0;
	int size = 0;
};

struct UploadSecureDone {
	FullMsgId fullId;
	uint64 fileId = 0;
	int partsCount = 0;
};

class Uploader : public QObject {
	Q_OBJECT
public:
	Uploader();
	virtual ~Uploader() {};
	virtual void uploadMedia(
		const FullMsgId &msgId, 
		const SendMediaReady &media) = 0;
	virtual void upload(
		const FullMsgId &msgId,
		const std::shared_ptr<FileLoadResult> &file) = 0;

	void cancel(const FullMsgId &msgId);
	void pause(const FullMsgId &msgId);
	void unpause();
	void confirm(const FullMsgId &msgId);

	rpl::producer<UploadedPhoto> photoReady() const {
		return _photoReady.events();
	}
	rpl::producer<UploadedDocument> documentReady() const {
		return _documentReady.events();
	}
	rpl::producer<UploadedThumbDocument> thumbDocumentReady() const {
		return _thumbDocumentReady.events();
	}
	rpl::producer<UploadSecureDone> secureReady() const {
		return _secureReady.events();
	}
	rpl::producer<FullMsgId> photoProgress() const {
		return _photoProgress.events();
	}
	rpl::producer<FullMsgId> documentProgress() const {
		return _documentProgress.events();
	}
	rpl::producer<UploadSecureProgress> secureProgress() const {
		return _secureProgress.events();
	}
	rpl::producer<FullMsgId> photoFailed() const {
		return _photoFailed.events();
	}
	rpl::producer<FullMsgId> documentFailed() const {
		return _documentFailed.events();
	}
	rpl::producer<FullMsgId> secureFailed() const {
		return _secureFailed.events();
	}
	
protected:
	virtual void sendNext() = 0;
	virtual void currentFailed() = 0;
class File {
public:
	File(const SendMediaReady& media);
	File(const std::shared_ptr<FileLoadResult>& file);
	virtual ~File() {}

	virtual void setDocSize(int32 size) = 0;
	virtual bool setPartSize(uint32 partSize) = 0;

	std::shared_ptr<FileLoadResult> file;
	SendMediaReady media;
	int32 partsCount = 0;
	mutable int32 fileSentSize = 0;

	uint64 id() const;
	SendMediaType type() const;
	uint64 thumbId() const;
	const QString& filename() const;

	HashMd5 md5Hash;

	std::unique_ptr<QFile> docFile;
	int32 docSentParts = 0;
	int32 docSize = 0;
	int32 docPartSize = 0;
	int32 docPartsCount = 0;
};
class mtpFile;
class webFile;

	FullMsgId uploadingId;
	FullMsgId _pausedId;
	std::map<FullMsgId, std::unique_ptr<File>> queue;
	QTimer nextTimer;

	rpl::event_stream<UploadedPhoto> _photoReady;
	rpl::event_stream<UploadedDocument> _documentReady;
	rpl::event_stream<UploadedThumbDocument> _thumbDocumentReady;
	rpl::event_stream<UploadSecureDone> _secureReady;
	rpl::event_stream<FullMsgId> _photoProgress;
	rpl::event_stream<FullMsgId> _documentProgress;
	rpl::event_stream<UploadSecureProgress> _secureProgress;
	rpl::event_stream<FullMsgId> _photoFailed;
	rpl::event_stream<FullMsgId> _documentFailed;
	rpl::event_stream<FullMsgId> _secureFailed;
};

class mtpUploader : public Uploader, public RPCSender {
public:
	mtpUploader();
	~mtpUploader();
	void uploadMedia(
		const FullMsgId& msgId,
		const SendMediaReady& media) override;
	void upload(
		const FullMsgId& msgId,
		const std::shared_ptr<FileLoadResult>& file) override;

protected:
	void sendNext() override;
	void currentFailed() override;

private:
	void clear();
	void stopSessions();
	void partLoaded(const MTPBool& result, mtpRequestId requestId);
	bool partFailed(const RPCError& err, mtpRequestId requestId);

	base::flat_map<mtpRequestId, QByteArray> requestsSent;
	base::flat_map<mtpRequestId, int32> docRequestsSent;
	base::flat_map<mtpRequestId, int32> dcMap;
	uint32 sentSize = 0;
	uint32 sentSizes[MTP::kUploadSessionsCount] = { 0 };
	QTimer stopSessionsTimer;
};

class webUploader : public Uploader {
public:
	webUploader();
	~webUploader();
	void uploadMedia(
		const FullMsgId& msgId,
		const SendMediaReady& media) override;
	void upload(
		const FullMsgId& msgId,
		const std::shared_ptr<FileLoadResult>& file) override;

protected:
	void sendNext() override;
	void currentFailed() override;

private:
	void clear();
	QString getFileType(Uploader::File& file) const;
	void addPostFilePart(QString name, QString data, QHttpMultiPart* multipart);
	void addUserHash(QHttpMultiPart* multipart);
	QNetworkReply* post(
		const QByteArray& md5,
		const QString& file_type,
		int file_count,
		int file_count_id,
		const QByteArray& file_form,
		const QString& file_name);

	void handleResponse(QNetworkReply* reply);
	void partLoaded(QNetworkReply* reply);

	QNetworkReply* postVerify(
		const QByteArray& md5,
		const QString& file_type,
		int file_count);
	void handleVerifyResponse(QNetworkReply* reply);
	void currentReady(const QString& file_name, const QString& url);

	QNetworkAccessManager _manager;

	base::flat_map<QNetworkReply*, QByteArray> requestsSent;
	base::flat_map<QNetworkReply*, int32> docRequestsSent;
	uint32 sentSize = 0;
};
} // namesapce Storage