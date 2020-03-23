#include "datadefine.h"
#include <QFile>
#include "data/data_peer_values.h"
namespace Contact {
    
    QString getAllFileContent(const QString& path)
    {
        QFile file(path);
        file.open(QFile::ReadOnly);
        QString skinInfo = file.readAll();
        file.close();
        return skinInfo;
    }

	void genContact(ContactInfo* ci, UserData* user, PeerData* peer, uint64 parentId)
	{
		auto time = unixtime();
		//qDebug() << user->id << peer->name << Data::OnlineText(user, time);
		ci->id = user->id;
		ci->firstName = peer->name;
		ci->lastName = qsl("");
		if (auto userpic = peer->currentUserpic()) {
			ci->hasAvatar = true;
		}
		ci->peerData = peer;
		ci->parentId = parentId;
		ci->online = Data::OnlineTextActive(user, time);
		ci->lastLoginTime = Data::OnlineText(user, time);
	}

}