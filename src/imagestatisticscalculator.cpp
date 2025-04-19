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

#include "imagestatisticscalculator.h"


ImageStatisticsCalculator::ImageStatisticsCalculator(QObject *parent) : QObject(parent)
{
	this->stats = {0,0,0,0};
	this->histogramX.reserve(256);
	this->histogramY.resize(NUMBER_OF_HISTOGRAM_BUFFERS); //use multi buffer for histogram values
	for(int i = 0; i < NUMBER_OF_HISTOGRAM_BUFFERS; i++){
		this->histogramY[i].reserve(256);
	}
	this->currHistogramBufferID = 0;
	this->calculationRunnging = false;
	this->roi.setRect(0, 0, 1024, -1024);
}

QPoint ImageStatisticsCalculator::indexToPoint(int index, int width){
	int x = index%width;
	int y = index/width;
	return QPoint(x, y);
}

qreal ImageStatisticsCalculator::standardDeviation(QVector<qreal> samples, qreal mean) {
	qreal sum = 0;
	for (int i = 0; i < samples.length(); i++){
		sum = sum + qPow(samples.at(i) - mean, 2);
	}
	return qSqrt(sum/(samples.length()));
}

void ImageStatisticsCalculator::slot_calculateStatistics(void* frameBuffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame) {
	if(!this->calculationRunnging){
		this->calculationRunnging = true;

		//set buffer datatype according bitdepth and start statistics calculation
		//uchar
		if(bitDepth <= 8){
			unsigned char* frame = static_cast<unsigned char*>(frameBuffer);
			this->calculateStatistics(frame, bitDepth, samplesPerLine, linesPerFrame);
		}
		//ushort
		else if(bitDepth > 8 && bitDepth <= 16){
			unsigned short* frame = static_cast<unsigned short*>(frameBuffer);
			this->calculateStatistics(frame, bitDepth, samplesPerLine, linesPerFrame);
		}
		//unsigned long int
		else if(bitDepth > 16 && bitDepth <= 32){
			unsigned long int* frame = static_cast<unsigned long int*>(frameBuffer);
			this->calculateStatistics(frame, bitDepth, samplesPerLine, linesPerFrame);
		}

		emit statisticsCalculated(&(this->stats));
		emit histogramCalculated(&(this->histogramX), &(this->histogramY[this->currHistogramBufferID]));

		QApplication::processEvents();
		this->calculationRunnging = false;
	}
}

void ImageStatisticsCalculator::slot_setROI(int x, int y, int width, int height) {
	this->roi.setRect(x, y, width, height);
}

template<typename T>
void ImageStatisticsCalculator::calculateStatistics(T frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame) {
	//resize histogram plot vectors if necessary
	int numberOfPossibleValues = static_cast<int>(pow(2, bitDepth));
	if(this->histogramX.size() != numberOfPossibleValues){
		this->histogramX.resize(numberOfPossibleValues);
		for(int i = 0; i < NUMBER_OF_HISTOGRAM_BUFFERS; i++){
			this->histogramY[i].resize(numberOfPossibleValues);
		}
		for(int i = 0; i<numberOfPossibleValues; i++){
			this->histogramX[i] = i;
		}
	}
	//init params for statistic calculation
	this->currHistogramBufferID = (this->currHistogramBufferID+1)%NUMBER_OF_HISTOGRAM_BUFFERS;
	qreal sum = 0;
	qreal length = samplesPerLine*linesPerFrame;
	this->histogramY[this->currHistogramBufferID].fill(0);
	qreal maxValue = 0;
	qreal minValue = 999999999;
	int pixels = 0;
	QVector<qreal> samples;

	//statistics calculation
	for(int i = 0; i < length; i++){
		if(this->roi.contains(this->indexToPoint(i, static_cast<int>(samplesPerLine)))){
			qreal currValue = frame[i];
			samples.append(currValue);
			if(maxValue < currValue){maxValue = currValue;}
			if(minValue > currValue){minValue = currValue;}
			pixels++;
			sum += currValue;

			if(currValue>(numberOfPossibleValues-1)){
				currValue = numberOfPossibleValues-1;
			}
			if(currValue<0){
				currValue = 0;
			}
			this->histogramY[this->currHistogramBufferID][static_cast<int>(currValue)]++;
		}
	}

	//update ImageStatistics struct
	this->stats.max = maxValue;
	this->stats.min = minValue;
	this->stats.pixels = pixels;
	this->stats.sum = sum;
	this->stats.average = this->stats.sum/pixels;
	this->stats.stdDeviation = this->standardDeviation(samples, this->stats.average);
	this->stats.coeffOfVariation = this->stats.stdDeviation/this->stats.average;
	this->stats.roiX = this->roi.x();
	this->stats.roiY = this->roi.y();
	this->stats.roiWidth = this->roi.width();
	this->stats.roiHeight = this->roi.height();
}
