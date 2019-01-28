#pragma once

#define		ES_BUFFER		600000					/* ES ���ݴ�С */
#define		TS_PACKET_LEN	188						/* TS ���ݰ����� */

/* pts���������� */
#ifndef AV_RB16
#   define AV_RB16(x)                        \
	((((const unsigned char*)(x))[0] << 8) | \
	((const unsigned char*)(x))[1])
#endif

/* ��Ƶ�������Ͷ��� */
typedef enum TS_VideoEncodeType_E
{
	TS_VIDEO_ENCODE_TYPE_INVALID = -1,
	TS_VIDEO_ENCODE_TYPE_H264
}TS_VideoEncodeType_E;

/* ��Ƶ�������Ͷ��� */
typedef enum TS_AudioEncodeType_E
{
	TS_AUDIO_ENCODE_TYPE_INVALID = -1,
	TS_AUDIO_ENCODE_TYPE_PCMA,						//g711
	TS_AUDIO_ENCODE_TYPE_AAC						//aac
}TS_AudioEncodeType_E;

/* ES���ݵ����Ͷ��� */
typedef enum TS_ESType_E
{
	TS_ES_TYPE_INVALID = -1,
	TS_ES_TYPE_VIDEO,
	TS_ES_TYPE_AUDIO
}TS_ESType_E;

/* TS���ݰ����Ͷ��� */
typedef enum TS_TSPacketType_E
{
	TS_TS_PACKET_TYPE_INVALID = -1,
	TS_TS_PACKET_TYPE_PAT,
	TS_TS_PACKET_TYPE_PMT,
	TS_TS_PACKET_TYPE_VIDEO,
	TS_TS_PACKET_TYPE_AUDIO
}TS_TSPacketType_E;

/* ES����֡���Ͷ��� */
typedef enum TS_ESFrameType_E
{
	TS_ES_FRAME_TYPE_INVALID = -1,
	TS_ES_FRAME_TYPE_DATA = 2,
	TS_ES_FRAME_TYPE_IDR = 5,
	TS_ES_FRAME_TYPE_SEI = 6,
	TS_ES_FRAME_TYPE_SPS = 7,
	TS_ES_FRAME_TYPE_PPS = 8,
}TS_ESFrameType_E;

/* ES�ص�����֮�е�ES����֮�е���Ƶ�������� */
typedef struct TS_ESVideoParam_S
{
	TS_VideoEncodeType_E video_encode_type;			//��Ƶ��������
	bool is_i_frame;								//�Ƿ�Ϊ�ؼ�֡
	__int64 pts;									//pts
	__int64 dts;									//dts
	TS_ESFrameType_E frame_type;					//֡����

	TS_ESVideoParam_S()
	{
		video_encode_type = TS_VIDEO_ENCODE_TYPE_INVALID;
		is_i_frame = false;
		pts = 0;
		dts = 0;
		frame_type = TS_ES_FRAME_TYPE_INVALID;
	}
}TS_ESVideoParam_S;

/* ES�ص�����֮�е�ES����֮�е���Ƶ�������� */
typedef struct TS_ESAudioParam_S
{
	TS_AudioEncodeType_E audio_encode_type;			//��Ƶ��������
	int channels;									//ͨ����
	int samples_rate;								//��Ƶ������
	__int64 pts;									//pts

	TS_ESAudioParam_S()
	{
		audio_encode_type = TS_AUDIO_ENCODE_TYPE_INVALID;
		channels = 1;								//g711Ĭ������
		samples_rate = 8000;	
		pts = 0;
	}
}TS_ESAudioParam_S;

/* ES�ص�֮�е�ES�������� */
typedef struct TS_ESParam_S
{
	TS_ESType_E es_type;							//es��������
	TS_ESVideoParam_S video_param;					//��Ƶ����, ��es��������Ϊ��Ƶʱ��Ч
	TS_ESAudioParam_S audio_param;					//��Ƶ����, ��es��������Ϊ��Ƶʱ��Ч

	TS_ESParam_S()
	{
		es_type = TS_ES_TYPE_INVALID;
	}
}TS_ESParam_S;

/**
* @brief ES���ݻص�����
* @param[in] es_data	 ES��������
* @param[in] es_data_len ES�������ݳ���
* @param[in] es_param	 ES���ݲ���
* @param[in] user_param	 �û�����
*
* @return void
* @note ��ǰδʵ��
*/
typedef void (__stdcall *es_callback)(unsigned char* es_data, int es_data_len, TS_ESParam_S es_param, void* user_param);
