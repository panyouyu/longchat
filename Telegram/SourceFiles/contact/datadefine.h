#ifndef DATADEFINE
#define DATADEFINE
#include "base/basic_types.h"
#include "ui/image/image_location.h"
#include "data/data_peer.h"
#include <QString>
namespace Contact {
#define DEFAULT_VALUE_ZERO    0
    //#define NOT_SET_VALUE    4294967295 //-1 99999999  4294967295

    struct ContactInfo
    {
        ContactInfo()
            : id(DEFAULT_VALUE_ZERO)
            , parentId(DEFAULT_VALUE_ZERO)
            , userTotalCount(0)
            , userOnlineCount(0)
            //, pAvatarImage(nullptr)
            , peerData(nullptr)
            , hasAvatar(false)
            , online(false)
            , expanded(false)
            , isGroup(false)
            
        {
        }

        uint64 id;
        uint64 parentId;
        QString firstName; //名 Give Name
        QString lastName;  //姓 Family Name
        QString phoneNum; //手机号码
        QString lastLoginTime; //最后一次上线时间
        QString showUserCount; //分组内用户数信息 在线数/总数
        QString serverCount; //服务人数
        QString queueCount; //排队人数
        int32 userTotalCount; //总用户数
        int32 userOnlineCount; //在线用户数
        
        QVector<uint64> userIds;
        PeerData* peerData;
        bool hasAvatar; //是否有头像
        bool online; //是否在线
        bool expanded;
        bool isGroup; //就否分组

    };
    
    struct ContactStyleInfo
    {
        ContactStyleInfo()
            : fontName("Microsoft YaHei")
        {
        }

        QString fontName;
        QColor fontColor;
        int32 fontSize;
        QColor userCountFontColor;
        int32 avatarWidth;
        int32 avatarHeight;

    };
   

    // 借用隐藏列存储的数据
    //enum CustomColumn
    //{
    //    IsExpandedColn = 1,// 是否展开
    //};

    // 绘制角色
    enum CustomRole
    {
        IsExpandedRole = Qt::UserRole + 1000, // 是否展开
        IsGroupRole, // 是否是群组
        PeerRole, // 获取peer
    };
    //创建树类型
	enum CreatingTreeType {
		CTT_FULL, //联系人主界面的树 支持搜索右键 二级目录
        CTT_TOSELECT, //分组维护窗口 支持选择到右边树，不支持二级目录 
		CTT_SHOW, //分组维护窗口 右侧树 只显示选中的用户 不支持二级目录 
        CTT_SWITCH, //转换窗口
	};

	//分组窗口操作类型
	enum GroupOperWindowType {
		GOWT_ADD,
		GOWT_MOD,
	};

    QString getAllFileContent(const QString& path);
}

#endif // DATADEFINE

