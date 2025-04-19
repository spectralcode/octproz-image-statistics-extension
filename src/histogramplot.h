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

#ifndef HISTOGRAMPLOT_H
#define HISTOGRAMPLOT_H

#define MAX_FPS 25

#include <QTimer>
#include "qcustomplot.h"

class HistogramPlot : public QCustomPlot
{
	Q_OBJECT
public:
	explicit HistogramPlot(QWidget* parent = nullptr);

private:
	QCPBars* bars;
	bool updatingEnabled;
	bool fpsLimit;

	void setAxisColor(QColor color);
	void zoomOutSlightly();
	void fitView();


protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

signals:

public slots:
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	void slot_saveToDisk();
	void slot_updatePlot(QVector<qreal>* x, QVector<qreal>* y);
	void slot_disableFpsLimit();
	void slot_enableUpdating(bool enable){this->updatingEnabled = enable;}
};

#endif // HISTOGRAMPLOT_H
