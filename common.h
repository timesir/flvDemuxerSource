#ifndef COMMON
#define COMMON

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>

#ifdef __linux
#include <stdio.h>
#include <cstring>
#endif

using std::cout;
using std::endl;
using std::string;
using std::vector;

enum MediaCodecID {
    /* video codecs */
    V_CODEC_ID_NONE,
    V_CODEC_ID_H264,
    V_CODEC_ID_H263,
    V_CODEC_ID_MPEG4,

    /* audio codecs */
    A_CODEC_ID_NONE,
    A_CODEC_ID_PCM,
    A_CODEC_ID_MP2,
    A_CODEC_ID_MP3,
    A_CODEC_ID_AAC,

};

enum MEDIA_TYPE
{
	TYPE_VIDEO,
	TYPE_AUDIO,
};

enum AudioObjectType
{
	AOT_NULL = 0,
	AOT_AAC_MAIN =1,
	AOT_AAC_LC = 2,
	AOT_AAC_SSR = 3,
	AOT_AAC_LTP = 4,
	AOT_SRC = 5,
	AOT_AAC_SCALABLE = 6,

};

enum SampleFrequencyIndex
{
	SFI_96000,
	SFI_88200,
	SFI_64000,
	SFI_48000,
	SFI_44100,
	SFI_32000,
	SFI_24000,
	SFI_22050,
	SFI_16000,
	SFI_12000,
	SFI_11025,
	SFI_8000,
	SFI_7350,
};

typedef struct
{
	char SPSData[128];
	int  SPSDataSize;
	char PPSData[128];
	int  PPSDataSize;

}SPS_PPS_t;

typedef struct MediaInfo{
    enum MediaCodecID video_codec_id;
    int video_width;
    int video_height;
    float  fps;
    int64_t video_bit_rate;                  //bps
	uint64_t time;                           //ms

    enum MediaCodecID audio_codec_id;
    int audio_channel;
    int audio_sample_rate;
	int audio_sample_size;                   //8 or 16
    int audio_bit_rate;                      //bps

	SPS_PPS_t    SPS_PPS;
	char         AacHeader[7];
}MediaInfo;

typedef struct DataPacket
{
	MEDIA_TYPE stream_type;     //0：video 1:audio
    int64_t pts;
    int64_t dts;
    int key_frame;              //-1:unknow 0:not keyframe 1:keyframe
    int data_size;
    char *data;

}DataPacket;

#endif // COMMON

