#include "flv.h"

// �����ֽڵ�С��
template <class T>
T ntole(T num)
{
	T ret = 0;
	unsigned char * p = (unsigned char*)&num;
	for (int i = 0; i < sizeof(T); i++)
		ret |= (p[i] << ((sizeof(T) - i - 1) * 8));

	return ret;
}

CFlvParser::CFlvParser():
	m_iCurrIndex(0),
    m_uiFileLen(0),
	m_bIsAdtsHeadInit(false),
	m_bIsSPSPPSInit(false)
{

}

CFlvParser::~CFlvParser() {

    if(m_pFlvFile != NULL) {
        fclose(m_pFlvFile);
        m_pFlvFile = NULL;
    }
}

bool CFlvParser::OpenFlv(FILE *file)
{
	m_pFlvFile = file;
	if (NULL == m_pFlvFile)
	{
		std::cout << "Open flv file error";
		return false;
	}

	int iRet = ParseFile();

    return true;
}

int CFlvParser::ParseFile()
{
	uint32_t curPos = 0;
	//�ļ�ָ���Ƶ�ĩβ
	fseek(m_pFlvFile, 0, SEEK_END);
	//�õ���ǰ�ļ�ָ��λ��
	m_uiFileLen = ftell(m_pFlvFile);
	//�Ƶ�ͷ��
	fseek(m_pFlvFile, 0, SEEK_SET);

	//��ȡͷ��9�ֽ�
	fread(m_Headder.Signature, 1, 3, m_pFlvFile);
	fread(&m_Headder.Version, 1, 1, m_pFlvFile);
	fread(&m_Headder.Flags, 1, 1, m_pFlvFile);
	m_Headder.DataOffset = ReadFile32();

	fseek(m_pFlvFile, 4, SEEK_CUR);
	curPos = ParseMetaTag()+13;

	//ȥ������ĸ�������־�ֽڣ�00 00 00 10
	while (curPos<m_uiFileLen-4)
	{
		uint32_t uiPreTagSize = ReadFile32();
		TagHeader tagHeader;
		FlvPacket tmpPacket;
		
		fread(&tagHeader, 1, TAG_HEAD_LEN, m_pFlvFile);

		curPos += 15;//previous tag size 4 + tag head 11,�ƶ�����Ƶ������Ƶ�����ݲ���

		int tagheader_datasize = tagHeader.DataSize[0] * 65536 + tagHeader.DataSize[1] * 256 + tagHeader.DataSize[2];
		int tagheader_timestamp = tagHeader.Timestamp[0] * 65536 + tagHeader.Timestamp[1] * 256 + tagHeader.Timestamp[2];
       
		char tagtype_str[10];
		switch (tagHeader.TagType) {
		case TAG_TYPE_AUDIO:
		{
			ParseAudioTag();//һ�ֽ�:0xaf
			//�õ�adtsͷ
			if (!m_bIsAdtsHeadInit && A_CODEC_ID_AAC == m_mediaInfo.audio_codec_id)
			{
				uint8_t aacPacketType = 0;
				fread(&aacPacketType, 1, 1, m_pFlvFile);
				if (0x00 == aacPacketType)
				{
					//��һ��auido tag���ݰ���AudioSpecificConfig����������������adtsͷ
					fread(m_szAudioSpecificConfig, 1, tagheader_datasize - 2, m_pFlvFile);
					uint8_t audio_object_type = 2;
					uint8_t sampling_frequency_index = 7;
					uint8_t channel_config = 2;

					if (0 != tagheader_datasize-2)
					{
						//audio object type:5bit
						audio_object_type = m_szAudioSpecificConfig[0] & 0xf8;
						audio_object_type >>= 3;

						//sampling frequency index:4bit
						//��3bits
						sampling_frequency_index = m_szAudioSpecificConfig[0] & 0x07;
						sampling_frequency_index <<= 1;
						//��1bit
						uint8_t tmp = m_szAudioSpecificConfig[1] & 0x80;
						tmp >>= 7;
						sampling_frequency_index |= tmp;

						//channel config:4bits
						channel_config = m_szAudioSpecificConfig[1] & 0x78;
						channel_config >>= 3;
					}
				
					//����adtsͷ(�ȵõ�ǰ��λ)
					m_mediaInfo.AacHeader[0] = 0xff;         //syncword:0xfff                          ��8bits
					m_mediaInfo.AacHeader[1] = 0xf0;         //syncword:0xfff                          ��4bits
					m_mediaInfo.AacHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
					m_mediaInfo.AacHeader[1] |= (0 << 1);    //Layer:0                                 2bits 
					m_mediaInfo.AacHeader[1] |= 1;           //protection absent:1                     1bit

					m_mediaInfo.AacHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
					m_mediaInfo.AacHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits 
					m_mediaInfo.AacHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
					m_mediaInfo.AacHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               ��1bit

					m_mediaInfo.AacHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      ��2bits
					m_mediaInfo.AacHeader[3] |= (0 << 5);                      //original��0                               1bit
					m_mediaInfo.AacHeader[3] |= (0 << 4);                      //home��0                                   1bit
					m_mediaInfo.AacHeader[3] |= (0 << 3);                      //copyright id bit��0                       1bit  
					m_mediaInfo.AacHeader[3] |= (0 << 2);                      //copyright id start��0                     1bit
					//m_mediaInfo.AacHeader[3] |= ((m_mediaInfo. & 0x1800) >> 11); //frame length��value,                  ��2bits 

				
					//m_mediaInfo.AacHeader[4] = (tmp >> 3) & 0xff;
					//m_mediaInfo.AacHeader[5] = ((tmp & 0x07) << 5 | 0x1f);
					m_mediaInfo.AacHeader[6] = 0xfc;

				}

				m_bIsAdtsHeadInit = true;
				break;
			}

			if (A_CODEC_ID_AAC == m_mediaInfo.audio_codec_id)
			{
				tmpPacket.size = tagheader_datasize - 1 - 1; //��ȥͷ��1�ֽڣ�AACPacketType
				//����ǰһ���ֽڣ�AACPacketType 
				tmpPacket.offset = ftell(m_pFlvFile) + 1;
			}
			else
			{
				tmpPacket.size = tagheader_datasize - 1;
				tmpPacket.offset = ftell(m_pFlvFile);
			}
			
			tmpPacket.dts = tagheader_timestamp;
			tmpPacket.pts = tmpPacket.dts;
			tmpPacket.stream_type = TYPE_AUDIO;

            //����data
			fseek(m_pFlvFile, tagheader_datasize - 1,SEEK_CUR);
			if (tmpPacket.size > 0)
			{
				m_vcAVFrameIndex.push_back(tmpPacket);
			}
			curPos += tagheader_datasize;
			//cout<<c++<<" Audio��"<< tagheader_datasize<<" "<<tmpPacket.offset<<endl;
		}
		break;
		case TAG_TYPE_VIDEO:
		{
			int iRet = ParseVideoTag();

			//��һ��video tag Ϊsps��pps����
			if (!m_bIsSPSPPSInit && V_CODEC_ID_H264 == m_mediaInfo.video_codec_id)
			{
				uint8_t spsCount = 0;
				uint8_t ppsCount = 0;

				//����AVCDecoderConfigurationRecord(��ISO_IEC_14496-15)�ṹǰ9���ֽ�
				fseek(m_pFlvFile, 9, SEEK_CUR);

				fread(&spsCount, 1, 1, m_pFlvFile);
				m_mediaInfo.SPS_PPS.SPSDataSize= ReadFile16();
				fread(m_mediaInfo.SPS_PPS.SPSData, m_mediaInfo.SPS_PPS.SPSDataSize, 1, m_pFlvFile);

				fread(&ppsCount, 1, 1, m_pFlvFile);
				m_mediaInfo.SPS_PPS.PPSDataSize = ReadFile16();
				fread(m_mediaInfo.SPS_PPS.PPSData, m_mediaInfo.SPS_PPS.PPSDataSize, 1, m_pFlvFile);


				for (int i = m_mediaInfo.SPS_PPS.SPSDataSize; i >= 0; i--)
				{
					m_mediaInfo.SPS_PPS.SPSData[i + 4] = m_mediaInfo.SPS_PPS.SPSData[i];
				}
				m_mediaInfo.SPS_PPS.SPSData[0] = 0x00;
				m_mediaInfo.SPS_PPS.SPSData[1] = 0x00;
				m_mediaInfo.SPS_PPS.SPSData[2] = 0x00;
				m_mediaInfo.SPS_PPS.SPSData[3] = 0x01;
				m_mediaInfo.SPS_PPS.SPSDataSize = m_mediaInfo.SPS_PPS.SPSDataSize + 4;

				for (int i = m_mediaInfo.SPS_PPS.PPSDataSize; i >= 0; i--)
				{
					m_mediaInfo.SPS_PPS.PPSData[i + 4] = m_mediaInfo.SPS_PPS.PPSData[i];
				}
				m_mediaInfo.SPS_PPS.PPSData[0] = 0x00;
				m_mediaInfo.SPS_PPS.PPSData[1] = 0x00;
				m_mediaInfo.SPS_PPS.PPSData[2] = 0x00;
				m_mediaInfo.SPS_PPS.PPSData[3] = 0x01;
				m_mediaInfo.SPS_PPS.PPSDataSize = m_mediaInfo.SPS_PPS.PPSDataSize + 4;

				curPos += tagheader_datasize;
				m_bIsSPSPPSInit = true;
				break;
			}
			
			if (V_CODEC_ID_H264 == m_mediaInfo.video_codec_id)
			{
				tmpPacket.size = tagheader_datasize - 1 -4;
				//ǰ4���ֽڣ�AVCPacketType(1),composition time(3)
				uint8_t AVCPacketType = 0;
				fread(&AVCPacketType, 1, 1, m_pFlvFile);
				uint8_t CompositionTimeTmp[3] = { 0 };
				fread(&CompositionTimeTmp, 1, 3, m_pFlvFile);
				int compostime = CompositionTimeTmp[0] * 65536 + CompositionTimeTmp[1] * 256 + CompositionTimeTmp[2];

				tmpPacket.offset = ftell(m_pFlvFile);
				tmpPacket.dts = tagheader_timestamp;
				tmpPacket.pts = tmpPacket.dts + compostime;
				fseek(m_pFlvFile, -4, SEEK_CUR);
			}
			else
			{
				tmpPacket.dts = tagheader_timestamp;
				tmpPacket.pts = tmpPacket.dts;
				tmpPacket.size = tagheader_datasize - 1;
				tmpPacket.offset = ftell(m_pFlvFile);
			}
			tmpPacket.key_frame = iRet;
			tmpPacket.stream_type = TYPE_VIDEO;
			
			fseek(m_pFlvFile, tagheader_datasize - 1, SEEK_CUR);
			m_vcAVFrameIndex.push_back(tmpPacket);

			curPos += tagheader_datasize;
		}
		break;
		default:break;
		}

	}
	//��ȡ�ؼ�֡����
	for (int i=0;i<m_vcAVFrameIndex.size();i++)
	{
		if (m_vcAVFrameIndex[i].key_frame == 1 && m_vcAVFrameIndex[i].stream_type == TYPE_VIDEO)
		{
			m_vcKeyFrameIndex.push_back(i);
		}
	}
	return 0;
}

int CFlvParser::ParseMetaTag()
{
	FlvMetaData meatdata;
	TagHeader ScrTagHeader;
	fread(&ScrTagHeader, 1, TAG_HEAD_LEN, m_pFlvFile);
	int tagheader_datasize = ScrTagHeader.DataSize[0] * 65536 + ScrTagHeader.DataSize[1] * 256 + ScrTagHeader.DataSize[2];
	int tagheader_timestamp = ScrTagHeader.Timestamp[0] * 65536 + ScrTagHeader.Timestamp[1] * 256 + ScrTagHeader.Timestamp[2];

	if (TAG_TYPE_META != ScrTagHeader.TagType)
		return 0;

	uint16_t metaname_length = 0;
	int amf1_type = 0,amf2_type = 0;
	int amf1_strsize = 0, amf2_arraysize = 0;
	char amf1_str[64] = { 0 };

	//��һ��AMF��
	fread(&amf1_type, 1, 1, m_pFlvFile);
	amf1_strsize = ReadFile16();
	fread(amf1_str, 1, amf1_strsize, m_pFlvFile);

	//�ڶ���AMF��
	fread(&metaname_length, 1, 1, m_pFlvFile);
	amf2_arraysize = ReadFile32();

	int metadata_size = tagheader_datasize - 18;
	//������������
	ParseMetaData(metadata_size);
	
	return tagheader_datasize+ TAG_HEAD_LEN;
}

int CFlvParser::ParseAudioTag()
{
	char szFirsrByte;
	fread(&szFirsrByte, 1, 1, m_pFlvFile);

	int audio_codec_id = 0;
	int audio_sample_rate = 0;
	int audio_sample_bit = 0;
	int audio_channel = 0;
	audio_codec_id = szFirsrByte & 0xf0;
	audio_codec_id >>= 4;

	//ǰ��bit�õ���������
	switch (audio_codec_id)
	{
	case FLV_A_CODECID_MP3:
		m_mediaInfo.audio_codec_id = A_CODEC_ID_MP3;
		break;
	case FLV_A_CODECID_AAC:
		m_mediaInfo.audio_codec_id = A_CODEC_ID_AAC;
		break;
	default:
		m_mediaInfo.audio_codec_id = A_CODEC_ID_AAC;
		break;
	}

	//������bit ������
	audio_sample_rate = szFirsrByte & 0x0c;
	audio_sample_rate >>= 2;
	switch (audio_sample_rate)
	{
	case 0:
		m_mediaInfo.audio_sample_rate = 5500;
		break;
	case 1:
		m_mediaInfo.audio_sample_rate = 11000;
		break;
	case 2:
		m_mediaInfo.audio_sample_rate = 22000;
		break;
	case 3:
		m_mediaInfo.audio_sample_rate = 44000;
		break;
	}

	audio_sample_bit = szFirsrByte & 0x02;
	audio_sample_bit >>= 1;
	if (0 == audio_sample_bit)
	{
		m_mediaInfo.audio_sample_size = 8;
	}
	else if(1 == audio_sample_bit)
	{
		m_mediaInfo.audio_sample_size = 16;
	}

	audio_channel = szFirsrByte & 0x01;
	if (0 == audio_channel)
	{
		m_mediaInfo.audio_channel = 1;
	}
	else
	{
		m_mediaInfo.audio_channel = 2;
	}
		
	return 1;
}


int CFlvParser::ParseVideoTag()
{
	char szFirsrByte;
	fread(&szFirsrByte, 1, 1, m_pFlvFile);

	int keyFrame = 0;
	int video_codec_id = 0;
	keyFrame = szFirsrByte & 0xf0;
	keyFrame >>= 4;
	if (FLV_FRAME_KEY == keyFrame)
	{
		keyFrame = 1;
	}
	else if (FLV_FRAME_INTER == keyFrame)
	{
		keyFrame = 0;
	}

	video_codec_id = szFirsrByte & 0x0f;
	if (FLV_V_CODECID_H263 == video_codec_id)
	{
		m_mediaInfo.video_codec_id = V_CODEC_ID_H263;
	}
	else if (FLV_V_CODECID_H264 == video_codec_id)
	{
		m_mediaInfo.video_codec_id = V_CODEC_ID_H264;
	}
	else if (FLV_V_CODECID_MPEG4 == video_codec_id)
	{
		m_mediaInfo.video_codec_id = V_CODEC_ID_MPEG4;
	}


	return keyFrame;
}

int CFlvParser::ParseMetaData(int size)
{
	uint16_t metaname_length = 0;
	char metaname[64] = {0};
	uint8_t amf_data_type = 0;
	int offset = 0;

	while (1)
	{
		metaname_length = ReadFile16();
		//��metanamer����Ϊ0����ʾ����metadata��ȡ���
		if (DATA_TYPE_NUMBER == metaname_length)
		{
			offset += 2;
			//����������������
			if (offset<size)
			{
				fseek(m_pFlvFile, size - offset, SEEK_CUR);
				break;
			}
		}
		fread(metaname, 1, metaname_length, m_pFlvFile);
		metaname[metaname_length] = '\0';
		fread(&amf_data_type, 1, 1, m_pFlvFile);
		offset = offset + 2 + metaname_length + 1;

		std::cout << std::string(metaname)<< std::endl;
		//double����
		if (0 == amf_data_type)
		{
			if (std::string(metaname) == "duration"){
				m_metaData.duration = ReadDouble();             
			}
			else if (std::string(metaname) == "width"){
				m_metaData.width = ReadDouble();                  //width
				m_mediaInfo.video_width = m_metaData.width;
			}
			else if (std::string(metaname) == "height"){
				m_metaData.height = ReadDouble();                 //height
				m_mediaInfo.video_height = m_metaData.height;
			}
			else if (std::string(metaname) == "videodatarate"){
				m_metaData.videodatarate = ReadDouble();          //videodatarate
				m_mediaInfo.video_bit_rate = m_metaData.videodatarate;
			}
			else if (std::string(metaname) == "videocodecid"){
				m_metaData.videocodecid = ReadDouble();           //videocodeid
			}
			else if (std::string(metaname) == "framerate"){
				m_metaData.framerate = ReadDouble();              //framerate
				m_mediaInfo.fps = m_metaData.framerate;
			}
			else if (std::string(metaname) == "audiodatarate"){
				m_metaData.audiodatarate = ReadDouble();          //audiodatarate
				m_mediaInfo.audio_bit_rate = m_metaData.audiodatarate;
			}
			else if (std::string(metaname) == "audiosamplerate"){
				m_metaData.audiosamplerate = ReadDouble();        //audiosamplerate
			}
			else if (std::string(metaname) == "audiosamplesize"){
				m_metaData.audiosamplesize = ReadDouble();        //audiosamplesize
			}
			else if (std::string(metaname) == "audiocodecid"){
				m_metaData.audiocodecid = ReadDouble();           //audiocodecid
			}
			else if (std::string(metaname) == "filesize"){
				m_metaData.filesize = ReadDouble();                 //file size or encoder
			}else {
				fseek(m_pFlvFile, 8, SEEK_CUR);            //����ֱ������
			}

			offset += 8;

		}else if (DATA_TYPE_BOOL == amf_data_type) //boolean
		{
			if (std::string(metaname) == "stereo")
			{
				uint8_t stereo;
				fread(&stereo, 1, 1, m_pFlvFile);
				m_metaData.stereo = stereo;
			}
			else
			{
				fseek(m_pFlvFile, 1, SEEK_CUR);
			}
			offset += 1;
			
		}
		else if (DATA_TYPE_STRING == amf_data_type)   //string����
		{
			if (std::string(metaname) == "encoder")
			{
				int tmpSize = ReadFile16();
				char encoderName[64] = { 0 };
				fread(encoderName, 1, tmpSize, m_pFlvFile);
				m_metaData.encodername = encoderName;
				offset = offset + 2 + tmpSize;
			}
			else
			{
				int tmpSize = ReadFile16();
				char strName[64] = { 0 };
				fread(strName, 1, tmpSize, m_pFlvFile);
				offset = offset + 2 + tmpSize;
			}
			
		}
		
	}
	

	return 0;
}

/******************************************************
* ���ܣ���ȡ����Ƶ֡
* ���룺��Ƶ֡�洢���壬֡������Ĭ��-1,��ʾ�ӵ�ǰ�������������¶�ȡ��
* ���أ���Ƶ֡��С
/******************************************************/
uint32_t CFlvParser::ReadAvFrame(DataPacket &packet, int64_t index)
{
	std::vector<FlvPacket>& vcTmpIndex = m_vcAVFrameIndex;
	int64_t tmpIndex = 0;
	if (index == -1)
	{
		if (m_iCurrIndex < vcTmpIndex.size())
			tmpIndex = m_iCurrIndex++;
		else
			return -1;
	}
	else
	{
		m_iCurrIndex = index;
		if (index < vcTmpIndex.size())
			tmpIndex = index;
		else
			return -1;
	}

	FlvPacket flvPacket = m_vcAVFrameIndex[tmpIndex];
	packet.stream_type = flvPacket.stream_type;
	packet.data_size = flvPacket.size;
	packet.key_frame = flvPacket.key_frame;
	packet.pts = flvPacket.pts;
	packet.dts = flvPacket.dts;

	char szTmpBuf[1024 * 512] = {0};
	if (TYPE_VIDEO == packet.stream_type)
	{
		
		int64_t ret = fseek(m_pFlvFile, flvPacket.offset, SEEK_SET);
		int iRet = fread(szTmpBuf, 1, flvPacket.size, m_pFlvFile);

		if (V_CODEC_ID_H264 == m_mediaInfo.video_codec_id)
		{
			//h264������Ƶ����ͷ�ĸ��ֽ�Ϊ��nalu���ȣ������滻Ϊ��׼�ָ���
			szTmpBuf[0] = 0x00;
			szTmpBuf[1] = 0x00;
			szTmpBuf[2] = 0x00;
			szTmpBuf[3] = 0x01;
		}
		
	}
	else if (TYPE_AUDIO == packet.stream_type)
	{
		
		int64_t ret = fseek(m_pFlvFile, flvPacket.offset, SEEK_SET);
		int iRet = fread(szTmpBuf, 1, flvPacket.size, m_pFlvFile);

		if (A_CODEC_ID_AAC == m_mediaInfo.audio_codec_id)
		{
			//�õ�aacͷ��
			int tmpLen = packet.data_size + 7;
			m_mediaInfo.AacHeader[3] |= ((tmpLen & 0x1800) >> 11);           //frame length��tmpLen      ��2bits
			m_mediaInfo.AacHeader[4] = (uint8_t)((tmpLen & 0x7f8) >> 3);     //frame length��tmpLen      �м�8bits 
			m_mediaInfo.AacHeader[5] = (uint8_t)((tmpLen & 0x7) << 5);       //frame length��tmpLen      ��3bits  
			m_mediaInfo.AacHeader[5] |= 0x1f;                                //buffer fullness��0x7ff    ��5bits

			for (int j = flvPacket.size; j >= 0; j--)
			{
				szTmpBuf[j + 7] = szTmpBuf[j];
			}
			for (int i=0;i<7;i++)
			{
				szTmpBuf[i] = m_mediaInfo.AacHeader[i];
			}
			packet.data_size += 7;
		}
		

	}
	
	packet.data = szTmpBuf;
	return packet.data_size;

}


void CFlvParser::GetMediaInfo(MediaInfo &media)
{
	media = m_mediaInfo;
}

/******************************************
 *���ļ�����С�˷�ʽ��ȡ4�ֽ�����
/******************************************/
uint32_t CFlvParser::ReadFile32()
{
	uint32_t tmp = 0;
	int ret = fread(&tmp, 1, 4, m_pFlvFile);
	return ntole(tmp);
}

uint16_t CFlvParser::ReadFile16()
{
	uint16_t tmp = 0;
	int ret = fread(&tmp, 1, 2, m_pFlvFile);
	return ntole(tmp);
}

double CFlvParser::ReadDouble()
{
	uint8_t tmp[8] = { 0 };
	double d = 0;
	for (int i = 7; i >= 0;i--)
	    fread(&(tmp[i]), 1, 1, m_pFlvFile);
	memcpy(&d, tmp, sizeof(double));
	return d;
}
