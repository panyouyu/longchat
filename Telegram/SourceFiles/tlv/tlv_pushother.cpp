#include "tlv/tlv_pushother.h"

#include "app.h"
#include "apiwrap.h"
#include "mainwidget.h"
#include "auth_session.h"
#include "logs.h"
#include "window/window_controller.h"
#include "data/data_session.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_key.h"

namespace TLV {

namespace {
constexpr auto kIntSize = static_cast<int>(sizeof(mtpPrime));
}

void feedUserGroupUpdates(const MTPDtlv_list& tlv_list) {
	const auto &vector = tlv_list.vtlvs.v;
	for (auto &item : vector) {
		const auto &tlv = item.c_tlv();
		switch (tlv.vtlv_constructor.v) {
		case mtpc_tlvc_updateUserGroups: {
			auto from = reinterpret_cast<const mtpPrime*>(tlv.vtlv_data.v.constData());			
			auto end = from + tlv.vtlv_data.v.size() / kIntSize;
			auto sfrom = from - 4U;
			TLV_LOG(("TLV: ") + mtpTextSerialize(sfrom, end));
			from++;
			try {
				MTPupdateUserGroups groups;
				groups.read(from, end);
				const auto& d = groups.c_tlvc_updateUserGroups();
				const auto& adds = d.vadds.v;
				const auto& removes = d.vremoves.v;
				for (const auto& add : adds) {
					const auto &add_group = add.c_tlvc_updateUserGroup();
					auto user = Auth().data().user(add_group.vuser_id.v);
					Auth().api().requestPeerRelatedInfo(user);
					Auth().api().requestPeerLabels(user);
				}
				for (const auto& remove : removes) {
					const auto& remove_group = remove.c_tlvc_updateUserGroup();
					auto user_id = remove_group.vuser_id.v;
					auto history = Auth().data().history((PeerId)user_id);
					if (App::main()->dialogsList()->contains(history)) {
						App::main()->removeDialog(Dialogs::Key(history));
						if (App::main()->peer() && App::main()->peer()->id == user_id) {
							App::main()->showBackFromStack(Window::SectionShow());
						}
					}
				}
			} catch (Exception &e) {
			}
		} break;
			
		default: break;
		}
	}
}

} // namespace TLV