/* ��׼��ͷ�ļ� */
#include <string>
/* ˽��ͷ�ļ� */
#include "ts_struct.h"
#include "ts_demux.h"

CParseTS::CParseTS():
es_cb_(NULL),
es_cb_user_param_(NULL),
es_audio_param_get_(false),
ts_pmt_pid_(-1),
ts_video_pid_(-1),
ts_audio_pid_(-1),
es_video_data_index_(0),
es_audio_data_index_(0),
es_video_data_(NULL),
es_audio_data_(NULL)
{
	return;
}

CParseTS::~CParseTS()
{
	if(NULL != es_video_data_)
	{
		free(es_video_data_);
		es_video_data_ = NULL;
	}

	if(NULL != es_audio_data_)
	{
		free(es_audio_data_);
		es_audio_data_ = NULL;
	}

	return;
}

/* ��ʼ������TS�� */
int CParseTS::init_parse()
{
	es_video_data_ = (unsigned char*)malloc(ES_BUFFER);
	if(NULL == es_video_data_)
	{
		printf("failed to malloc video es cache memory!");
		return -1;
	}

	es_audio_data_ = (unsigned char*)malloc(ES_BUFFER);
	if(NULL == es_audio_data_)
	{
		printf("failed to malloc audio es cache memory!");
		return -1;
	}

	return 0;
}

/* ���ý���es�������ݵ����ݻص����� */
int CParseTS::set_es_callback(es_callback es_cb, void* user_param)
{
	es_cb_ = es_cb;
	es_cb_user_param_ = user_param;

	return 0;
}

/* ���TS���ݣ����ݳ��ȱ�����TSͷ��ʼ������Ϊts packet������ */
void CParseTS::put_pkt_data(unsigned char* pkt_data, int pkt_data_len)
{
	if(NULL == pkt_data || TS_PACKET_LEN > pkt_data_len)
	{	
		printf("invalid input param!");
		return;
	}

	/* ��ȡһ��ts packet */
	unsigned char* ts_data_tmp = pkt_data;
	pkt_data += TS_PACKET_LEN;
	pkt_data_len -= TS_PACKET_LEN;

	int errcode = 0;

	/* ����һ��������ts packet */
	while(pkt_data_len >= 0)	
	{
		TS_TSPacketType_E ts_type;
		int ts_head_offset = 0;		//tsͷ����

		/* ��ts packet֮�н���ts packet type, tsͷ���� */
		errcode = parse_a_ts_packet(ts_data_tmp, ts_type, ts_head_offset);
		if(0 != errcode)
		{
			/* ����ʧ�����ȡ��һ��ts packet���н��� */
			ts_data_tmp = pkt_data;
			pkt_data += TS_PACKET_LEN;
			pkt_data_len -= TS_PACKET_LEN;
			continue;
		}

		/* ��ǰts packet����Ϊpat ���ݣ� ���ǵ�һ�λ�ȡ�������ݣ������pmt pid */
		if(TS_TS_PACKET_TYPE_PAT == ts_type && -1 == ts_pmt_pid_)
		{
			/* ƫ��������pat data��ʼ */
			unsigned char* pat_data = ts_data_tmp + ts_head_offset;

			/* �жϵ�ǰpat���Ƿ���ã��������������һ�� ts packet
			*  
			* pat data ֮�е� 
			* table id ӦΪ 0x00 
			* current_next_indicator ��ʾ��ǰpat���Ƿ���ã�������ֶ�Ϊ0��ǰpat����Ч 
			*
			*/
			if((0x00 != pat_data[0]) || (0 == (pat_data[5]&0x01)))	
			{
				ts_data_tmp = pkt_data;
				pkt_data += TS_PACKET_LEN;
				pkt_data_len -= TS_PACKET_LEN;
				continue;
			}

			/* ����section_length��section_length��ȥpat��Ŀ��֮����ֽ���Ϊpat��Ŀ���ֽ���������ÿ����Ŀ��ĳ���4�ֽ�Ϊ��Ŀ������� */
			char byte2_len_sz = pat_data[1]&0x0F;
			int byte2_len_i = (int)(byte2_len_sz<<8);
			int byte3_len_i = (int)pat_data[2];
			int cycle_count = (byte2_len_i+byte3_len_i-9)/4;		

			/* ���ҽ�Ŀ��֮�еĽ�Ŀ���program_number�ֶ�Ϊ 0x0001�Ľ�Ŀ����Ϊpmt�����Ŀid��pid */
			for(int i = 0; i != cycle_count; i++)
			{
				int index = 8 + 4*i;
				if(pat_data[index] == 0x00 && pat_data[index+1] == 0x01)
				{
					/* ����pmt pid */
					char byte_high_sz = pat_data[index+2]&0x1F;
					int byte_higt_i = (int)(byte_high_sz<<8);
					int byte_low_i = (int)pat_data[index+3];

					ts_pmt_pid_ = byte_higt_i + byte_low_i;

					break;
				}
			}
		}
		/* ��ǰts packetΪpmt���ݣ��� video_id �� audio_id δ������ȫ������� pmt data֮�е� video_id �� audio_id */
		else if(TS_TS_PACKET_TYPE_PMT == ts_type && (-1 == ts_video_pid_ || -1 == ts_audio_pid_))
		{
			/* ƫ����pmt data��ʼ */
			unsigned char* pmt_data = ts_data_tmp + ts_head_offset;

			/* �жϵ�ǰpmt���Ƿ���ã��������������һ�� ts packet
			*  
			* pmt data ֮�е� 
			* table id ӦΪ 0x02 
			* current_next_indicator ��ʾ��ǰpmt���Ƿ���ã�������ֶ�Ϊ0��ǰpmt����Ч 
			*
			*/
			if((0x02 != pmt_data[0]) || (0 == (pmt_data[5]&0x01)))	
			{
				ts_data_tmp = pkt_data;
				pkt_data += TS_PACKET_LEN;
				pkt_data_len -= TS_PACKET_LEN;
				continue;
			}

			/* ����section_length��section_length��ȥpmt��Ŀ��֮����ֽ���Ϊpmt��Ŀ���ֽ���, Ȼ�󰤸�����pmt��Ŀ���ҵ�video��Ŀ����audio��Ŀ�������������ͺ�pid */
			char byte2_len_sz = pmt_data[1]&0x0F;
			int byte2_len_i = (int)(byte2_len_sz<<8);
			int byte3_len_i = (int)pmt_data[2];
			int cycle_len = byte2_len_i+byte3_len_i-13;		
			int index = 12;
			while(cycle_len > 0)
			{
				/* ��Ŀ�� stream_type == 0x1B ˵����ǰ��Ŀ��Ϊ H264 �������Ƶ��Ŀ�� */
				if(pmt_data[index] == 0x1B)	
				{
					/* ����video pid������ encode_type */
					char byte_high_sz = pmt_data[index+1]&0x1F;
					int byte_higt_i = (int)(byte_high_sz<<8);
					int byte_low_i = (int)pmt_data[index+2];

					ts_video_pid_ = byte_higt_i + byte_low_i;

					es_param_.video_param.video_encode_type = TS_VIDEO_ENCODE_TYPE_H264;
				}
				/* ��Ŀ�� stream_type == 0x0F ˵����ǰ��Ŀ��Ϊ AAC �������Ƶ��Ŀ�� */
				if(pmt_data[index] == 0x0F)	
				{
					/* ����audio pid������ encode_type */
					char byte_high_sz = pmt_data[index+1]&0x1F;
					int byte_higt_i = (int)(byte_high_sz<<8);
					int byte_low_i = (int)pmt_data[index+2];

					ts_audio_pid_ = byte_higt_i + byte_low_i;

					es_param_.audio_param.audio_encode_type = TS_AUDIO_ENCODE_TYPE_AAC;		
				}
				/* ��Ŀ�� stream_type == 0x90 ˵����ǰ��Ŀ��Ϊ G711 �������Ƶ��Ŀ�� */
				if(pmt_data[index] == 0x90)	//g711������Ƶ
				{
					/* ����audio pid������ encode_type */
					char byte_high_sz = pmt_data[index+1]&0x1F;
					int byte_higt_i = (int)(byte_high_sz<<8);
					int byte_low_i = (int)pmt_data[index+2];

					ts_audio_pid_ = byte_higt_i + byte_low_i;

					es_param_.audio_param.audio_encode_type = TS_AUDIO_ENCODE_TYPE_PCMA;	//g711
				}

				/* ������ES_info_length�ֶΣ����ݸ��ֶμ�����һ����Ŀ��λ�� */
				int byte_high = pmt_data[index+3]&0x0F;
				int byte_low = pmt_data[index+4]&0xFF;
				int es_info_len = byte_high + byte_low;
				index += 4 + es_info_len + 1;
				cycle_len = cycle_len - es_info_len - 4 - 1;
			}
		}
		/* ��ǰts packet����Ϊ video pes���� */
		else if(TS_TS_PACKET_TYPE_VIDEO == ts_type)	
		{
			/* ���� ts ͷƫ�Ƶ� ��������λ�� */
			unsigned char* pes_data = ts_data_tmp + ts_head_offset;
			unsigned int pes_data_len = TS_PACKET_LEN - ts_head_offset;

			/* �ҵ�pes���ݰ�֮�е�es������λ�� */
			unsigned char* es_data = get_es_pos(pes_data);
			unsigned int es_length = get_es_length(pes_data, pes_data_len);

			/* �����ǰ���ز�����pesͷ��ʼ������es ���ݲ��Ǵ�nalu��ʼ�򽫸������ݿ�������Ƶ����
			*
			* ��������˵ts��ÿһ�� pes ��������һ֡����, pes�����ȳ���pes�������ֶ��ܱ�ʾ�ĳ��ȷ�Χ����0��䣬һ��ΪI֡; �����ĺ�������
			*/
			if(false == is_pes_begin(pes_data) || false == is_startcode_begin(es_data))
			{
				cpy_data_to_video_es_memory(es_data, es_length);
			}
			/* �����ȴ���ES���ݻص�������ES���������ݻص����ٻ������ݸ��� */
			else
			{
				unsigned char* es_all_v = es_video_data_;
				int es_all_len = es_video_data_index_;

				do
				{
					/* �жϵ�ǰ����֮�е������ǲ�����nalu��ʼ������������ */
					int remain_data_len_f = es_all_len;
					unsigned char* nalu_head_first = find_nalu_startcode(es_all_v, remain_data_len_f);
					/* ����������������������ǰ���ݣ���ջ��� */
					if(NULL == nalu_head_first)
					{
						break;
					}

					while(5 < remain_data_len_f)	//nalu head len
					{
						/* ���˵������ĵ����ݣ� �˴����˵��ָ���֡*/
						if(0x09 == nalu_head_first[4])	//����������0x0000000109�ָ���
						{
							nalu_head_first += 4;
							remain_data_len_f -= 4;
							nalu_head_first = find_nalu_startcode(nalu_head_first, remain_data_len_f);
							if(NULL == nalu_head_first)
							{
								break;
							}
						}

						/* �жϻ���֮���Ƿ�Ϊ����һ��nalu������ */
						int remain_data_len_s = remain_data_len_f-4;
						unsigned char* nalu_head_second = find_nalu_startcode(nalu_head_first+4, remain_data_len_s);
						/* ���� */
						if(NULL == nalu_head_second)
						{
							/* ������Ƶ���ص� */
							es_param_.video_param.is_i_frame = es_is_i_frame((const unsigned char*)nalu_head_first);
							es_param_.video_param.frame_type = get_video_frame_type(nalu_head_first);
							es_param_.es_type = TS_ES_TYPE_VIDEO;
							if(NULL != es_cb_)
								es_cb_((unsigned char*)nalu_head_first, remain_data_len_f, es_param_, es_cb_user_param_);

							break;
						}
						/* �� */
						else
						{
							/* ������Ƶ���ص� */
							es_param_.video_param.is_i_frame = es_is_i_frame((const unsigned char*)nalu_head_first);
							es_param_.video_param.frame_type = get_video_frame_type(nalu_head_first);
							es_param_.es_type = TS_ES_TYPE_VIDEO;

							int touch_data_len = remain_data_len_f - remain_data_len_s;
							if(NULL != es_cb_)
								es_cb_((unsigned char*)nalu_head_first, touch_data_len, es_param_, es_cb_user_param_);

							/* ����������һ��nalu�����лص� */
							nalu_head_first = nalu_head_second;
							remain_data_len_f = remain_data_len_s;
						}

					}
				}while(false);

				/* �����ƵES���� */
				memset(es_video_data_, 0, ES_BUFFER);
				es_video_data_index_ = 0;
				es_param_.video_param.is_i_frame = false;

				/* ������һ֡���ݵ�pts��dts*/
				es_param_.video_param.pts = get_es_pts(pes_data);		//��ȡpts
				es_param_.video_param.dts = get_es_dts(pes_data);

				/* ������һ��ts���ĸ��� */
				cpy_data_to_video_es_memory(es_data, es_length);
			}
		}
		/* ��ǰts packet����Ϊ audio pes���� */
		else if(TS_TS_PACKET_TYPE_AUDIO == ts_type)	
		{
			/* ����ts����λ�� */
			unsigned char* pes_data = ts_data_tmp + ts_head_offset;
			unsigned int pes_data_len = TS_PACKET_LEN - ts_head_offset;

			/* ts��������Ƶ�����ʺ�ͨ����û�и��µ�ʱ���ȸ��²����ʺ�ͨ���� */
			if(!es_audio_param_get_)
			{
				if(update_es_audio_common_param(pes_data, pes_data_len))
				{
					/* ����ʧ����ֱ�ӽ�����һ��ts packet */
					ts_data_tmp = pkt_data;
					pkt_data += TS_PACKET_LEN;
					pkt_data_len -= TS_PACKET_LEN;
					continue;
				}
				es_audio_param_get_ = true;
				continue;
			}

			/* ��������λ�� */
			unsigned char* es_data = get_es_pos(pes_data);
			unsigned int es_length = get_es_length(pes_data, pes_data_len);

			/* һ��pes��Ϊһ֡��Ƶ֡���������pes header����Ƶ����֮��Ϊһ�������� ��Ƶ֡�򴥷�ES�ص�����֮��������ݻ��� */
			if(!is_pes_begin(pes_data))
			{
				cpy_data_to_audio_es_memory(es_data, es_length);
			}
			else
			{
				/* �����ص� */
				es_param_.es_type = TS_ES_TYPE_AUDIO;
				if (es_cb_)
					es_cb_((unsigned char*)es_audio_data_, es_audio_data_index_, es_param_, es_cb_user_param_);

				/* ��ջ��� */
				memset(es_audio_data_, 0, ES_BUFFER);
				es_audio_data_index_ = 0;

				/* ������һ֡���ݵ�pts���������µ�һ֡���� */
				es_param_.audio_param.pts = get_es_pts(pes_data);		//��ȡpts
				cpy_data_to_audio_es_memory(es_data, es_length);
			}
		}
		else
		{
			/* unknow ts format */
		}

		/* ���������ŵ���һ��ts packet */
		ts_data_tmp = pkt_data;
		pkt_data += TS_PACKET_LEN;
		pkt_data_len -= TS_PACKET_LEN;
	}

	return ;
}

/**
* @brief ����һ��������ts���ݰ�
*
* @param[in] ts_data ts����
* @param[out] ts_type ts���ݽ������
* @param[out] ts_head_offset ts����ͷ����
*
*/
int CParseTS::parse_a_ts_packet(unsigned char* ts_data, TS_TSPacketType_E &ts_type, int &ts_head_offset)
{
	/* У��TS��ͷ */
	if(0x47 != ts_data[0])		
	{
		return -1;
	}

	/* ���� adaptation_field_control �ֶεĸ��ر�ʶ���жϵ�ǰTS���Ƿ��и��أ���û�к��и��ص�TS���ݰ����й��� */
	if(0 == (ts_data[3]&0x10))	
	{
		return -1;
	}

	/* ����TS��ͷ(��������Խӽ������ӣ����������£�����Τ�����̵�TS����û������)
	*
	*  ���� adaptation_field_control �ֶ�֮�е�����Ӧ��ʶ���жϵ�ǰTS���Ƿ�������Ӧ�ֶΣ����������Ӧ�ֶγ����������Ӧ�ֶμ���TS��ͷ����֮�У�
	*���������Ӧ�ֶΣ���adaptation_field_control�ֶκ������һ���ֽڱ�ʶ�ڸ��ֽں������Ӧ�ֶεĳ����Ƕ��١�
	*
	*  adaptation_field_control �ֶΣ�TS 4�ֽڰ�ͷ֮�е����һ���ֽڵĸ���λ
	*  ��00����������01��Ϊ������Ӧ�򣬽�����Ч���أ���10��Ϊ��������Ӧ������Ч���أ���11��Ϊͬʱ��������Ӧ�����Ч����
	*/
	ts_head_offset = 4;			
	if(0 != (ts_data[3]&0x20))
	{
		ts_head_offset += 1;	
		int adaptation_field_length = (int)ts_data[4];		
		ts_head_offset += adaptation_field_length;
	}

	/* ���´������TS��ͷ֮�е�PID�ֶ�(TS��ͷ֮�еڶ����ֽڵĵ�5λ��TS��ͷ֮�е������ֽ�)���жϵ�ǰTS���ݰ������� */
	
	/* ��� PID == 0x00 ��ǰTS������ΪPAT */
	if(0 == (ts_data[1]&0x1F) && 0 == (ts_data[2]&0xFF))
	{
		ts_type = TS_TS_PACKET_TYPE_PAT;
		/* �����ǰ���ݰ���PAT/PMT����TS���صĿ�ʼǰ��TS������Ӧ�ֶ�֮����һ�������ֶΣ�����Ӧ�ֶκ�ĵ�һ���ֽڱ�ʶ�������ŵĵ����ֶγ����Ƕ��� */
		ts_head_offset += ts_data[ts_head_offset] + 1;

		return 0;
	}

	/* �����ǰTS������PAT����PMT PIDδ��������������ֱ�ӷ���ʧ�� */
	if(-1 == ts_pmt_pid_)
		return -1;

	/* ��� PID = PMT PID ��ǰTS���ݰ�����Ϊ PMT */
	int pmt_pid_13bit = ts_pmt_pid_&0x00001fff;
	int pmt_pid_low_8bit = pmt_pid_13bit&0x000000ff;
	int pmt_pid_high_8bit = pmt_pid_13bit>>8;
	if(pmt_pid_high_8bit == (ts_data[1]&0x1F) && pmt_pid_low_8bit == ts_data[2])
	{
		ts_type = TS_TS_PACKET_TYPE_PMT;
		/* �����ǰ���ݰ���PAT/PMT����TS���صĿ�ʼǰ��TS������Ӧ�ֶ�֮����һ�������ֶΣ�����Ӧ�ֶκ�ĵ�һ���ֽڱ�ʶ�������ŵĵ����ֶγ����Ƕ��� */
		ts_head_offset += ts_data[ts_head_offset] + 1;

		return 0;
	}

	/* �����ǰTS������PAT��PMT����VIDEO PIDδ��������������ֱ�ӷ���ʧ�� */
	if(-1 == ts_video_pid_)
		return -1;

	/* ��� PID = VIDEO PID ��ǰTS���ݰ�����Ϊ VIDEO */
	int video_pid_13bit = ts_video_pid_&0x00001fff;
	int video_pid_low_8bit = video_pid_13bit&0x000000ff;
	int video_pid_high_8bit = video_pid_13bit>>8;
	if(video_pid_high_8bit == (ts_data[1]&0x1F) && video_pid_low_8bit == ts_data[2])
	{
		ts_type = TS_TS_PACKET_TYPE_VIDEO;

		return 0;
	}

	/* �����ǰTS������PAT��PMT��VIDEO����AUDIO PIDδ��������������ֱ�ӷ���ʧ�� */
	if(-1 == ts_audio_pid_)
		return -1;

	/* ��� PID = VIDEO PID ��ǰTS���ݰ�����Ϊ AUDIO */
	int audio_pid_13bit = ts_audio_pid_&0x00001fff;
	int audio_pid_low_8bit = audio_pid_13bit&0x000000ff;
	int audio_pid_high_8bit = audio_pid_13bit>>8;
	if(audio_pid_high_8bit == (ts_data[1]&0x1F) && audio_pid_low_8bit == ts_data[2])
	{
		ts_type = TS_TS_PACKET_TYPE_AUDIO;

		return 0;
	}

	return -1;
}

/* ����Ƶpes���ݰ�֮����ȡ��Ƶ�Ĳ����ʺ�ͨ�������и��� */
int CParseTS::update_es_audio_common_param(unsigned char* pes_data, int pes_data_len)
{
	/* �����Чֵ�жϣ� ������Ч�İ�Ҳ���й��� */
	if(NULL == pes_data || 9 > pes_data_len)		
	{
		return -1;
	}

	/* ����pes����startcode�жϵ�ǰ���ݰ��Ƿ�Ϊ����pes�������ǵ����ݰ�Ҳ���й��� */
	if(false == is_pes_begin(pes_data))
	{
		return -1;
	}

	/* �����ǰTS��֮�е���Ƶ��������Ϊg711, ��ͨ������������ǹ̶���ֵ */
	if (es_param_.audio_param.audio_encode_type == TS_AUDIO_ENCODE_TYPE_PCMA)
	{
		/* g711ͨ�����̶�Ϊ1�� �����ʹ̶�Ϊ8000 */
		es_param_.audio_param.channels = 1;
		es_param_.audio_param.samples_rate = 8000;
	}
	/* �����ǰTS��֮�е���Ƶ��������Ϊaac����ͨ�����Ͳ����ʸ���aac����֮�е�adtsͷ���н��� */
	else if (es_param_.audio_param.audio_encode_type == TS_AUDIO_ENCODE_TYPE_AAC)
	{
		/* ����pes��ͷ����(9�ֽڵĹ̶�����+��ѡ���ȣ���9���ֽڵ���������Ϊ�̶����Ⱥ��ѡ��ͷ�ĳ���) */
		int es_begin_distance = pes_data[8]+9;
		/* pes�������ݲ��������ֽڵ�ʱ��ֱ�ӹ���(�����ֽڵĸ���ʱ����ͨ����������ʵĹؼ�) */
		if(es_begin_distance+3 > pes_data_len)		
		{
			return -1;
		}

		/* pes�ĸ��������������adts startcode(0xFFF)��ֱ�ӷ��ش��� */
		if(!(0xFF == (pes_data[es_begin_distance]&0xFF) && 0xF0 == (pes_data[es_begin_distance]&0xF0)))		//adts startcode
		{
			return -1;
		}

		/* ��������֮�е� sampling_frequency_index �ֶΣ����ݽ����������ֶ����ݶ�Ӧ������ */
		int frequency = (pes_data[es_begin_distance+2]&0x3C)>>2;
		if(0 == frequency)
		{
			es_param_.audio_param.samples_rate = 96000;
		}
		else if(1 == frequency)
		{
			es_param_.audio_param.samples_rate = 88200;
		}
		else if(2 == frequency)
		{
			es_param_.audio_param.samples_rate = 64000;
		}
		else if(3 == frequency)
		{
			es_param_.audio_param.samples_rate = 48000;
		}
		else if(4 == frequency)
		{
			es_param_.audio_param.samples_rate = 44100;
		}
		else if(5 == frequency)
		{
			es_param_.audio_param.samples_rate = 32000;
		}
		else if(6 == frequency)
		{
			es_param_.audio_param.samples_rate = 24000;
		}
		else if(7 == frequency)
		{
			es_param_.audio_param.samples_rate = 22050;
		}
		else if(8 == frequency)
		{
			es_param_.audio_param.samples_rate = 16000;
		}
		else if(9 == frequency)
		{
			es_param_.audio_param.samples_rate = 12000;
		}
		else if(10 == frequency)
		{
			es_param_.audio_param.samples_rate = 11025;
		}
		else if(11 == frequency)
		{
			es_param_.audio_param.samples_rate = 8000;
		}
		else if(12 == frequency)
		{
			es_param_.audio_param.samples_rate = 7350;
		}
		else
		{
			printf("unkown audio frequency");
			return -1;
		}

		/* ��������֮�е� channel_configuration �ֶΣ����ݽ����������ֶ����ݶ�Ӧ������ */
		int channles = ((pes_data[es_begin_distance+2]&0x01)<<2) + ((pes_data[es_begin_distance+3]&0xC0)>>6);
		if(0 == channles)						//����ط����� ���µ��������������channles�� 0��ֱ�Ӹ�ֵ 2 �Ż�����
		{
			es_param_.audio_param.channels = 2;	
		}
		else if(1 == channles)
		{
			es_param_.audio_param.channels = 1;
		}
		else if(2 == channles)
		{
			es_param_.audio_param.channels = 2;
		}
		else if(3 == channles)
		{
			es_param_.audio_param.channels = 3;
		}
		else if(4 == channles)
		{
			es_param_.audio_param.channels = 4;
		}
		else if(5 == channles)
		{
			es_param_.audio_param.channels = 5;
		}
		else if(6 == channles)
		{
			es_param_.audio_param.channels = 6;
		}
		else if(7 == channles)
		{
			es_param_.audio_param.channels = 8;
		}
		else
		{
			printf("unknow audio channle num!");
			return -1;
		}
	}
	else
	{
		printf("know audio decode type!");
		return  -1;
	}

	return 0;
}

/* Ѱ��һ��������֮��nalu startcode��λ�� */
unsigned char* CParseTS::find_nalu_startcode(unsigned char* data, int &data_len)
{
	if(NULL == data || data_len < 4)
	{
		return NULL; 
	}

	data += 3;
	data_len -= 3;

	while(data_len >= 0)
	{
		if(*data == 0x01)
		{
			if(0x00 == *(data-1) && 0x00 == *(data-2) && 0x00 == *(data-3))
			{
				data_len += 3;	//��Ϊ����λ����ǰ�ƶ�3���ֽڣ�����data_lenӦ�ü�3
				return data-3;	
			}
			else
			{
				data += 4;
				data_len -= 4;
				continue;
			} 	
		}
		data += 1;
		data_len -= 1;
	}

	return NULL;
}

/* ����һ����pes���ݰ�֮�н�����������Ƶ��������ƵES����֮�� */
int CParseTS::cpy_data_to_video_es_memory(unsigned char* data, int data_len)
{
	if(NULL == data || data_len <= 0)
	{
		return -1;
	}

	if(data_len <= ES_BUFFER - es_video_data_index_)
	{
		memcpy(es_video_data_+es_video_data_index_, data, data_len);
		es_video_data_index_ += data_len;
		return data_len;
	}
	else
	{
		return -1;
	}
}

/* ����һ����pes���ݰ�֮�н�����������Ƶ��������ƵES����֮�� */
int CParseTS::cpy_data_to_audio_es_memory(unsigned char* data, int data_len)
{
	if(NULL == data || data_len <= 0)
	{
		return -1;
	}

	if(data_len <= ES_BUFFER - es_audio_data_index_)
	{
		memcpy(es_audio_data_+es_audio_data_index_, data, data_len);
		es_audio_data_index_ += data_len;
		return data_len;
	}
	else
	{
		return -1;
	}
}

/* �ж�pes���Ƿ���Ч(��ʼ��pes startcode) */
bool CParseTS::is_pes_begin(unsigned char* pes_data)
{
	if(NULL == pes_data)
	{
		return false;
	}

	if(!(pes_data[0] == 0x00 && pes_data[1] == 0x00 && pes_data[2] == 0x01))
	{
		return false;
	}
	else
	{
		return true;
	}
}

/* �ж�es֡�����Ƿ���Ч(��ʼ��nalu startcode) */
bool CParseTS::is_startcode_begin(unsigned char* es_data)
{
	if(NULL == es_data)
	{
		return false;
	}

	if(!(es_data[0] == 0x00 && es_data[1] == 0x00 && es_data[2] == 0x00 && es_data[3] == 0x01))
	{
		return false;
	}
	else
	{
		return true;
	}
}

/* ��ȡһ��pes���ݰ�֮�и���ES���ݵ�ƫ��λ�� */
unsigned char* CParseTS::get_es_pos(unsigned char* pes_data)
{
	unsigned char* pes_es_pos =  (true == is_pes_begin(pes_data)) ? pes_data+pes_data[8]+9 : pes_data;	

	return pes_es_pos;
}

/* ��ȡһ��pes���ݰ�֮�и���ES���ݵĳ��� */
int CParseTS::get_es_length(unsigned char* pes_data, int pes_len)
{
	int es_data_len = (true == is_pes_begin(pes_data)) ? pes_len-pes_data[8]-9 : pes_len;

	return es_data_len;
}

/* ��pes���ݵ�pesͷ֮�н���pts */
unsigned __int64 CParseTS::get_es_pts(const unsigned char* pes_data) 
{
	pes_data += 9;		//ƫ����pts����λ��
	return (unsigned __int64)(*pes_data & 0x0e) << 29 |
		(AV_RB16(pes_data + 1) >> 1) << 15 |
		AV_RB16(pes_data + 3) >> 1;
}

/* ��pes���ݵ�pesͷ֮�н���dts */
unsigned __int64 CParseTS::get_es_dts(const unsigned char* pes_data)
{
	pes_data += 14;		//ƫ����dts����λ��
	return (unsigned __int64)(*pes_data & 0x0e) << 29 |
		(AV_RB16(pes_data + 1) >> 1) << 15 |
		AV_RB16(pes_data + 3) >> 1;
}

/* �ж�һ֡�����Ƿ�Ϊ�ؼ�֡�� sps, ppsҲ��Ϊ�ǹؼ�֡ */
bool CParseTS::es_is_i_frame(const unsigned char* es_data)
{
	bool is_key_frame = ((es_data[4]&0x1F) == 5 || (es_data[4]&0x1F) == 2 || 
						 (es_data[4]&0x1F) == 7 || (es_data[4]&0x1F) == 8) ? true : false;

	return is_key_frame;
}

/* ��ȡ��Ƶ֡���� */
TS_ESFrameType_E CParseTS::get_video_frame_type(unsigned char* es_data)
{
	if(NULL == es_data)
	{
		return TS_ES_FRAME_TYPE_INVALID;
	}

	int type = es_data[4]&0x1F;
	switch(type){
	case 2:
		return TS_ES_FRAME_TYPE_DATA;
	case 5:
		return TS_ES_FRAME_TYPE_IDR;
	case 6:
		return TS_ES_FRAME_TYPE_SEI;
	case 7:
		return TS_ES_FRAME_TYPE_SPS;
	case 8:
		return TS_ES_FRAME_TYPE_PPS;
	}

	return TS_ES_FRAME_TYPE_INVALID;
}

/* TS�������Ƿ�����Ƶ���� */
bool CParseTS::has_audio_stream()
{
	/* ���Ѱ�ҵ���Ƶ ts_audio_pid �ᱻ���µ��� */
	if(-1 == ts_audio_pid_)
	{
		return false;
	}
	else
	{
		return true;
	}
}