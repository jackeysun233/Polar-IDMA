#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include "IDMA.h"
#include "PolarCode.h"
#include "global_variables.h"

using namespace std;

void InterLeaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& ILData) {
    // ȷ�� ILData �Ѿ������
    for (size_t i = 0; i < NUSERS; ++i) {
        fill(ILData[i].begin(), ILData[i].end(), 0);
    }

    // ��ÿһ�н��н�֯����
    for (size_t i = 0; i < FrameLen; ++i) {
        for (size_t j = 0; j < NUSERS; ++j) {
            // Temporary vector for each user (column)
            ILData[j][i] = Datain[j][ILidx[j][i]];  // ����֯������ݴ���ILData
        }
    }
}


void deInterleaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& deILData) {
    // ȷ�� deILData �Ѿ������
    for (size_t i = 0; i < NUSERS; ++i) {
        fill(deILData[i].begin(), deILData[i].end(), 0);
    }

    // ��ÿһ�н��з���֯����
    for (size_t i = 0; i < FrameLen; ++i) {
        for (size_t j = 0; j < NUSERS; ++j) {
            // ���ݽ�֯���������ݻָ���ԭʼλ��
            deILData[j][ILidx[j][i]] = Datain[j][i];
        }
    }
}



void spreader(const vector<vector<double>>& inputData, vector<vector<double>>& spreadedData) {

    // ����α�����Ƶ����
    vector<int> repCode(SF);
    for (size_t i = 0; i < SF; ++i) {
        repCode[i] =  1 - 2 * (i % 2);  // 1-2*mod(0:(SF-1), 2)
    }


    // ��ÿ���û�����Ϣ������Ƶ����
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            // ÿ���û���ÿһ����Ƶ
            for (size_t k = 0; k < SF; ++k) {
                spreadedData[i][j * SF + k] = inputData[i][j] * repCode[k];
            }
        }
    }
}


void despreader(const vector<vector<double>>& spreadedData, vector<vector<double>>& despreadedData) {


    // ��ʼ�� despreadedData ���󣬴�СΪ NUSERS x N
    despreadedData.assign(NUSERS, vector<double>(N, 0));

    // ��ÿ���û������ݽ��з���Ƶ����
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < SF; ++k) {
                despreadedData[i][j] += spreadedData[i][j * SF + k] * (1 - 2 * static_cast<double>(k % 2));  // α�����Ƶ����
            }
        }
    }
}






void calESE(const vector<double>& R,
    const vector<vector<double>>& apLLR,
    const vector<vector<double>>& H,
    double noiseVar,
    vector<vector<double>>& extrLLR) {

    size_t H_rows = H.size();        // H ������
    size_t H_cols = H[0].size();     // H ������

    // ��ʼ����ֵ�ͷ���
    vector<vector<double>> avgX(H_rows, vector<double>(H_cols));
    vector<vector<double>> varX(H_rows, vector<double>(H_cols));

    // �����ֵ�ͷ���
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            avgX[j][i] = tanh(apLLR[j][i] / 2);  // ʹ�� apLLR[j][i] ������ avgX
            varX[j][i] = 1 - avgX[j][i] * avgX[j][i];  // ���㷽��
        }
    }

    // ��������ź� r �ľ�ֵ�ͷ���
    vector<double> avgR(H_cols, 0.0);
    vector<double> varR(H_cols, 0.0);
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            avgR[i] += avgX[j][i] * H[j][i];  // ���� H ���м��� avgR
            varR[i] += varX[j][i] * H[j][i] * H[j][i];  // ���� varR
        }
        varR[i] += noiseVar;  // ������������
    }

    // ���� ESE (Extrinsic Log-Likelihood Ratio)
    extrLLR.resize(H_rows, vector<double>(H_cols)); // extrLLR �Ƕ�ά����
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            // ���� a �� b
            double a = R[i] - avgR[i] + H[j][i] * avgX[j][i];
            double b = varR[i] - H[j][i] * H[j][i] * varX[j][i];

            // ���� extrLLR
            extrLLR[j][i] = 2 * H[j][i] * (a / b);
        }
    }
}



// BPSK ����
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data) {


    // ���� BPSK �����ź�
    for (int user = 0; user < NUSERS; ++user) {
        for (int bit = 0; bit < input_data[0].size(); ++bit) {
            // ��ÿ�����ؽ��е���
            modulated_data[user][bit] = (input_data[user][bit] == 0) ? -1.0 : 1.0;
        }
    }
}

// ����ÿ���û��Ľ�֯������
void GenILidx(std::vector<std::vector<int>>& ILidx) {
    // ��� ILidx�������ε���ʱ�ظ�����
    ILidx.clear();

    // ��ʼ�������������
    random_device rd;
    mt19937 rng(rd());

    // ��ÿ���û������������
    for (int x = 0; x < NUSERS; ++x) {
        // ���ɳ�ʼ���� 0, 1, ..., FrameLen-1
        std::vector<int> temp(FrameLen);
        for (int i = 0; i < FrameLen; ++i) {
            temp[i] = i;
        }

        // �����������
        shuffle(temp.begin(), temp.end(), rng);

        // �����Һ���������� ILidx
        ILidx.push_back(temp);
    }
}

// Channel����������������ݴ���˥�䡢����������
void channel(const vector<vector<double>>& ILData,
    const vector<vector<vector<double>>>& FadingCoff,
    const vector<vector<double>>& noise,
    double noise_variance,
    vector<vector<double>>& RxData) {

    // ��ÿ�����߽��д���
    for (int nr = 0; nr < Nr; ++nr) {
        // ��ʼ����������
        vector<double> receivedData(FrameLen, 0.0);

        // ��ÿ���û������źŵ���
        for (int user = 0; user < NUSERS; ++user) {
            // ����˥��ϵ��h��FadingCoff[nr][user]Ϊ�����ߺ��û���˥��ϵ��
            for (int i = 0; i < FrameLen; ++i) {
                receivedData[i] += FadingCoff[nr][user][i] * ILData[user][i];
            }
        }

        // Ϊ���յ���������������������������ʵĵ���
        for (int i = 0; i < FrameLen; ++i) {
            receivedData[i] += sqrt(noise_variance) * noise[nr][i]; // ����������ֵ
        }

        // �����յ������ݴ洢��RxData��
        for (int i = 0; i < FrameLen; ++i) {
            RxData[nr][i] = receivedData[i];
        }
    }
}  

// ����MIMO���ݣ�����������ݺ�˥��ϵ����ƽ��ֵ
void processMIMOData(const vector<vector<double>>& RxData,
    const vector<vector<vector<double>>>& FadingCoff,
    vector<double>& avg_RxData,
    vector<vector<double>>& avg_FadingCoff) {

    // �Խ������ݽ���ƽ������
    for (int i = 0; i < FrameLen; ++i) {
        double sum = 0.0;
        for (int nr = 0; nr < Nr; ++nr) {
            sum += RxData[nr][i];
        }
        avg_RxData[i] = sum / Nr;  // ��ƽ��ֵ
    }

    // ��ÿ������ʱ�̵�FadingCoff����ƽ������
    for (int user = 0; user < NUSERS; ++user) {
        for (int i = 0; i < FrameLen; ++i) {
            double sum = 0.0;
            for (int nr = 0; nr < Nr; ++nr) {
                sum += FadingCoff[nr][user][i];
            }
            avg_FadingCoff[user][i] = sum / Nr;  // ��ƽ��ֵ
        }
    }
}

void hardDecision(const vector<vector<double>>& deSpData,
    vector<vector<vector<int>>>& output_data,
    int i) {  
    for (size_t user = 0; user < NUSERS; ++user) {
        for (size_t bit = 0; bit < NBITS; ++bit) {
            // ��ÿ�����յ��ķ��Ž���Ӳ�о�
            if (deSpData[user][bit] > 0) {
                output_data[i][user][bit] = 1;  // �о�Ϊ0
            }
            else {
                output_data[i][user][bit] = 0;  // �о�Ϊ1
            }
        }
    }
}


void calcError(const vector<vector<vector<int>>>& output_data,
    const vector<vector<int>>& input_data,
    int sim) {
    int total_bits = NUSERS * NBITS;  // �ܱ�������

    for (int snr = 0; snr < SNR_NUM; ++snr) {
        int bit_errors = 0;  // ��ǰ SNR �ı��ش�������
        int block_errors = 0; // ��ǰ SNR �Ŀ��������

        // ��ÿ���û�����������
        for (int user = 0; user < NUSERS; ++user) {
            int user_bit_errors = 0; // ��ǰ�û��ı��ش�������

            // ��ȡ output_data �е��о����ݺ� input_data �е���ʵ����
            const vector<int>& estimated_codeword = output_data[snr][user];
            const vector<int>& true_codeword = input_data[user];

            // ��λ�Ƚ� estimated_codeword �� true_codeword
            for (int bit = 0; bit < NBITS; ++bit) {
                if (estimated_codeword[bit] != true_codeword[bit]) {
                    user_bit_errors++; // ��¼���ش���
                }
            }

            // ������ڱ��ش����������������
            if (user_bit_errors > 0) {
                block_errors++;
            }

            // �ۼӱ��ش�������
            bit_errors += user_bit_errors;
        }

        // ���㲢���� BER �� PUPE
        BER[snr][sim] = static_cast<double>(bit_errors) / total_bits;
        PUPE[snr][sim] = static_cast<double>(block_errors) / NUSERS;
    }
}

// ����PolarCode������ŵ�����
void ChannelEncode(const vector<vector<int>>& input_data, PolarCode& pc, vector<vector<int>>& encoded_data) {
    // ����ÿ���û�
    for (int k = 0; k < NUSERS; ++k) {
        // ����������ת��Ϊ uint8_t ����
        vector<uint8_t> tmp(input_data[k].size());
        transform(input_data[k].begin(), input_data[k].end(), tmp.begin(), [](int x) {
            return static_cast<uint8_t>(x);
            });

        // ��ȡ����������
        vector<uint8_t> encoded = pc.encode(tmp);

        // ������������ת��Ϊ int ���Ͳ��洢�� encoded_data ��
        for (size_t i = 0; i < encoded.size(); ++i) {
            encoded_data[k][i] = static_cast<int>(encoded[i]);  // ת��Ϊ int ���洢
        }
    }
}

// Polar��������
void ChannelDecoder(const vector<vector<double>>& deSpData,
    vector<vector<vector<int>>>& output_data,
    int i, PolarCode& pc) {
    for (size_t user = 0; user < NUSERS; ++user) {
        // ����PolarCode��SCL������
        auto decoded = pc.decode_scl_llr(deSpData[user], L); // ����list_sizeΪ8

        for (size_t bit = 0; bit < NBITS; ++bit) {
            // ��������ת��Ϊint������output_data
            output_data[i][user][bit] = static_cast<int>(decoded[bit]);
        }
    }
}


vector<vector<double>> Transmitter(
const vector<vector<int>>& InputData,
const vector<vector<int>>& ScrambleRule,
const vector<int>& SpreadSeq
)
{
    int i, j, nuser;
    
    double tmp;

    vector<vector<double>> Chip(NUSERS, vector<double>(FrameLen));
    vector<vector<double>> Tx(NUSERS, vector<double>(FrameLen));


    // Spreading process.
    for (int j = 0; j < NUSERS; j++) for (int i = 0; i < FrameLen; i++)
    {    
        tmp = 1 - (2 * InputData[j][i]);
        for (int s= 0; s < SF; s++) Chip[j][i] = tmp * SpreadSeq[s];
    }

    // Interleaving process.
    for (int j = 0; j < NUSERS; j++) for (int i = 0; i < FrameLen; i++)
        Tx[nuser][i] = Chip[nuser][ScrambleRule[nuser][i]];

    return Tx;
}


vector<double> Channel(double sigma, 
    const vector<double>& Noise, 
    const vector<vector<double>>& H,
    const vector<vector<double>>& Tx) {

    vector<double> Rx(FrameLen);

    for (int i = 0; i < FrameLen; i++)
    {
        // Additive white Gaussian noise.
        Rx[i] = sigma * Noise[i];

        // Multiple access channel and energy scaling.
        for (int j = 0; j < NUSERS; j++) Rx[i] += H[j][i] * Tx[j][i];
    }

}

vector<vector<double>> Receiver(
    const double sigma,
    const int IDMAitr,
    const vector<vector<int>>& ScrambleRule,
    const vector<vector<double>>& H,
    const vector<double> Rx
) {

    // ���岢��ʼ�� TotalMean �� TotalVar
    vector<double> TotalMean(FrameLen, 0.0); // �ܾ�ֵ����ʼ��Ϊ 0
    vector<double> TotalVar(FrameLen, sigma * sigma); // �ܷ����ʼ��Ϊ sigma^2

    // ���岢��ʼ�� Mean �� Var
    vector<vector<double>> Mean(NUSERS, vector<double>(FrameLen, 0.0)); // �û���ֵ����ʼ��Ϊ 0
    vector<vector<double>> Var(NUSERS, vector<double>(FrameLen, 0.0));  // �û������ʼ��Ϊ 0



    // ��ʼ����ֵ����
    for (int i = 0; i < FrameLen; i++)
    {
        TotalMean[i] = 0;
        TotalVar[i] = sigma * sigma;
    }
    for (int j = 0; j < NUSERS; j++) for (int i = 0; i < FrameLen; i++)
    {
        Mean[j][i] = 0;
        Var[j][i] = H[j][i] * H[j][i];
        TotalVar[i] += H[j][i] * H[j][i];
    }


    for (int it = 0; it < IDMAitr; it++) for (int j = 0; j < NUSERS; j++)
    {
        // Produce the LLR values for the de-interleaved chip sequence.
        for (int i = 0; i < FrameLen; i++)
        {
            TotalMean[i] -= Mean[j][i];
            TotalVar[i] -= Var[j][i];
            Chip[j][ScrambleRule[j][i]] = 2 * H[j][i] * (Rx[i] - TotalMean[i]) / TotalVar[i];
        }

        // De-spreading operation.(TODO)
        for (int k = 0; k < NBITS; k++)
        {
            
            for (int s = 0; s < SF; s++) appllr[k] += SpreadSeq[s] * chip[m++];
        }

        // Feed the AppLlr to decoder, if there is a FEC codec in the system.

        // Spreading operation: Produce the extrinsic LLR for each chip
        for (m = i = 0; i < _DataLen; i++) for (j = 0; j < _SpreadLen; j++, m++)
            Ext[nuser][m] = SpreadSeq[j] * AppLlr[nuser][i] - Chip[nuser][m];

        // Update the statistic variable together with the interleaving process.
        for (i = 0; i < _ChipLen; i++)
        {
            Mean[nuser][i] = H[nuser] * tanh(Ext[nuser][ScrambleRule[nuser][i]] / 2);
            Var[nuser][i] = H2[nuser] - Mean[nuser][i] * Mean[nuser][i];
            TotalMean[i] += Mean[nuser][i];
            TotalVar[i] += Var[nuser][i];
        }
    }
}