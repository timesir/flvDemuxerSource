#ifndef FLV_H
#define FLV_H

#include"common.h"

#define FLV_HEAD_LEN 9
#define TAG_HEAD_LEN 11


enum TagType {
	TAG_TYPE_AUDIO = 0x08,
	TAG_TYPE_VIDEO = 0x09,
	TAG_TYPE_META = 0x12,
};

enum {
	FLV_V_CODECID_H263 = 2,
	FLV_V_CODECID_SCREEN = 3,
	FLV_V_CODECID_VP6 = 4,
	FLV_V_CODECID_VP6A = 5,
	FLV_V_CODECID_SCREEN2 = 6,
	FLV_V_CODECID_H264 = 7,
	FLV_V_CODECID_REALH263 = 8,
	FLV_V_CODECID_MPEG4 = 9,
};

enum {
	FLV_A_CODECID_PCM = 0,
	FLV_A_CODECID_ADPCM = 1,
	FLV_A_CODECID_MP3,
	FLV_A_CODECID_PCM_LE,
	FLV_A_CODECID_NELLYMOSER_16KHZ_MONO,
	FLV_A_CODECID_NELLYMOSER_8KHZ_MONO,
	FLV_A_CODECID_NELLYMOSER,
	FLV_A_CODECID_PCM_ALAW,
	FLV_A_CODECID_PCM_MULAW,
	FLV_A_CODECID_AAC = 10,
	FLV_A_CODECID_SPEEX,
};
enum {
	FLV_FRAME_KEY = 1, 
	FLV_FRAME_INTER = 2 , 
	FLV_FRAME_DISP_INTER = 3, 
	FLV_FRAME_GENERATED_KEY = 4,
	FLV_FRAME_VIDEO_INFO_CMD = 5, 
};

typedef struct {
	uint8_t Signature[3];
	uint8_t Version;
	uint8_t Flags;
	uint32_t DataOffset;
} FlvHeader;

typedef struct {
	uint8_t TagType;
	uint8_t DataSize[3];
	uint8_t Timestamp[3];
	uint32_t Reserved;
} TagHeader;

typedef struct  {
    double duration;
    double width;
    double height;
    double framerate;
    double videodatarate;
    double audiodatarate;
    double videocodecid;
    double audiocodecid;
    double audiosamplerate;
    double audiosamplesize;
	double filesize;
    bool   stereo;
	char   *encodername;
}FlvMetaData;

typedef enum {
	DATA_TYPE_NUMBER = 0x00,
	DATA_TYPE_BOOL = 0x01,
	DATA_TYPE_STRING = 0x02,
	DATA_TYPE_OBJECT = 0x03,
	DATA_TYPE_NULL = 0x05,
	DATA_TYPE_UNDEFINED = 0x06,
	DATA_TYPE_REFERENCE = 0x07,
	DATA_TYPE_MIXEDARRAY = 0x08,
	DATA_TYPE_OBJECT_END = 0x09,
	DATA_TYPE_ARRAY = 0x0a,
	DATA_TYPE_DATE = 0x0b,
	DATA_TYPE_LONG_STRING = 0x0c,
	DATA_TYPE_UNSUPPORTED = 0x0d,
} FlvAMFDataType;

typedef struct FlvPacket
{
	MEDIA_TYPE   stream_type;           //0£ºvideo 1:audio
	uint64_t size;
	uint64_t offset;;
	uint32_t key_frame;
	uint64_t dts;
	uint64_t pts;

}FlvPacket;

class CFlvParser {

public:
    CFlvParser();
    ~CFlvParser();

public:
	bool OpenFlv(FILE *file);
	void GetMediaInfo(MediaInfo &media);
	uint32_t ReadAvFrame(DataPacket &packet, int64_t index);
	
    int ParseMetaTag();
	int ParseMetaData(int size);
	int ParseAudioTag();
	int ParseVideoTag();
	
private:
	int ParseFile();
	double ReadDouble();
	uint32_t ReadFile32();
	uint16_t ReadFile16();
private:
    std::string m_file;
    FILE *m_pFlvFile;
	int64_t m_iCurrIndex;
	FlvHeader m_Headder;
	FlvMetaData m_metaData;
	uint32_t m_uiFileLen;
	MediaInfo m_mediaInfo;
	bool m_bIsSPSPPSInit;
	bool m_bIsAdtsHeadInit;
	char m_szAudioSpecificConfig[64];
	std::vector<FlvPacket> m_vcAVFrameIndex;
	std::vector<int> m_vcKeyFrameIndex;
};

#endif // FLV

