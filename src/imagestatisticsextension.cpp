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

#include "imagestatisticsextension.h"


ImageStatisticsExtension::ImageStatisticsExtension() : Extension() {
	qRegisterMetaType<AcquisitionParams >("BUFFER_SOURCE");

	//init extension
	this->setType(EXTENSION);
	this->displayStyle = SEPARATE_WINDOW;
	this->name = "Image statistics";
	this->toolTip = "Live image statistics";

	//init gui
	this->form = new ImageStatisticsExtensionForm();
	this->roiSelect = this->form->getROISelector();
	this->widgetDisplayed = false;
	connect(this->form, &ImageStatisticsExtensionForm::parametersUpdated, this, &ImageStatisticsExtension::storeParameters);
	connect(this->form, &ImageStatisticsExtensionForm::sourceChanged, this, &ImageStatisticsExtension::setBufferSource);
	connect(this->form, &ImageStatisticsExtensionForm::frameNrChanged, this, &ImageStatisticsExtension::setFrameNr);
	connect(this->form, &ImageStatisticsExtensionForm::bufferNrChanged, this, &ImageStatisticsExtension::setBufferNr);
	connect(this, &ImageStatisticsExtension::maxFrames, this->form, &ImageStatisticsExtensionForm::slot_setMaximumFrameNr);
	connect(this, &ImageStatisticsExtension::maxBuffers, this->form, &ImageStatisticsExtensionForm::slot_setMaximumBufferNr);
	connect(this, &ImageStatisticsExtension::newFrame, this->roiSelect, &ROISelector::slot_receiveFrame);
	connect(this->roiSelect, &ROISelector::info, this, &ImageStatisticsExtension::info);
	connect(this->roiSelect, &ROISelector::error, this, &ImageStatisticsExtension::error);

	//init multi buffers
	this->frameBuffersRaw.resize(NUMBER_OF_BUFFERS);
	this->frameBuffersProcessed.resize(NUMBER_OF_BUFFERS);
	for(int i = 0; i < NUMBER_OF_BUFFERS; i++){
		this->frameBuffersRaw[i] = nullptr;
		this->frameBuffersProcessed[i] = nullptr;
	}
	this->copyBufferId = -1;
	this->bytesPerFrameProcessed = 0;
	this->bytesPerFrameRaw = 0;
	this->isCalculating = false;
	this->active = false;

	//init statistic calculater
	this->statisticsCalculator = new ImageStatisticsCalculator();
	this->statisticsCalculator->moveToThread(&statisticsCalculatorThread);
	connect(this->roiSelect, &ROISelector::roiChanged, this->statisticsCalculator, &ImageStatisticsCalculator::slot_setROI);
	connect(this, &ImageStatisticsExtension::newFrame, this->statisticsCalculator, &ImageStatisticsCalculator::slot_calculateStatistics);
	connect(this->statisticsCalculator, &ImageStatisticsCalculator::histogramCalculated, this->form, &ImageStatisticsExtensionForm::slot_updateHistogramPlot);
	connect(this->statisticsCalculator, &ImageStatisticsCalculator::statisticsCalculated, this->form, &ImageStatisticsExtensionForm::slot_updateStatistics);
	connect(this->statisticsCalculator, &ImageStatisticsCalculator::info, this, &ImageStatisticsExtension::info);
	connect(this->statisticsCalculator, &ImageStatisticsCalculator::error, this, &ImageStatisticsExtension::error);
	connect(&statisticsCalculatorThread, &QThread::finished, this->statisticsCalculator, &ImageStatisticsCalculator::deleteLater);
	statisticsCalculatorThread.start();

	//init variables for receiving buffers
	this->lostBuffersRaw = 0;
	this->lostBuffersProcessed = 0;
	this->bufferSource = PROCESSED;
	this->frameNr = 0;
	this->bufferNr = 0;
	this->framesPerBuffer = 0;
	this->buffersPerVolume = 0;
}

ImageStatisticsExtension::~ImageStatisticsExtension() {
	statisticsCalculatorThread.quit();
	statisticsCalculatorThread.wait();

	if(!this->widgetDisplayed){
		delete this->form;
	}

	this->releaseFrameBuffers(this->frameBuffersProcessed);
	this->releaseFrameBuffers(this->frameBuffersRaw);
}

QWidget* ImageStatisticsExtension::getWidget() {
	this->widgetDisplayed = true;
	return this->form;
}

void ImageStatisticsExtension::activateExtension() {
	//this method is called by OCTproZ as soon as user activates the extension. If the extension controls hardware components, they can be prepared, activated, initialized or started here.
	this->active = true;
}

void ImageStatisticsExtension::deactivateExtension() {
	//this method is called by OCTproZ as soon as user deactivates the extension. If the extension controls hardware components, they can be deactivated, resetted or stopped here.
	this->active = false;
}

void ImageStatisticsExtension::settingsLoaded(QVariantMap settings) {
	//this method is called by OCTproZ and provides a QVariantMap with stored settings/parameters.
	this->form->setSettings(settings); //update gui with stored settings
}

void ImageStatisticsExtension::storeParameters() {
	//update settingsMap, so parameters can be reloaded into gui at next start of application
	this->form->getSettings(&this->settingsMap);
	emit storeSettings(this->name, this->settingsMap);
}

void ImageStatisticsExtension::setFrameNr(int frameNr){
	this->frameNr = frameNr;
}

void ImageStatisticsExtension::setBufferNr(int bufferNr){
	this->bufferNr = bufferNr;
}

void ImageStatisticsExtension::releaseFrameBuffers(QVector<void *> buffers) {
	for (int i = 0; i < buffers.size(); i++) {
		if (buffers[i] != nullptr) {
			free(buffers[i]);
			buffers[i] = nullptr;
		}
	}
}

void ImageStatisticsExtension::rawDataReceived(void* buffer, unsigned bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->bufferSource == RAW && this->active){
		if(!this->isCalculating && this->rawGrabbingAllowed){
			this->isCalculating = true;

			//calculate size of single frame
			size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth)/8.0));
			size_t bytesPerFrame = samplesPerLine*linesPerFrame*bytesPerSample;

			//check if number of frames per buffer has changed and emit maxFrames to update gui
			if(this->framesPerBuffer != framesPerBuffer){
				emit maxFrames(framesPerBuffer-1);
				this->framesPerBuffer = framesPerBuffer;
			}
			//check if number of buffers per volume has changed and emit maxBuffers to update gui
			if(this->buffersPerVolume != buffersPerVolume){
				emit maxBuffers(buffersPerVolume-1);
				this->buffersPerVolume = buffersPerVolume;
			}

			//check if buffer size changed and allocate buffer memory
			if(this->frameBuffersRaw[0] == nullptr || this->bytesPerFrameRaw != bytesPerFrame){
				if(bitDepth == 0 || samplesPerLine == 0 || linesPerFrame == 0 || framesPerBuffer == 0){
					emit error(this->name + ":  " + tr("Invalid data dimensions!"));
					return;
				}
				//(re)create copy buffers
				if(this->frameBuffersRaw[0] != nullptr){
					this->releaseFrameBuffers(this->frameBuffersRaw);
				}
				for (int i = 0; i < this->frameBuffersRaw.size(); i++) {
					this->frameBuffersRaw[i] = static_cast<void*>(malloc(bytesPerFrame));
				}
				this->bytesPerFrameRaw = bytesPerFrame;
			}

			//copy single frame of received data and emit it for further processing
			this->copyBufferId = (this->copyBufferId+1)%NUMBER_OF_BUFFERS;
			char* frameInBuffer = static_cast<char*>(buffer);
			if(this->frameNr>static_cast<int>(framesPerBuffer-1)){this->frameNr = static_cast<int>(framesPerBuffer-1);}
			if(this->bufferNr>static_cast<int>(buffersPerVolume-1)){this->bufferNr = static_cast<int>(buffersPerVolume-1);}
			if(this->bufferNr == -1 || this->bufferNr == static_cast<int>(currentBufferNr)){
				memcpy(this->frameBuffersRaw[this->copyBufferId], &(frameInBuffer[bytesPerFrame*this->frameNr]), bytesPerFrame);
				emit newFrame(this->frameBuffersRaw[this->copyBufferId], bitDepth, samplesPerLine, linesPerFrame);
			}

			this->isCalculating = false;
		}
		else{
			this->lostBuffersRaw++;
			emit info(this->name + ": " + tr("Raw buffer lost. Total lost raw buffers: ") + QString::number(lostBuffersRaw));
			if(this->lostBuffersRaw>= INT_MAX){
				this->lostBuffersRaw = 0;
				emit info(this->name + ": " + tr("Lost raw buffer counter overflow. Counter set to zero."));
			}
		}
	}
}

void ImageStatisticsExtension::processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) {
	if(this->bufferSource == PROCESSED && this->active){
		if(!this->isCalculating && this->processedGrabbingAllowed){
			//check if current buffer is selected. If it is not selected discard it and do nothing (just return).
			if(this->bufferNr>static_cast<int>(buffersPerVolume-1)){this->bufferNr = static_cast<int>(buffersPerVolume-1);}
			if(!(this->bufferNr == -1 || this->bufferNr == static_cast<int>(currentBufferNr))){
				return;
			}

			this->isCalculating = true;

			//calculate size of single frame
			size_t bytesPerSample = static_cast<size_t>(ceil(static_cast<double>(bitDepth)/8.0));
			size_t bytesPerFrame = samplesPerLine*linesPerFrame*bytesPerSample;

			//check if number of frames per buffer has changed and emit maxFrames to update gui
			if(this->framesPerBuffer != framesPerBuffer){
				emit maxFrames(framesPerBuffer-1);
				this->framesPerBuffer = framesPerBuffer;
			}
			//check if number of buffers per volume has changed and emit maxBuffers to update gui
			if(this->buffersPerVolume != buffersPerVolume){
				emit maxBuffers(buffersPerVolume-1);
				this->buffersPerVolume = buffersPerVolume;
			}

			//check if buffer size changed and allocate buffer memory
			if(this->frameBuffersProcessed[0] == nullptr || this->bytesPerFrameProcessed != bytesPerFrame){
				if(bitDepth == 0 || samplesPerLine == 0 || linesPerFrame == 0 || framesPerBuffer == 0){
					emit error(this->name + ":  " + tr("Invalid data dimensions!"));
					return;
				}
				//(re)create copy buffers
				if(this->frameBuffersProcessed[0] != nullptr){
					this->releaseFrameBuffers(this->frameBuffersProcessed);
				}
				for (int i = 0; i < this->frameBuffersProcessed.size(); i++) {
					this->frameBuffersProcessed[i] = static_cast<void*>(malloc(bytesPerFrame));
				}
				this->bytesPerFrameProcessed = bytesPerFrame;
			}

			//copy single frame of received data and emit it for further processing
			this->copyBufferId = (this->copyBufferId+1)%NUMBER_OF_BUFFERS;
			char* frameInBuffer = static_cast<char*>(buffer);
			if(this->frameNr>static_cast<int>(framesPerBuffer-1)){this->frameNr = static_cast<int>(framesPerBuffer-1);}
			memcpy(this->frameBuffersProcessed[this->copyBufferId], &(frameInBuffer[bytesPerFrame*this->frameNr]), bytesPerFrame);
			emit newFrame(this->frameBuffersProcessed[this->copyBufferId], bitDepth, samplesPerLine, linesPerFrame);

			this->isCalculating = false;
		}
		else{
			this->lostBuffersProcessed++;
			emit info(this->name + ": " + tr("Processed buffer lost. Total lost buffers: ") + QString::number(lostBuffersProcessed));
			if(this->lostBuffersProcessed>= INT_MAX){
				this->lostBuffersProcessed = 0;
				emit info(this->name + ": " + tr("Lost processed buffer counter overflow. Counter set to zero."));
			}
		}
	}
}

