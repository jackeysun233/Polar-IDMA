#include <vector>
#include <random>
#include "global_variables.h"

using namespace std;

// ����SNR��ֵ
void GenSNR() {
    // ���� SNR �Ĳ���
    double step = (SNR_NUM > 1) ? (SNR_END - SNR_BEGIN) / (SNR_NUM - 1) : 0.0;

    // ��ʼ�� SNR ֵ����
    vector<double> SNR_values(SNR_NUM, 0.0);

    // ���� SNR �ľ���ȡֵ
    for (int i = 0; i < SNR_NUM; ++i) {
        SNR_dB[i] = SNR_BEGIN + i * step;                // SNR �� dB ֵ
        SNR_values[i] = pow(10, SNR_dB[i] / 10.0);       // ת��Ϊ���Ե�λ
    }
}

// �����û���Ϣ
void GenMessage(vector<vector<int>>& input_data) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> bit_dist(0, 1);

    // �������������
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            input_data[j][i] = bit_dist(rng);
        }
    }
}

// ������������Ϊ1����������
void GenNoise(vector<vector<double>>& noise_parity) {
    // ��ʼ��������������ͷֲ�
    random_device rd;                   // ��������
    mt19937 generator(rd());            // Mersenne Twister ����
    normal_distribution<double> distribution(0.0, 1.0);  // ��ֵΪ0����׼��Ϊ1����̬�ֲ�

    // ��� noise_parity ��ÿ��Ԫ��
    for (int i = 0; i < Nr; ++i) {
        for (int j = 0; j < FrameLen; ++j) {
            noise_parity[i][j] = distribution(generator);
        }
    }
}

// ����˥��ϵ����������Ҫ�޸ģ�
void GenFadingCoff(vector<vector<vector<double>>>& FadingCoff) {
    // ������� NUSERS * NBITS < M
    if (NUSERS * NBITS > M) {
        throw invalid_argument("����NUSERS * NBITS ����С�ڵ��� M��");
    }

    // ��ʼ�������������
    mt19937 rng(random_device{}());

    // ����ÿ�����˥��ϵ������Ĵ�С
    int num_blocks = FrameLen / BlockLen; // �����������
    int remaining_slots = FrameLen % BlockLen; // ʣ�ಿ��
    if (remaining_slots > 0) {
        num_blocks++; // �����ʣ�ಿ�֣�����һ����
    }

    // ��ʼ�� FadingCoff ���󣬴�СΪ Nr x NUSERS x FrameLen
    FadingCoff.resize(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen, 0.0)));

    // Ϊÿ�����ߺ�ÿ���û�����˥��ϵ��
    for (int nr = 0; nr < Nr; ++nr) {
        for (int user = 0; user < NUSERS; ++user) {
            // ÿ�����˥��ϵ��
            uniform_real_distribution<double> uniformDist(0.0, 1.0);
            for (int block = 0; block < num_blocks; ++block) {
                double U = uniformDist(rng);
                double h = sqrt(-2.0 * log(U)); // �����ֲ������ɷ�ʽ

                // ����Ӧ��˥��ϵ��
                for (int i = block * BLOCK_LEN; i < (block + 1) * BLOCK_LEN && i < FrameLen; ++i) {
                    FadingCoff[nr][user][i] = h; // �����ɵ�˥��ϵ�����ÿ���û���ÿ������
                }
            }
        }
    }
}
