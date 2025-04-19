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

#ifndef ROISELECTOR_H
#define ROISELECTOR_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QThread>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QtMath>
#include "bitdepthconverter.h"
#include "resizablerectitem.h"
#include "resizablerectitemsettings.h"

class ROISelector : public QGraphicsView
{
	Q_OBJECT
	QThread converterThread;

public:
	explicit ROISelector(QWidget *parent = nullptr);
	~ROISelector();

private:
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void scaleView(qreal scaleFactor);



private:
	BitDepthConverter* bitConverter;
	QGraphicsScene* scene;
	QGraphicsPixmapItem* inputItem;
	ResizableRectItem* roiRect;
	ResizableRectItemSettings* roiRectSettings;
	QGraphicsTextItem* roiRectText;
	int frameWidth;
	int frameHeight;
	int mousePosX;
	int mousePosY;

signals:
	void roiChanged(int x, int y, int width, int height);
	void non8bitFrameReceived(void *frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void info(QString);
	void error(QString);

public slots:
	void slot_zoomIn();
	void slot_zoomOut();
	void slot_receiveFrame(void* frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void slot_displayFrame(uchar* frame, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void slot_updateROI();
};

#endif // ROISELECTOR_H
