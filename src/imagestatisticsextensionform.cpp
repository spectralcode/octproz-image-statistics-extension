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

#include "imagestatisticsextensionform.h"
#include "ui_imagestatisticsextensionform.h"

ImageStatisticsExtensionForm::ImageStatisticsExtensionForm(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ImageStatisticsExtensionForm)
{
	this->ui->setupUi(this);

	this->updateHistogramOnce = false;
	this->updateStatisticsOnce = false;

	QStringList srcOptions = { "Raw", "Processed"};
	this->ui->comboBox_source->addItems(srcOptions);

	connect(this->ui->comboBox_source, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageStatisticsExtensionForm::slot_setSource);
	connect(this->ui->pushButton_updateHistogram, &QPushButton::clicked, this, &ImageStatisticsExtensionForm::slot_updateHistogramPlotOnce);
	connect(this->ui->pushButton_updateStatistics, &QPushButton::clicked, this, &ImageStatisticsExtensionForm::slot_updateStatisticsOnce);
	connect(this->ui->checkBox_autoUpdateHistogram, &QAbstractButton::toggled, this, &ImageStatisticsExtensionForm::slot_enableAutoUpdateHistogram);
	connect(this->ui->checkBox_autoUpdateStatistics, &QAbstractButton::toggled, this, &ImageStatisticsExtensionForm::slot_enableAutoUpdateStatistics);

	connect(this->ui->horizontalSlider_frame, &QSlider::valueChanged, this->ui->spinBox_frame, &QSpinBox::setValue);
	connect(this->ui->spinBox_frame, QOverload<int>::of(&QSpinBox::valueChanged), this->ui->horizontalSlider_frame, &QSlider::setValue);
	connect(this->ui->horizontalSlider_frame, &QSlider::valueChanged, this, &ImageStatisticsExtensionForm::slot_setFrameNr);
	this->slot_setMaximumFrameNr(512);

	this->ui->spinBox_buffer->setMaximum(2);
	this->ui->spinBox_buffer->setMinimum(-1);
	this->ui->spinBox_buffer->setSpecialValueText(tr("All"));
	connect(this->ui->spinBox_buffer, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageStatisticsExtensionForm::slot_setBufferNr);
}

ImageStatisticsExtensionForm::~ImageStatisticsExtensionForm()
{
	delete this->ui;
}

void ImageStatisticsExtensionForm::setSettings(QVariantMap settings){
	this->slot_enableAutoUpdateHistogram(settings.value(AUTO_UPDATE_HISTOGRAM).toBool());
	this->slot_enableAutoUpdateStatistics(settings.value(AUTO_UPDATE_STATISTICS).toBool());
	this->slot_setSource(settings.value(BUFFER_SRC).toInt());
	this->slot_setBufferNr(settings.value(BUFFER_NR).toInt());
	this->slot_setFrameNr(settings.value(FRAME_NR).toInt());
	restoreGeometry(settings.value(GEOMETRY).toByteArray());
}

void ImageStatisticsExtensionForm::getSettings(QVariantMap* settings) {
	settings->insert(AUTO_UPDATE_HISTOGRAM, this->parameters.updateHistogramEnabled);
	settings->insert(AUTO_UPDATE_STATISTICS, this->parameters.updateStatisticsEnabled);
	settings->insert(BUFFER_SRC, this->parameters.bufferSrc);
	settings->insert(BUFFER_NR,this->parameters.bufferNr);
	settings->insert(FRAME_NR, this->parameters.frameNr);
	settings->insert(GEOMETRY, saveGeometry());
}

ROISelector *ImageStatisticsExtensionForm::getROISelector() {
	return this->ui->widget_roiselector;
}

HistogramPlot *ImageStatisticsExtensionForm::getHistogramPlot() {
	return this->ui->widget_histogramplot;
}

void ImageStatisticsExtensionForm::slot_updateHistogramPlot(QVector<qreal> *x, QVector<qreal> *y) {
	if(this->parameters.updateHistogramEnabled || this->updateHistogramOnce){
		this->ui->widget_histogramplot->slot_updatePlot(x, y);
		this->updateHistogramOnce = false;
	}
}

void ImageStatisticsExtensionForm::slot_updateStatistics(ImageStatistics* statistics) {
	if(this->parameters.updateStatisticsEnabled || this->updateStatisticsOnce){
		this->ui->label_pixels->setText(QString::number(statistics->pixels));
		this->ui->label_sum->setText(QString::number(statistics->sum));
		this->ui->label_average->setText(QString::number(statistics->average));
		this->ui->label_stdDeviation->setText(QString::number(statistics->stdDeviation));
		this->ui->label_coeffOfVariation->setText(QString::number(statistics->coeffOfVariation));
		this->ui->label_min->setText(QString::number(statistics->min));
		this->ui->label_max->setText(QString::number(statistics->max));
		this->ui->label_roix->setText(QString::number(statistics->roiX));
		this->ui->label_roiy->setText(QString::number(statistics->roiY));
		this->ui->label_roiwidth->setText(QString::number(statistics->roiWidth));
		this->ui->label_roiheight->setText(QString::number(statistics->roiHeight));
		this->updateStatisticsOnce = false;
	}
}

void ImageStatisticsExtensionForm::slot_enableAutoUpdateHistogram(bool enable) {
	this->ui->checkBox_autoUpdateHistogram->setChecked(enable);
	this->ui->pushButton_updateHistogram->setEnabled(!enable);
	this->ui->widget_histogramplot->slot_enableUpdating(enable);
	this->parameters.updateHistogramEnabled = enable;
	emit parametersUpdated();
}

void ImageStatisticsExtensionForm::slot_enableAutoUpdateStatistics(bool enable) {
	this->ui->checkBox_autoUpdateStatistics->setChecked(enable);
	this->ui->pushButton_updateStatistics->setEnabled(!enable);
	this->parameters.updateStatisticsEnabled = enable;
	emit parametersUpdated();
}

void ImageStatisticsExtensionForm::slot_updateHistogramPlotOnce() {
	this->updateHistogramOnce = true;
}

void ImageStatisticsExtensionForm::slot_updateStatisticsOnce() {
	this->updateStatisticsOnce = true;
}

void ImageStatisticsExtensionForm::slot_setSource(int index) {
	this->parameters.bufferSrc = static_cast<BUFFER_SOURCE>(index);
	this->ui->comboBox_source->setCurrentIndex(index);
	emit sourceChanged(static_cast<BUFFER_SOURCE>(index));
	emit parametersUpdated();
}

void ImageStatisticsExtensionForm::slot_setMaximumFrameNr(int maximum) {
	this->ui->horizontalSlider_frame->setMaximum(maximum);
	this->ui->spinBox_frame->setMaximum(maximum);
}

void ImageStatisticsExtensionForm::slot_setMaximumBufferNr(int maximum) {
	this->ui->spinBox_buffer->setMaximum(maximum);
}

void ImageStatisticsExtensionForm::slot_setFrameNr(int frameNr) {
	this->ui->horizontalSlider_frame->setValue(frameNr);
	this->parameters.frameNr = frameNr;
	emit frameNrChanged(frameNr);
	emit parametersUpdated();
}

void ImageStatisticsExtensionForm::slot_setBufferNr(int bufferNr) {
	this->ui->spinBox_buffer->setValue(bufferNr);
	this->parameters.bufferNr = bufferNr;
	emit bufferNrChanged(bufferNr);
	emit parametersUpdated();
}

void ImageStatisticsExtensionForm::resizeEvent(QResizeEvent *event) {
	emit parametersUpdated();
	QWidget::resizeEvent(event);
}

void ImageStatisticsExtensionForm::moveEvent(QMoveEvent *event) {
	emit parametersUpdated();
	QWidget::moveEvent(event);
}
