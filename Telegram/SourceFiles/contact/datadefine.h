﻿#ifndef DATADEFINE
#define DATADEFINE
#include "base/basic_types.h"
#include "ui/image/image_location.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include <QString>
namespace Contact {
#define DEFAULT_VALUE_ZERO    0

    struct ContactInfo
    {
        ContactInfo()
            : id(DEFAULT_VALUE_ZERO)
            , parentId(DEFAULT_VALUE_ZERO)
            , serverNum(0)
            , serviceMax(5)
            , queueNum(0)
            , userTotalCount(0)
            , userOnlineCount(0)
            , otherId(0)
            , peerId(0)
            , hasAvatar(false)
            , online(false)
            , expanded(false)
            , isGroup(false)
            , userInGroup(false)
            , userInGroupTemp(false)
            
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
        int32 serverNum; //服务人数
        int32 serviceMax; //服务最大人数
        QString queueCount; //排队人数
        int32 queueNum; //排队人数
        int32 userTotalCount; //总用户数
        int32 userOnlineCount; //在线用户数
        int32 otherId;//功能 0=普通组 1=咨询中 2=已结束
        
        QVector<uint64> userIds;
        uint64 peerId;
        bool hasAvatar; //是否有头像
        bool online; //是否在线
        bool expanded;
        bool isGroup; //是否分组
        bool userInGroup; //是否用户所在分组
        bool userInGroupTemp; //记录原来的值，如果点取消，则要恢复

        ContactInfo& operator =(ContactInfo& r)
        {
            id = r.id;
			parentId = r.parentId;
			firstName = r.firstName;
			lastName = r.lastName;
			phoneNum = r.phoneNum;
			lastLoginTime = r.lastLoginTime;
			showUserCount = r.showUserCount;
			serverCount = r.serverCount;
			serverNum = r.serverNum;
			serviceMax = r.serviceMax;
			queueCount = r.queueCount;
			queueNum = r.queueNum;
			userTotalCount = r.userTotalCount;
			userOnlineCount = r.userOnlineCount;
			otherId = r.otherId;

			userIds = r.userIds;
			peerId = r.peerId;
			hasAvatar = r.hasAvatar;
			online = r.online;
			expanded = r.expanded;
			isGroup = r.isGroup;
			userInGroup = r.userInGroup;
			userInGroupTemp = r.userInGroupTemp;
            return *this;
        }

    };


	struct GroupInfo
	{
        GroupInfo()
			: id(DEFAULT_VALUE_ZERO)
            , otherId(0)

		{
		}

		uint64 id; //组id
        int32 otherId;//功能 0=普通组 1=咨询中 2=已结束
        QString gName;  //组名
        QVector<uint64> userIds; //组内的用户
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
        SwitchRole, //转接
        GroupCheckRole, //群组是否选中
    };
    //创建树类型
	enum CreatingTreeType {
		CTT_FULL, //联系人主界面的树 支持搜索右键 二级目录
        CTT_TOSELECT, //分组维护窗口 左侧树 支持选择到右边树，不支持二级目录 
		CTT_SHOW, //分组维护窗口 右侧树 只显示选中的用户 不支持二级目录 
        CTT_SWITCH, //转换窗口
        CTT_USERINGROUP, //用户所属分组选择窗口
	};

	//分组窗口操作类型
	enum GroupOperWindowType {
		GOWT_ADD,
		GOWT_MOD,
	};


	//分组功能 0=普通组 1=咨询中 2=已结束 3=排队中
	enum GroupFunctionType {
		GFT_NORMAL,
        GFT_SEEKING,
        GFT_SEEKED,
        GFT_QUEUE,
	};

    QString getAllFileContent(const QString& path);
    void genContact(ContactInfo* ci, uint64 peerId, uint64 parentId);
}
Q_DECLARE_METATYPE(Contact::ContactInfo*);
#endif // DATADEFINE
