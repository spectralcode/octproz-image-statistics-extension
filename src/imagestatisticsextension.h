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

#ifndef DEMOEXTENSION_H
#define DEMOEXTENSION_H

#define NUMBER_OF_BUFFERS 2

#include <QCoreApplication>
#include <QThread>
#include "octproz_devkit.h"
#include "imagestatisticsextensionform.h"
#include "imagestatisticscalculator.h"
#include "roiselector.h"


class ImageStatisticsExtension : public Extension
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID Extension_iid)
	Q_INTERFACES(Extension)
	QThread statisticsCalculatorThread;

public:
	ImageStatisticsExtension();
	~ImageStatisticsExtension();

	virtual QWidget* getWidget() override;
	virtual void activateExtension() override;
	virtual void deactivateExtension() override;
	virtual void settingsLoaded(QVariantMap settings) override;


private:
	ImageStatisticsCalculator* statisticsCalculator;
	ROISelector* roiSelect;
	QVector<void*> frameBuffersRaw;
	QVector<void*> frameBuffersProcessed;
	int copyBufferId;
	size_t bytesPerFrameRaw;
	size_t bytesPerFrameProcessed;

	ImageStatisticsExtensionForm* form;
	bool widgetDisplayed;
	bool isCalculating;
	bool active;

	int lostBuffersRaw;
	int lostBuffersProcessed;
	BUFFER_SOURCE bufferSource;
	int frameNr;
	int bufferNr;
	unsigned int framesPerBuffer;
	unsigned int buffersPerVolume;

	void releaseFrameBuffers(QVector<void*> buffers);

public slots:
	void storeParameters();
	void setBufferSource(BUFFER_SOURCE src){this->bufferSource = src;}
	void setFrameNr(int frameNr);
	void setBufferNr(int bufferNr);
	virtual void rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;
	virtual void processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;

signals:
	void newFrame(void* frame, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void maxFrames(int max);
	void maxBuffers(int max);
};

#endif // DEMOEXTENSION_H
