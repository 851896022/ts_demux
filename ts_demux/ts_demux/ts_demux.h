#pragma once

/*  
*  �⸴��ts������
*/
class CParseTS
{
public:
	CParseTS();
	~CParseTS();

	/* ���ý���es�������ݵ����ݻص����� */
	int set_es_callback(es_callback es_cb, void* user_param);

	/* ��ʼ���⸴�� */
	int init_parse();

	/* ���TS���ݣ����ݳ��ȱ�����TSͷ��ʼ������Ϊts packet������ */
	void put_pkt_data(unsigned char* pkt_data, int pkt_data_len);

	/* TS�������Ƿ�����Ƶ���� */
	bool has_audio_stream();

protected:
private:
	/**
	* @brief ����һ��������ts���ݰ�
	*
	* @param[in] ts_data ts����
	* @param[out] ts_type ts���ݽ������
	* @param[out] ts_head_offset ts����ͷ����
	*
	*/
	int parse_a_ts_packet(unsigned char* ts_data, TS_TSPacketType_E &ts_type, int &ts_head_offset);

	/* ����Ƶpes���ݰ�֮����ȡ��Ƶ�Ĳ����ʺ�ͨ�������и��� */
	int update_es_audio_common_param(unsigned char* pes_data, int pes_data_len);

	/* ����һ����pes���ݰ�֮�н�����������Ƶ��������ƵES����֮�� */
	int cpy_data_to_video_es_memory(unsigned char* data, int data_len);

	/* ����һ����pes���ݰ�֮�н�����������Ƶ��������ƵES����֮�� */
	int cpy_data_to_audio_es_memory(unsigned char* data, int data_len);

	/* �ж�pes���Ƿ���Ч(��ʼ��pes startcode) */
	bool is_pes_begin(unsigned char* pes_data);

	/* �ж�es֡�����Ƿ���Ч(��ʼ��nalu startcode) */
	bool is_startcode_begin(unsigned char* es_data);

	/* ��ȡһ��pes���ݰ�֮�и���ES���ݵ�ƫ��λ�� */
	unsigned char* get_es_pos(unsigned char* pes_data);

	/* ��ȡһ��pes���ݰ�֮�и���ES���ݵĳ��� */
	int get_es_length(unsigned char* pes_data, int pes_len);

	/* ��pes���ݵ�pesͷ֮�н���pts */
	unsigned __int64 get_es_pts(const unsigned char* pes_data);

	/* ��pes���ݵ�pesͷ֮�н���dts */
	unsigned __int64 get_es_dts(const unsigned char* pes_data);

	/* �ж�һ֡�����Ƿ�Ϊ�ؼ�֡ */
	bool es_is_i_frame(const unsigned char* es_data);

	/* Ѱ��һ��������֮��nalu startcode��λ�� */
	static unsigned char* find_nalu_startcode(unsigned char* data, int &data_len);

	/* ��ȡ��Ƶ֡���� */
	TS_ESFrameType_E get_video_frame_type(unsigned char* es_data);

	es_callback es_cb_;
	void* es_cb_user_param_;		// ������û��ص��������û�����

	int ts_pmt_pid_;				// pmt pid
	int ts_video_pid_;				// video pid
	int ts_audio_pid_;				// audio pid

	TS_ESParam_S es_param_;
	bool es_audio_param_get_;		// �Ƿ��Ѿ����¹���Ƶ����

	unsigned char* es_video_data_;  // ��ƵES���ݻ���
	int es_video_data_index_;       // ��ƵES�������ݵĳ���

	unsigned char* es_audio_data_;	// ��ƵES���ݻ���
	int es_audio_data_index_;		// ��ƵES�������ݵĳ���
};