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

#include "histogramplot.h"

HistogramPlot::HistogramPlot(QWidget* parent) : QCustomPlot(parent)
{
	//configure appearance of pot area
	this->yAxis->setVisible(false);
	this->xAxis->setVisible(false);
	this->setBackground( QColor(50, 50, 50));
	this->axisRect()->setBackground(QColor(25, 25, 25));

	//maximize size of plot area
	this->axisRect()->setAutoMargins(QCP::msNone);
	this->axisRect()->setMargins(QMargins(0,0,0,0));

	//configure appearance of bars in plot area
	this->bars = new QCPBars(this->xAxis, this->yAxis);
//	this->bars->setAntialiased(false);
//	this->bars->setAntialiasedScatters(false);
//	this->bars->setAntialiasedFill(false);
//	this->setNoAntialiasingOnDrag(true);
	QPen barPen = QPen(QColor(200, 200, 200));
	barPen.setWidth(1);
	this->bars->setPen(barPen);
	this->bars->setBrush(QColor(150, 150, 150));
	this->bars->setWidthType(QCPBars::wtPlotCoords);
	this->bars->setWidth(1.0);

	//set user interactions
	this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

	//init bools for frames per second limitation and autoscale on first run
	this->updatingEnabled = false;
	this->fpsLimit = false;

	//fill histogram plot with arbitrary data to see appearance of plot without providing actual data
	QVector<double> x3(4096), y3(4096);
	int center = x3.size()/2;
	for (int i=0; i<x3.size(); i++){
		x3[i] = i;
		double xi = i - center;
		double xiNorm = xi / (x3.size() - 1);
		y3[i] = 512 * exp(-64*(xiNorm*xiNorm));
	}
	this->bars->setData(x3, y3);
	this->fitView();
}

void HistogramPlot::setAxisColor(QColor color) {
	this->xAxis->setBasePen(QPen(color, 1));
	this->yAxis->setBasePen(QPen(color, 1));
	this->xAxis->setTickPen(QPen(color, 1));
	this->yAxis->setTickPen(QPen(color, 1));
	this->xAxis->setSubTickPen(QPen(color, 1));
	this->yAxis->setSubTickPen(QPen(color, 1));
	this->xAxis->setTickLabelColor(color);
	this->yAxis->setTickLabelColor(color);
	this->xAxis->setLabelColor(color);
	this->yAxis->setLabelColor(color);
}

void HistogramPlot::zoomOutSlightly() {
	this->yAxis->scaleRange(1.1, this->yAxis->range().center());
	this->xAxis->scaleRange(1.1, this->xAxis->range().center());
}

void HistogramPlot::fitView() {
	this->rescaleAxes();
	this->zoomOutSlightly();
	this->replot();
}

void HistogramPlot::contextMenuEvent(QContextMenuEvent* event) {
	QMenu menu(this);
	QAction savePlotAction(tr("Save Plot as..."), this);
	connect(&savePlotAction, &QAction::triggered, this, &HistogramPlot::slot_saveToDisk);
	menu.addAction(&savePlotAction);
	menu.exec(event->globalPos());
}

void HistogramPlot::mouseMoveEvent(QMouseEvent* event) {
	if(!(event->buttons() & Qt::LeftButton)){
		int x = this->xAxis->pixelToCoord(event->pos().x());
		int y = this->yAxis->pixelToCoord(event->pos().y());
		this->setToolTip(QString("%1 , %2").arg(x).arg(y));
	}else{
		QCustomPlot::mouseMoveEvent(event);
	}
}

void HistogramPlot::mouseDoubleClickEvent(QMouseEvent* event) {
	this->fitView();
}

void HistogramPlot::slot_saveToDisk() {
	QString filters("Image (*.png);;Vector graphic (*.pdf)");
	QString defaultFilter("Image (*.png)");
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Plot"), QDir::currentPath(), filters, &defaultFilter);
	if(fileName == ""){
		//emit error(tr("Save plot to disk canceled."));
		return;
	}
	bool saved = false;
	if(defaultFilter == "Image (*.png)"){
		saved = this->savePng(fileName);
	}else if(defaultFilter == "Vector graphic (*.pdf)"){
		saved = this->savePdf(fileName);
	}
	if(saved){
		//emit info(tr("Plot saved to ") + fileName);
	}else{
		//emit error(tr("Could not save plot to disk."));
	}
}

void HistogramPlot::slot_updatePlot(QVector<qreal>* x, QVector<qreal>* y) {
	if(!this->fpsLimit){
		this->fpsLimit = true;
		this->bars->setData(*x, *y, true);
		this->replot();
		QTimer::singleShot(1000/MAX_FPS, this, SLOT(slot_disableFpsLimit()));
	}
}

void HistogramPlot::slot_disableFpsLimit() {
	this->fpsLimit = false;
}
