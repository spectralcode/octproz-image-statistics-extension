//This file is a modified version of code originally created by GitHub user sashoalm, please see: https://github.com/sashoalm/ResizableRectItem


#ifndef RESIZABLERECTITEM_H
#define RESIZABLERECTITEM_H

#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QCursor>

class ResizableRectItemSettings;

class ResizableRectItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT
public:
	ResizableRectItem(QRectF rect, const ResizableRectItemSettings *settings, QGraphicsItem *parent = 0);
	qreal getInnerRectWidth();
	qreal getInnerRectHeight();
	QRectF getInnerRect() const;


signals:
	void rectChanged();

private:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void resizeRect(QGraphicsSceneMouseEvent *event);
	const ResizableRectItemSettings *settings;
};

#endif // RESIZABLERECTITEM_H
