#ifndef PARSEAPI_H
#define PARSEAPI_H

#include "flv.h"

enum VideoType {
	TYPE_MP4 = 0,
	TYPE_AVI,
	TYPE_FLV,
};

//接口
int DEMUX_OpenFile(const char *name,MediaInfo &info);
int DEMUX_ReadFrame(DataPacket &packet);
int DEMUX_CloseFile();


#endif // PARSEAPI_H
