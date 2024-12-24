#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <vector>
#include <string>

using namespace std;

// ȫ�ֳ�������
extern const int NUSERS;     // ��Ծ�û�����
extern const int NBITS;      // ÿ���û����͵ı�������
extern const int SF;         // ��Ƶ�ı���
extern const int Nr;         // ��������
extern const int N;			 // ���������ֳ���
extern const int FrameLen;   // �ܵ����ֵĳ���
extern const int L;

extern const double SNR_BEGIN;  // SNR�Ŀ�ʼֵ
extern const double SNR_END;    // SNR�Ľ���ֵ
extern const int SNR_NUM;       // SNR����

extern const int NUM_FRAMES;   // ֡����
extern const int NUM_PRINT;    // ��ӡ��ʾ���
extern const int BlockLen;     // ��˥��ĳ���

extern const string CodeMode;  // ���Ʊ���ģʽ��Polar or None��
extern const bool IsFading;    // ����˥��ģʽ��ȫ�ֿ��أ�

extern std::string filename;   // ���ݱ�����ļ���

// ȫ�ֱ�������
extern std::vector<double> SNR_dB;                         // SNRֵ������dB��
extern std::vector<double> snr;							   // SNRֵ�����ԣ�
extern std::vector<std::vector<double>> BER;               // �洢������
extern std::vector<std::vector<double>> PUPE;              // �洢PUPE������Ϊĳ������������

#endif // GLOBAL_VARIABLES_H
