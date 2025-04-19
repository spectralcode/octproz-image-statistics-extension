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

#ifndef IMAGESTATISTICSEXTENSIONFORM_H
#define IMAGESTATISTICSEXTENSIONFORM_H


#define AUTO_UPDATE_HISTOGRAM "auto_upudate_histogram"
#define AUTO_UPDATE_STATISTICS "auto_update_statistics"
#define BUFFER_SRC "buffer_source"
#define BUFFER_NR "buffer_nr"
#define FRAME_NR "frame_nr"
#define GEOMETRY "geometry"

#include <QWidget>
#include "roiselector.h"
#include "histogramplot.h"
#include "imagestatisticscalculator.h"

enum BUFFER_SOURCE{
	RAW,
	PROCESSED
};

namespace Ui {
class ImageStatisticsExtensionForm;
}


struct statisticExtensionParameters {
	bool updateStatisticsEnabled;
	bool updateHistogramEnabled;
	BUFFER_SOURCE bufferSrc;
	int bufferNr;
	int frameNr;
};

class ImageStatisticsExtensionForm : public QWidget
{
	Q_OBJECT

public:
	explicit ImageStatisticsExtensionForm(QWidget *parent = 0);
	~ImageStatisticsExtensionForm();

	void setSettings(QVariantMap settings);
	void getSettings(QVariantMap* settings);

	Ui::ImageStatisticsExtensionForm* ui;
	ROISelector* getROISelector();
	HistogramPlot* getHistogramPlot();

public slots:
	void slot_updateStatistics(ImageStatistics* statistics);
	void slot_enableAutoUpdateHistogram(bool enable);
	void slot_enableAutoUpdateStatistics(bool enable);
	void slot_updateHistogramPlot(QVector<qreal>* x, QVector<qreal>* y);
	void slot_updateHistogramPlotOnce();
	void slot_updateStatisticsOnce();
	void slot_setSource(int index);
	void slot_setMaximumFrameNr(int maximum);
	void slot_setMaximumBufferNr(int maximum);
	void slot_setFrameNr(int frameNr);
	void slot_setBufferNr(int bufferNr);

private:
	void resizeEvent(QResizeEvent* event) override;
	void moveEvent(QMoveEvent* event) override;

	statisticExtensionParameters parameters;
	bool updateStatisticsOnce;
	bool updateHistogramOnce;

signals:
	void parametersUpdated();
	void sourceChanged(BUFFER_SOURCE src);
	void frameNrChanged(int frameNr);
	void bufferNrChanged(int bufferNr);

};

#endif // IMAGESTATISTICSEXTENSIONFORM_H
