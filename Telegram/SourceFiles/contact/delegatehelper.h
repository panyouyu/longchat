#ifndef DELEGATEHELPER_H
#define DELEGATEHELPER_H
#include "datadefine.h"
namespace Contact {

    class DelegateHelper
    {

    public:
        DelegateHelper();
        ~DelegateHelper();

    public:
        //计算文本区域
		QRect calTextRect(QPainter* painter, int fontSize, QString text, QString fontFamily = "Microsoft YaHei") const;
		// 画文字
		void paintText(QPainter* painter, Qt::AlignmentFlag alignFlag, const QColor& color, const QRect& paintRect, int fontSize, QString content = "", QString fontFamily = "Microsoft YaHei");
        //画圆
        void paintEllipse(QPainter* painter, const QColor& color, const QRect& paintRect);

    };
}

#endif
