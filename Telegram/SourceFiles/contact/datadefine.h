#ifndef DATADEFINE
#define DATADEFINE
#include "dataType.h"
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
            , hasAvatar(false)
            , online(false)
            , expanded(false)
        {
        }

        uint32_t id;
        uint32_t parentId;
        QString firstName; //名 Give Name
        QString lastName;  //姓 Family Name
        QString phoneNum; //手机号码
        QString lastLoginTime; //最后一次上线时间
        QString showUserCount; //分组内用户数信息 在线数/总数
        int userTotalCount; //总用户数
        int userOnlineCount; //在线用户数
        bool hasAvatar; //是否有头像
        bool online; //是否在线
        bool expanded;

    };

    struct ContactStyleInfo
    {
        ContactStyleInfo()
            : fontName("Microsoft YaHei")
        {
        }

        QString fontName;
        QColor fontColor;
        int fontSize;
        QColor userCountFontColor;
        int avatarWidth;
        int avatarHeight;

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
    };

    QString getAllFileContent(const QString& path);
}

#endif // DATADEFINE

