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

#ifndef BITDEPTHCONVERTER_H
#define BITDEPTHCONVERTER_H

#include <QObject>

class BitDepthConverter : public QObject
{
	Q_OBJECT
public:
	explicit BitDepthConverter(QObject *parent = nullptr);
	~BitDepthConverter();

signals:
	void converted8bitData(uchar *output8bitData, unsigned int samplesPerLine, unsigned int linesPerFrame);
	void info(QString);
	void error(QString);

public slots:
	void slot_convertDataTo8bit(void *inputData, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame);

private:
	uchar* output8bitData;
	unsigned int bitDepth;
	unsigned int length;
	bool conversionRunning;
};
#endif // BITDEPTHCONVERTER_H
