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

#ifndef IMAGESTATISTICSCALCULATOR_H
#define IMAGESTATISTICSCALCULATOR_H

#define NUMBER_OF_HISTOGRAM_BUFFERS 2

#include <QObject>
#include <QVector>
#include <QRect>
#include <QApplication>
#include <QtMath>

struct ImageStatistics {
	int pixels;
	qreal max;
	qreal min;
	qreal sum;
	qreal average;
	qreal stdDeviation;
	qreal coeffOfVariation;
	int roiX;
	int roiY;
	int roiWidth;
	int roiHeight;
};

class ImageStatisticsCalculator : public QObject
{
	Q_OBJECT
public:
	explicit ImageStatisticsCalculator(QObject *parent = nullptr);

private:
	bool calculationRunnging;
	ImageStatistics stats;
	QVector<qreal> histogramX;
	QVector<QVector<qreal>> histogramY;
	int currHistogramBufferID;
	QRect roi;

	QPoint indexToPoint(int index, int width);
	qreal standardDeviation(QVector<qreal> samples, qreal mean);
	template <typename T> void calculateStatistics(T frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);


signals:
	void statisticsCalculated(ImageStatistics*);
	void histogramCalculated(QVector<qreal>* x, QVector<qreal>* y);
	void info(QString);
	void error(QString);

public slots:
	void slot_calculateStatistics(void* frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void slot_setROI(int x, int y, int width, int height);
};

#endif // IMAGESTATISTICSCALCULATOR_H
