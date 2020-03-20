#include "delegatehelper.h"
namespace Contact {

	DelegateHelper::DelegateHelper()
	{
	}

	DelegateHelper::~DelegateHelper()
	{
		
	}

	QRect DelegateHelper::calTextRect(QPainter* painter, int fontSize, QString text, QString fontFamily) const
	{
		//m_font->setPointSize(size);
		//QFontMetrics fm(*m_font);
		//return fm.boundingRect(text);

		if (nullptr == painter)
		{
			return QRect();
		}
		painter->save();
		QFont font = painter->font();
		font.setFamily(fontFamily);
		font.setPixelSize(fontSize);
		QFontMetrics fm(font);
		painter->restore();
		return fm.boundingRect(text);
	}

	void DelegateHelper::paintText(QPainter* painter, Qt::AlignmentFlag alignFlag, const QColor& color, const QRect& paintRect, int fontSize, QString content /*= ""*/, QString fontFamily /*= "Microsoft YaHei"*/)
	{
		if (nullptr == painter)
		{
			return;
		}
		painter->save();

		QFont font = painter->font();
		font.setFamily(fontFamily);
		font.setPixelSize(fontSize);
		painter->setFont(font);
		painter->setPen(color);
		painter->drawText(paintRect, alignFlag, content);
		//painter->save();
		painter->restore();
	}

	void DelegateHelper::paintEllipse(QPainter* painter, const QColor& color, const QRect& paintRect)
	{
		painter->save();
		// 反走样
		painter->setRenderHint(QPainter::Antialiasing, true);
		// 设置画笔颜色、宽度
		painter->setPen(QPen(color, 1));
		// 设置画刷颜色
		painter->setBrush(color);
		// 绘制圆
		painter->drawEllipse(paintRect);
		painter->restore();
	}

	void DelegateHelper::paintRect(QPainter* painter, const QColor& color, const QRect& paintRect)
	{
		painter->save();
		// 反走样
		painter->setRenderHint(QPainter::Antialiasing, true);
		// 设置画笔颜色、宽度
		painter->setPen(QPen(color, 1));
		// 设置画刷颜色
		painter->setBrush(color);
		// 绘制圆
		painter->drawRect(paintRect);
		painter->restore();
	}

}