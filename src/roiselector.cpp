/**
**  This file is part of ImageStatisticsExtension for OCTproZ.
**  ImageStatisticsExtension is a plugin for OCTproZ that displays
**  image statistics such as a histogram of live acquired OCT data.
**  Copyright (C) 2020 Miroslav Zabic
**
**  ImageStatisticsExtension is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program. If not, see http://www.gnu.org/licenses/.
**
****
** Author:	Miroslav Zabic
** Contact:	zabic
**			at
**			iqo.uni-hannover.de
****
**/

#include "roiselector.h"

ROISelector::ROISelector(QWidget *parent) : QGraphicsView(parent)
{
	this->scene = new QGraphicsScene(this);
	scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	//scene->setSceneRect(0, 0, this->width(), this->height());
	setScene(scene);
	setCacheMode(CacheBackground);
	setViewportUpdateMode(BoundingRectViewportUpdate);
	setRenderHint(QPainter::Antialiasing);
	setTransformationAnchor(AnchorUnderMouse);

	this->inputItem = new QGraphicsPixmapItem();
	this->scene->addItem(inputItem);
	this->scene->update();

	//add movable and resizable ROI rectangle
	QBrush brush(QColor(255, 0, 0, 64));
	this->roiRectSettings = new ResizableRectItemSettings(10, QSizeF(30, 30), QSizeF(8192, 8192), Qt::DashLine, brush);
	this->roiRect = new ResizableRectItem(QRectF(QPointF(10, 20), QPointF(250, 220)), roiRectSettings);
	this->roiRect->setBrush(brush);
	this->roiRect->setFlag(QGraphicsItem::ItemIsMovable, true);
	this->scene->addItem(this->roiRect);
	this->roiRect->setPos(10, 10);
	this->roiRectText = new QGraphicsTextItem("ROI", roiRect);
	this->roiRectText->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	connect(this->roiRect, &ResizableRectItem::rectChanged, this, &ROISelector::slot_updateROI);

	this->frameWidth = 0;
	this->frameHeight = 0;
	this->mousePosX = 0;
	this->mousePosY = 0;

	//setup bitconverter
	this->bitConverter = new BitDepthConverter();
	this->bitConverter->moveToThread(&converterThread);
	connect(this, &ROISelector::non8bitFrameReceived, this->bitConverter, &BitDepthConverter::slot_convertDataTo8bit);
	connect(this->bitConverter, &BitDepthConverter::info, this, &ROISelector::info);
	connect(this->bitConverter, &BitDepthConverter::error, this, &ROISelector::error);
	connect(this->bitConverter, &BitDepthConverter::converted8bitData, this, &ROISelector::slot_displayFrame);
	connect(&converterThread, &QThread::finished, this->bitConverter, &BitDepthConverter::deleteLater);
	converterThread.start();
}

ROISelector::~ROISelector()
{
	converterThread.quit();
	converterThread.wait();
}

void ROISelector::mouseDoubleClickEvent(QMouseEvent *event) {
	this->fitInView(this->scene->sceneRect(), Qt::KeepAspectRatio);
	this->ensureVisible(this->inputItem);
	this->centerOn(this->pos());
	this->scene->setSceneRect(this->scene->itemsBoundingRect());
	QGraphicsView::mousePressEvent(event);
}

void ROISelector::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::MiddleButton) {
		this->mousePosX = event->x();
		this->mousePosY = event->y();
	}
	QGraphicsView::mousePressEvent(event);
}

void ROISelector::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() & Qt::MidButton) {
		QPointF oldPosition = mapToScene(this->mousePosX, this->mousePosY);
		QPointF newPosition = mapToScene(event->pos());
		QPointF translation = newPosition - oldPosition;
		this->translate(translation.x(), translation.y());
		this->mousePosX = event->x();
		this->mousePosY = event->y();
	}
	QGraphicsView::mouseMoveEvent(event);
}

void ROISelector::keyPressEvent(QKeyEvent* event) {
	switch (event->key()) {
	case Qt::Key_Plus:
		slot_zoomIn();
		break;
	case Qt::Key_Minus:
		slot_zoomOut();
		break;
	default:
		QGraphicsView::keyPressEvent(event);
	}
}

void ROISelector::wheelEvent(QWheelEvent* event) {
	if (event->modifiers() & Qt::ControlModifier) {
		// Do a wheel-based zoom about the cursor position
		double angle = event->angleDelta().y();
		double factor = qPow(1.0015, angle);
		QPoint targetViewportPos = event->pos();
		QPointF targetScenePos = mapToScene(event->pos());
		this->scale(factor, factor);
		this->centerOn(targetScenePos);
		QPointF deltaViewportPos = targetViewportPos - QPointF(viewport()->width() / 2.0, viewport()->height() / 2.0);
		QPointF viewportCenter = mapFromScene(targetScenePos) - deltaViewportPos;
		this->centerOn(mapToScene(viewportCenter.toPoint()));
		return;
	}
	QGraphicsView::wheelEvent(event);
}

void ROISelector::scaleView(qreal scaleFactor) {
	qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
	if (factor < 0.07 || factor > 100){
		return;
	}
	this->scale(scaleFactor, scaleFactor);
}

void ROISelector::slot_zoomIn() {
	this->scaleView(qreal(1.2));
}

void ROISelector::slot_zoomOut() {
	this->scaleView(1/qreal(1.2));
}

void ROISelector::slot_receiveFrame(void *frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame) {
	if(bitDepth != 8){
		emit non8bitFrameReceived(frame, bitDepth, samplesPerLine, linesPerFrame);
	}else{
		this->slot_displayFrame(static_cast<uchar*>(frame), samplesPerLine, linesPerFrame);
	}
}

void ROISelector::slot_displayFrame(uchar* frame, unsigned int samplesPerLine, unsigned int linesPerFrame) {
	//create QPixmap from uchar array and update inputItem
	QImage image(frame, samplesPerLine, linesPerFrame, QImage::Format_Grayscale8 );
	this->inputItem->setPixmap(QPixmap::fromImage(image));

	//scale view if input sizes have changed
	if(this->frameWidth != samplesPerLine || this->frameHeight != linesPerFrame){
		this->frameWidth = samplesPerLine;
		this->frameHeight = linesPerFrame;
		this->fitInView(this->scene->sceneRect(), Qt::KeepAspectRatio);
		this->ensureVisible(this->inputItem);
		this->centerOn(this->pos());

		//set scene rect back to minimal size
		this->scene->setSceneRect(this->scene->itemsBoundingRect());

		this->slot_updateROI();
	}
}

void ROISelector::slot_updateROI() {
	QRectF innerRect = this->roiRect->getInnerRect();
	qreal xpos = this->roiRect->x()+innerRect.x();
	qreal ypos = this->roiRect->y()+innerRect.y();
	qreal width = this->roiRect->getInnerRectWidth();
	qreal height = this->roiRect->getInnerRectHeight();

	QRectF frameRect = this->inputItem->boundingRect();
	qreal xposFrame = this->inputItem->x();
	qreal yposFrame = this->inputItem->y();
	qreal widthFrame = frameRect.width();
	qreal heightFrame = frameRect.height();

	qreal roiX = frameRect.x() - xpos;
	qreal roiY = frameRect.y() - ypos;
	qreal widthdelta = frameRect.width() - width;
	qreal heightdelta = frameRect.height() - height;

	emit roiChanged(static_cast<int>(-roiX), static_cast<int>(-roiY), static_cast<int>(width), static_cast<int>(height));
}
