#include "datadefine.h"
#include <QFile>
#include "data/data_peer_values.h"
#include "auth_session.h"
#include "data/data_session.h"

namespace Contact {
    
    QString getAllFileContent(const QString& path)
    {
        QFile file(path);
        file.open(QFile::ReadOnly);
        QString skinInfo = file.readAll();
        file.close();
        return skinInfo;
    }

	void genContact(ContactInfo* ci, uint64 peerId, uint64 parentId)
	{
		auto time = unixtime();
		//qDebug() << user->id << peer->name << Data::OnlineText(user, time);
		PeerData* peer = Auth().data().peerLoaded(peerId);
		UserData* user = peer->asUser();
		ci->id = user->id;
		ci->firstName = peer->name;
		ci->lastName = qsl("");
		if (auto userpic = peer->currentUserpic()) {
			ci->hasAvatar = true;
		}
		ci->peerId = peerId;
		ci->parentId = parentId;
		ci->online = Data::OnlineTextActive(user, time);
		ci->lastLoginTime = Data::OnlineText(user, time);
	}

}
