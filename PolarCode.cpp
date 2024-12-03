#include "PolarCode.h"
#include <bits/stdc++.h>
#include <random>



void PolarCode::compute_weights(std::vector<int>& weights) {
    weights.assign(_block_length, 0);
    if (_block_length > 0) weights[0] = 1;
    if (_block_length > 1) weights[1] = 2;

    for (size_t w = 1; (1u << w) < _block_length; ++w) {
        uint32_t temp = 1u << w;
        for (size_t v = temp; v < (temp << 1) && v < _block_length; ++v) {
            weights[v] = weights[v - temp] * 2;
        }
    }
}

void PolarCode::sort_channel_orders(std::vector<double>& channel_vec, const std::vector<int>& weights) {
    // 排序降序
    std::sort(_channel_order_descending.begin(), _channel_order_descending.end(),
        [&](int i1, int i2) {
            if (weights[i1] != weights[i2]) {
                return weights[i1] > weights[i2];
            }
            return channel_vec[_bit_rev_order[i1]] < channel_vec[_bit_rev_order[i2]];
        });

    // 排序升序
    std::sort(_channel_order_ascending.begin(), _channel_order_ascending.end(),
        [&](int i1, int i2) {
            if (weights[i1] != weights[i2]) {
                return weights[i1] < weights[i2];
            }
            return channel_vec[_bit_rev_order[i1]] > channel_vec[_bit_rev_order[i2]];
        });
}

void PolarCode::initialize_frozen_bits() {
    // 预分配大小并初始化
    _channel_order_descending.resize(_block_length);
    _channel_order_ascending.resize(_block_length);
    _frozen_bits.resize(_block_length, 1); // 默认冻结比特为1

    if (!_5G) {
        // 使用构造函数直接初始化
        std::vector<double> channel_vec(_block_length, _design_epsilon);

        // 迭代更新 channel_vec
        for (uint8_t iteration = 0; iteration < _n; ++iteration) {
            uint32_t increment = 1u << iteration;
            for (uint32_t j = 0; j < increment; ++j) {
                for (uint32_t i = 0; i < _block_length; i += 2 * increment) {
                    double& c1 = channel_vec[i + j];
                    double& c2 = channel_vec[i + j + increment];
                    double new_c1 = c1 + c2 - c1 * c2;
                    double new_c2 = c1 * c2;
                    c1 = new_c1;
                    c2 = new_c2;
                }
            }
        }

        std::iota(_channel_order_descending.begin(), _channel_order_descending.end(), 0);
        std::iota(_channel_order_ascending.begin(), _channel_order_ascending.end(), 0);

        if (_RM) {
            // 计算权重
            std::vector<int> weights;
            compute_weights(weights);

            // 排序
            sort_channel_orders(channel_vec, weights);
        }
        else {
            // 注意比特反转
            std::sort(_channel_order_descending.begin(), _channel_order_descending.end(),
                [&](int i1, int i2) { return channel_vec[_bit_rev_order[i1]] < channel_vec[_bit_rev_order[i2]]; });

            std::sort(_channel_order_ascending.begin(), _channel_order_ascending.end(),
                [&](int i1, int i2) { return channel_vec[_bit_rev_order[i1]] > channel_vec[_bit_rev_order[i2]]; });
        }
    }
    else { // _5G
        // 使用静态常量数组初始化 _Q，避免每次调用函数时都重新赋值
        static const std::vector<int> Q_INIT = { 0,1,2,4,8,16,32,3,5,64,9,6,17,10,18,128,12,33,65,20,256,34,24,36,7,129,66,512,11,40,68,130,19,13,48,14,72,257,21,132,35,258,26,513,80,37,25,22,136,260,264,38,514,96,67,41,144,28,69,42,516,49,74,272,160,520,288,528,192,544,70,44,131,81,50,73,15,320,133,52,23,134,384,76,137,82,56,27,97,39,259,84,138,145,261,29,43,98,515,88,140,30,146,71,262,265,161,576,45,100,640,51,148,46,75,266,273,517,104,162,53,193,152,77,164,768,268,274,518,54,83,57,521,112,135,78,289,194,85,276,522,58,168,139,99,86,60,280,89,290,529,524,196,141,101,147,176,142,530,321,31,200,90,545,292,322,532,263,149,102,105,304,296,163,92,47,267,385,546,324,208,386,150,153,165,106,55,328,536,577,548,113,154,79,269,108,578,224,166,519,552,195,270,641,523,275,580,291,59,169,560,114,277,156,87,197,116,170,61,531,525,642,281,278,526,177,293,388,91,584,769,198,172,120,201,336,62,282,143,103,178,294,93,644,202,592,323,392,297,770,107,180,151,209,284,648,94,204,298,400,608,352,325,533,155,210,305,547,300,109,184,534,537,115,167,225,326,306,772,157,656,329,110,117,212,171,776,330,226,549,538,387,308,216,416,271,279,158,337,550,672,118,332,579,540,389,173,121,553,199,784,179,228,338,312,704,390,174,554,581,393,283,122,448,353,561,203,63,340,394,527,582,556,181,295,285,232,124,205,182,643,562,286,585,299,354,211,401,185,396,344,586,645,593,535,240,206,95,327,564,800,402,356,307,301,417,213,568,832,588,186,646,404,227,896,594,418,302,649,771,360,539,111,331,214,309,188,449,217,408,609,596,551,650,229,159,420,310,541,773,610,657,333,119,600,339,218,368,652,230,391,313,450,542,334,233,555,774,175,123,658,612,341,777,220,314,424,395,673,583,355,287,183,234,125,557,660,616,342,316,241,778,563,345,452,397,403,207,674,558,785,432,357,187,236,664,624,587,780,705,126,242,565,398,346,456,358,405,303,569,244,595,189,566,676,361,706,589,215,786,647,348,419,406,464,680,801,362,590,409,570,788,597,572,219,311,708,598,601,651,421,792,802,611,602,410,231,688,653,248,369,190,364,654,659,335,480,315,221,370,613,422,425,451,614,543,235,412,343,372,775,317,222,426,453,237,559,833,804,712,834,661,808,779,617,604,433,720,816,836,347,897,243,662,454,318,675,618,898,781,376,428,665,736,567,840,625,238,359,457,399,787,591,678,434,677,349,245,458,666,620,363,127,191,782,407,436,626,571,465,681,246,707,350,599,668,790,460,249,682,573,411,803,789,709,365,440,628,689,374,423,466,793,250,371,481,574,413,603,366,468,655,900,805,615,684,710,429,794,252,373,605,848,690,713,632,482,806,427,904,414,223,663,692,835,619,472,455,796,809,714,721,837,716,864,810,606,912,722,696,377,435,817,319,621,812,484,430,838,667,488,239,378,459,622,627,437,380,818,461,496,669,679,724,841,629,351,467,438,737,251,462,442,441,469,247,683,842,738,899,670,783,849,820,728,928,791,367,901,630,685,844,633,711,253,691,824,902,686,740,850,375,444,470,483,415,485,905,795,473,634,744,852,960,865,693,797,906,715,807,474,636,694,254,717,575,913,798,811,379,697,431,607,489,866,723,486,908,718,813,476,856,839,725,698,914,752,868,819,814,439,929,490,623,671,739,916,463,843,381,497,930,821,726,961,872,492,631,729,700,443,741,845,920,382,822,851,730,498,880,742,445,471,635,932,687,903,825,500,846,745,826,732,446,962,936,475,853,867,637,907,487,695,746,828,753,854,857,504,799,255,964,909,719,477,915,638,748,944,869,491,699,754,858,478,968,383,910,815,976,870,917,727,493,873,701,931,756,860,499,731,823,922,874,918,502,933,743,760,881,494,702,921,501,876,847,992,447,733,827,934,882,937,963,747,505,855,924,734,829,965,938,884,506,749,945,966,755,859,940,830,911,871,639,888,479,946,750,969,508,861,757,970,919,875,862,758,948,977,923,972,761,877,952,495,703,935,978,883,762,503,925,878,735,993,885,939,994,980,926,764,941,967,886,831,947,507,889,984,751,942,996,971,890,509,949,973,1000,892,950,863,759,1008,510,979,953,763,974,954,879,981,982,927,995,765,956,887,985,997,986,943,891,998,766,511,988,1001,951,1002,893,975,894,1009,955,1004,1010,957,983,958,987,1012,999,1016,767,989,1003,990,1005,959,1011,1013,895,1006,1014,1017,1018,991,1020,1007,1015,1019,1021,1022,1023 };

        _Q = Q_INIT;

        int cnt = 0;
        for (const int q : _Q) {
            if (q < static_cast<int>(_block_length)) {
                _channel_order_descending[_block_length - 1 - cnt] = q;
                _channel_order_ascending[cnt] = q;
                ++cnt;
            }
        }

        if (_block_length == 1024 && (_info_length + _crc_size) == 512) {
            _nodes_type = {
                {0, 63, NodeType::R0},
                {64, 127, NodeType::REP},
                {128, 191, NodeType::REP},
                {192, 831, NodeType::NONE},
                {832, 895, NodeType::SPC},
                {896, 959, NodeType::SPC},
                {960, 1023, NodeType::R1}
            };
        }
        else {
            _nodes_type = { {0, static_cast<int>(_block_length) - 1, NodeType::NONE} };
        }
    }

    uint32_t effective_info_length = _info_length + _crc_size;

    // 设置冻结比特，使用 operator[] 替代 at()
    for (uint32_t i = 0; i < effective_info_length; ++i) {
        _frozen_bits[_channel_order_descending[i]] = 0;
    }
    for (uint32_t i = effective_info_length; i < _block_length; ++i) {
        _frozen_bits[_channel_order_descending[i]] = 1;
    }

    // 使用现代随机数生成器
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1);
    std::uniform_int_distribution<int> pos_dist(0, _info_length - 1);
    _crc_matrix.resize(_crc_size, std::vector<uint8_t>(_info_length, 0));

    for (auto& row : _crc_matrix) {
        bool all_zero = true;
        // 生成随机位
        for (auto& bit : row) {
            bit = static_cast<uint8_t>(dist(rng));
            if (bit != 0) {
                all_zero = false;
            }
        }
        // 如果该行全为0，随机选择一个位置设置为1
        if (all_zero) {
            int pos = pos_dist(rng);
            row[pos] = 1;
        }
    }
}

/*
void PolarCode::initialize_frozen_bits() {
    _channel_order_descending.resize(_block_length);
    _channel_order_ascending.resize(_block_length);

    if (!_5G) {
        std::vector<double> channel_vec(_block_length);
        for (uint32_t i = 0; i < _block_length; ++i) {
            channel_vec.at(i) = _design_epsilon;
        }
        for (uint8_t iteration = 0; iteration < _n; ++iteration) {
            uint32_t  increment = 1 << iteration;
            for (uint32_t j = 0; j < increment; j += 1) {
                for (uint32_t i = 0; i < _block_length; i += 2 * increment) {
                    double c1 = channel_vec.at(i + j);
                    double c2 = channel_vec.at(i + j + increment);
                    channel_vec.at(i + j) = c1 + c2 - c1 * c2;
                    channel_vec.at(i + j + increment) = c1 * c2;
                }
            }
        }
        std::size_t n_t(0);
        std::generate(std::begin(_channel_order_descending), std::end(_channel_order_descending), [&] {
            return n_t++; });
        n_t = 0;
        std::generate(std::begin(_channel_order_ascending), std::end(_channel_order_ascending), [&] {
            return n_t++; });

        if (_RM) {
            // 记录行重
            std::vector<int> weights(_block_length);
            weights[0] = 1;
            weights[1] = 2;
            for (size_t w = 1; w < log2(_block_length); w++) {
                int temp = pow(2, w);
                for (size_t v = temp; v < temp * 2; v++) {
                    weights[v] = weights[v - temp] * 2;
                }
            }

            // 注意巴氏参数构造要反转比特，因为是_design_epsilon是awgn的参数往里面算，而编码是从里面往外面编码
            // 编码的图刚好是反着的，所以要比特翻转
            // RM码不需要翻转
            std::sort(std::begin(_channel_order_descending),
                std::end(_channel_order_descending),
                [&](int i1, int i2) {
                    if (weights[i1] != weights[i2]) {
                        return weights[i1] > weights[i2];
                    }
                    else {
                        return channel_vec[_bit_rev_order[i1]] < channel_vec[_bit_rev_order[i2]];
                    }
                });

            std::sort(std::begin(_channel_order_ascending),
                std::end(_channel_order_ascending),
                [&](int i1, int i2) {
                    if (weights[i1] != weights[i2]) {
                        return weights[i1] < weights[i2];
                    }
                    else {
                        return channel_vec[_bit_rev_order[i1]] > channel_vec[_bit_rev_order[i2]];
                    }
                });

        }
        else {
            std::sort(std::begin(_channel_order_descending),
                std::end(_channel_order_descending),
                [&](int i1, int i2) { return channel_vec[_bit_rev_order.at(i1)] < channel_vec[_bit_rev_order.at(i2)]; });

            std::sort(std::begin(_channel_order_ascending),
                std::end(_channel_order_ascending),
                [&](int i1, int i2) {
                    return channel_vec[_bit_rev_order.at(i1)] > channel_vec[_bit_rev_order.at(i2)];
                });
        }

    }
    else if (_5G)
    {
        _Q = { 0,1,2,4,8,16,32,3,5,64,9,6,17,10,18,128,12,33,65,20,256,34,24,36,7,129,66,512,11,40,68,130,19,13,48,14,72,257,21,132,35,258,26,513,80,37,25,22,136,260,264,38,514,96,67,41,144,28,69,42,516,49,74,272,160,520,288,528,192,544,70,44,131,81,50,73,15,320,133,52,23,134,384,76,137,82,56,27,97,39,259,84,138,145,261,29,43,98,515,88,140,30,146,71,262,265,161,576,45,100,640,51,148,46,75,266,273,517,104,162,53,193,152,77,164,768,268,274,518,54,83,57,521,112,135,78,289,194,85,276,522,58,168,139,99,86,60,280,89,290,529,524,196,141,101,147,176,142,530,321,31,200,90,545,292,322,532,263,149,102,105,304,296,163,92,47,267,385,546,324,208,386,150,153,165,106,55,328,536,577,548,113,154,79,269,108,578,224,166,519,552,195,270,641,523,275,580,291,59,169,560,114,277,156,87,197,116,170,61,531,525,642,281,278,526,177,293,388,91,584,769,198,172,120,201,336,62,282,143,103,178,294,93,644,202,592,323,392,297,770,107,180,151,209,284,648,94,204,298,400,608,352,325,533,155,210,305,547,300,109,184,534,537,115,167,225,326,306,772,157,656,329,110,117,212,171,776,330,226,549,538,387,308,216,416,271,279,158,337,550,672,118,332,579,540,389,173,121,553,199,784,179,228,338,312,704,390,174,554,581,393,283,122,448,353,561,203,63,340,394,527,582,556,181,295,285,232,124,205,182,643,562,286,585,299,354,211,401,185,396,344,586,645,593,535,240,206,95,327,564,800,402,356,307,301,417,213,568,832,588,186,646,404,227,896,594,418,302,649,771,360,539,111,331,214,309,188,449,217,408,609,596,551,650,229,159,420,310,541,773,610,657,333,119,600,339,218,368,652,230,391,313,450,542,334,233,555,774,175,123,658,612,341,777,220,314,424,395,673,583,355,287,183,234,125,557,660,616,342,316,241,778,563,345,452,397,403,207,674,558,785,432,357,187,236,664,624,587,780,705,126,242,565,398,346,456,358,405,303,569,244,595,189,566,676,361,706,589,215,786,647,348,419,406,464,680,801,362,590,409,570,788,597,572,219,311,708,598,601,651,421,792,802,611,602,410,231,688,653,248,369,190,364,654,659,335,480,315,221,370,613,422,425,451,614,543,235,412,343,372,775,317,222,426,453,237,559,833,804,712,834,661,808,779,617,604,433,720,816,836,347,897,243,662,454,318,675,618,898,781,376,428,665,736,567,840,625,238,359,457,399,787,591,678,434,677,349,245,458,666,620,363,127,191,782,407,436,626,571,465,681,246,707,350,599,668,790,460,249,682,573,411,803,789,709,365,440,628,689,374,423,466,793,250,371,481,574,413,603,366,468,655,900,805,615,684,710,429,794,252,373,605,848,690,713,632,482,806,427,904,414,223,663,692,835,619,472,455,796,809,714,721,837,716,864,810,606,912,722,696,377,435,817,319,621,812,484,430,838,667,488,239,378,459,622,627,437,380,818,461,496,669,679,724,841,629,351,467,438,737,251,462,442,441,469,247,683,842,738,899,670,783,849,820,728,928,791,367,901,630,685,844,633,711,253,691,824,902,686,740,850,375,444,470,483,415,485,905,795,473,634,744,852,960,865,693,797,906,715,807,474,636,694,254,717,575,913,798,811,379,697,431,607,489,866,723,486,908,718,813,476,856,839,725,698,914,752,868,819,814,439,929,490,623,671,739,916,463,843,381,497,930,821,726,961,872,492,631,729,700,443,741,845,920,382,822,851,730,498,880,742,445,471,635,932,687,903,825,500,846,745,826,732,446,962,936,475,853,867,637,907,487,695,746,828,753,854,857,504,799,255,964,909,719,477,915,638,748,944,869,491,699,754,858,478,968,383,910,815,976,870,917,727,493,873,701,931,756,860,499,731,823,922,874,918,502,933,743,760,881,494,702,921,501,876,847,992,447,733,827,934,882,937,963,747,505,855,924,734,829,965,938,884,506,749,945,966,755,859,940,830,911,871,639,888,479,946,750,969,508,861,757,970,919,875,862,758,948,977,923,972,761,877,952,495,703,935,978,883,762,503,925,878,735,993,885,939,994,980,926,764,941,967,886,831,947,507,889,984,751,942,996,971,890,509,949,973,1000,892,950,863,759,1008,510,979,953,763,974,954,879,981,982,927,995,765,956,887,985,997,986,943,891,998,766,511,988,1001,951,1002,893,975,894,1009,955,1004,1010,957,983,958,987,1012,999,1016,767,989,1003,990,1005,959,1011,1013,895,1006,1014,1017,1018,991,1020,1007,1015,1019,1021,1022,1023 };
        int cnt = 0;
        for (int i = 0; i < _Q.size(); i++) {
            if (_Q[i] < _block_length) {
                _channel_order_descending[_block_length - 1 - cnt] = _Q[i];
                _channel_order_ascending[cnt] = _Q[i];
                cnt++;
            }
        }
        if (_block_length == 1024 && _info_length + _crc_size == 512) {
            _nodes_type.push_back({ 0,63,NodeType::R0 });
            _nodes_type.push_back({ 64,127,NodeType::REP });
            _nodes_type.push_back({ 128,191,NodeType::REP });
            _nodes_type.push_back({ 192,831,NodeType::NONE });
            _nodes_type.push_back({ 832,895,NodeType::SPC });
            _nodes_type.push_back({ 896,959,NodeType::SPC });
            _nodes_type.push_back({ 960,1023,NodeType::R1 });

        }
        else {
            _nodes_type.push_back({ 0,(int)_block_length - 1, NodeType::NONE });
        }
    }

    uint32_t  effective_info_length = _info_length + _crc_size;



    for (uint32_t i = 0; i < effective_info_length; ++i) {
        _frozen_bits.at(_channel_order_descending.at(i)) = 0;
    }
    for (uint32_t i = effective_info_length; i < _block_length; ++i) {
        _frozen_bits.at(_channel_order_descending.at((i))) = 1;
    }


    _crc_matrix.resize(_crc_size);
    for (uint8_t bit = 0; bit < _crc_size; ++bit) {
        _crc_matrix.at(bit).resize(_info_length);
        for (uint32_t info_bit = 0; info_bit < _info_length; ++info_bit)
            _crc_matrix.at(bit).at(info_bit) = (uint8_t)(rand() % 2);
    }

}
*/
std::vector<uint8_t> PolarCode::sys_encode(std::vector<uint8_t> info_bits) {
    // 本质就是信息乘了GAA^-1,但是有nlogn的方法,就是两步编码
    // 需要注意的是，A所选择的数量为effective_info_length
    // 这里使用两步编码，无法使用的话，check_inverse会报错
    // 第一步编码
    uint32_t  effective_info_length = _info_length + _crc_size;
    auto info = this->encode(info_bits);
    // 非信息位填再次填0
    for (uint32_t i = effective_info_length; i < _block_length; ++i) {
        info.at(_channel_order_descending.at((i))) = 0;
    }
    // 直接过极化，不需要填了
    for (uint8_t iteration = 0; iteration < _n; ++iteration) {
        uint32_t  group_size = (uint32_t)(1 << iteration);
        for (uint32_t j = 0; j < group_size; j += 1) {
            for (uint32_t i = 0; i < _block_length; i += 2 * group_size) {
                info.at(i + j) = (uint8_t)((info.at(i + j) + info.at(i + j + group_size)) % 2);
            }
        }
    }
    return info;
}

std::vector<uint8_t> PolarCode::encode(std::vector<uint8_t> info_bits) {

    std::vector<uint8_t> info_bits_padded(_block_length, 0);
    std::vector<uint8_t> deocded_info_bits(_info_length);

    for (uint32_t i = 0; i < _info_length; ++i) {
        info_bits_padded.at(_channel_order_ascending.at(_block_length - _info_length + i)) = info_bits.at(i);
    }

    auto t = info_bits_padded;

    for (uint32_t i = _info_length; i < _info_length + _crc_size; ++i) {
        uint8_t  crc_bit = 0;
        for (uint32_t j = 0; j < _info_length; ++j) {
            crc_bit = (uint8_t)((crc_bit + _crc_matrix.at(i - _info_length).at(j) * info_bits.at(j)) % 2);
        }
        info_bits_padded.at(_channel_order_ascending.at(_block_length - _info_length - _crc_size + i - _info_length)) = crc_bit;
    }

    for (uint8_t iteration = 0; iteration < _n; ++iteration) {
        uint32_t  group_size = (uint32_t)(1 << iteration);
        for (uint32_t j = 0; j < group_size; j += 1) {
            for (uint32_t i = 0; i < _block_length; i += 2 * group_size) {
                info_bits_padded.at(i + j) =
                    (uint8_t)((info_bits_padded.at(i + j) + info_bits_padded.at(i + j + group_size)) % 2);
            }
        }
    }

    return info_bits_padded;

}

void PolarCode::info_return(uint8_t* info_bit_padded) {
    std::vector<uint8_t> deocded_info_bits(_info_length + _crc_size);
    for (uint32_t beta = 0; beta < _info_length + _crc_size; ++beta) {
        deocded_info_bits.at(beta) = info_bit_padded[_channel_order_ascending.at(_block_length - _info_length - _crc_size + beta)];
    }

    for (uint32_t i = 0; i < _block_length; i++) {
        info_bit_padded[i] = 0;
    }

    for (uint32_t i = 0; i < _info_length + _crc_size; ++i) {
        info_bit_padded[_channel_order_ascending.at(_block_length - _info_length - _crc_size + i)] = deocded_info_bits.at(i);
    }

    for (uint8_t iteration = 0; iteration < _n; ++iteration) {
        uint32_t  group_size = (uint32_t)(1 << iteration);
        for (uint32_t j = 0; j < group_size; j += 1) {
            for (uint32_t i = 0; i < _block_length; i += 2 * group_size) {
                info_bit_padded[i + j] = (uint8_t)((info_bit_padded[i + j] + info_bit_padded[i + j + group_size]) % 2);
            }
        }
    }
}

bool PolarCode::crc_check(uint8_t* info_bit_padded) {

    bool crc_pass = true;

    for (uint32_t i = _info_length; i < _info_length + _crc_size; ++i) {

        uint8_t  crc_bit = 0;

        for (uint32_t j = 0; j < _info_length; ++j) {

            crc_bit = (uint8_t)((crc_bit + _crc_matrix.at(i - _info_length).at(j) * info_bit_padded[_channel_order_ascending.at(_block_length - _info_length + j)]) % 2);

        }

        if (crc_bit != info_bit_padded[_channel_order_ascending.at(_block_length - _info_length - _crc_size + i - _info_length)]) {

            crc_pass = false;
            break;

        }
    }

    return crc_pass;
}

std::vector<uint8_t> PolarCode::decode_scl_p1(std::vector<double> p1, std::vector<double> p0, uint32_t list_size) {

    _list_size = list_size;
    _llr_based_computation = false;

    initializeDataStructures();

    uint32_t  l = assignInitialPath();

    double* p_0 = getArrayPointer_P(0, l);

    for (uint32_t beta = 0; beta < _block_length; ++beta) {
        p_0[2 * beta] = (double)p0.at(beta);
        p_0[2 * beta + 1] = (double)p1.at(beta);
    }

    return decode_scl();

}

std::vector<uint8_t> PolarCode::decode_scl_llr(std::vector<double> llr, uint32_t list_size) {

    _list_size = list_size;

    _llr_based_computation = true;

    double* llr_0 = nullptr;
    uint32_t  l;

    initializeDataStructures();

    l = assignInitialPath();

    llr_0 = getArrayPointer_LLR(0, l);

    for (uint32_t beta = 0; beta < _block_length; ++beta) {
        llr_0[beta] = llr.at(beta);
    }

    return decode_scl();
    /*return decode_fast_scl();*/

}

std::vector<uint8_t> PolarCode::decode_scl() {

    for (uint32_t phi = 0; phi < _block_length; ++phi) {

        // 计算LLR
        if (_llr_based_computation)
            recursivelyCalcLLR(_n, phi);
        else
            recursivelyCalcP(_n, phi);

        // 译码+复制路径
        if (_frozen_bits.at(phi) == 1)
            continuePaths_FrozenBit(phi);
        else
            continuePaths_UnfrozenBit(phi);

        if ((phi % 2) == 1)
            recursivelyUpdateC(_n, phi);

    }


    uint32_t l = findMostProbablePath((bool)_crc_size);

    // 码字
    uint8_t* c_0 = _arrayPointer_Info.at(l);

    // 提取信息
    std::vector<uint8_t> deocded_info_bits(_info_length);
    for (uint32_t beta = 0; beta < _info_length; ++beta) {
        deocded_info_bits.at(beta) = c_0[_channel_order_ascending.at(_block_length - _info_length + beta)];
    }

    if (!_memory) {
        // 释放内存，可修正为内存池避免释放
        for (uint32_t s = 0; s < _list_size; ++s) {
            delete[] _arrayPointer_Info.at(s);
            for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {

                if (_llr_based_computation)
                    delete[] _arrayPointer_LLR.at(lambda).at(s);
                else
                    delete[] _arrayPointer_P.at(lambda).at(s);
                delete[] _arrayPointer_C.at(lambda).at(s);
            }
        }
    }

    return deocded_info_bits;

}

class Cmp {
public:
    bool operator()(const std::pair<double, size_t>& a, const std::pair<double, size_t>& b) {
        return a.first < b.first;
    }
};

std::vector<uint8_t> PolarCode::decode_fast_scl() {


    for (uint32_t cnt = 0; cnt < _nodes_type.size(); cnt++) {
        // 快速节点的意义在于，对下给出答案，对上给出码字
        if (_nodes_type[cnt].type == NodeType::R0) {
            int begin = _nodes_type[cnt].begin;
            int end = _nodes_type[cnt].end;
            int sub_code_layer_num = log2(end - begin + 1);
            int parent = end / (pow(2, sub_code_layer_num));
            int layer = _n - sub_code_layer_num;
            recursivelyCalcLLR(layer, parent);
            int left_right = parent % 2;

            for (uint32_t l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0)
                    continue;
                /*double* llr_lambda = getArrayPointer_LLR(layer, l);*/
                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint32_t group_size = (1 << (_n - layer));
                for (size_t beta = 0; beta < group_size; beta++) {
                    c_lambda[beta + left_right * group_size] = 0;
                    _arrayPointer_Info.at(l)[begin + beta] = 0;
                }

            }

            recursivelyUpdateC(layer, parent);

        }
        else if (_nodes_type[cnt].type == NodeType::R1) {


            int begin = _nodes_type[cnt].begin;
            int end = _nodes_type[cnt].end;
            int sub_code_length = end - begin + 1;
            int sub_code_layer_num = log2(sub_code_length);
            int parent = end / (pow(2, sub_code_layer_num));
            int left_right = parent % 2;
            int layer = _n - sub_code_layer_num;
            recursivelyCalcLLR(layer, parent);

            std::vector<double>  probForks((unsigned long)(2 * _list_size));
            std::vector<double> probabilities;
            std::vector<uint8_t>  contForks((unsigned long)(2 * _list_size));
            uint32_t  i = 0;

            std::vector<int> pos(_list_size);
            std::vector<double> minns(_list_size, std::numeric_limits<double>::max());

            uint32_t group_size = (1 << (_n - layer));

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0) {

                    probForks.at(2 * l) = NAN;
                    probForks.at(2 * l + 1) = NAN;

                }
                else {

                    double* llr_p = getArrayPointer_LLR(layer, l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l);

                    probForks.at(2 * l) = -_pathMetric_LLR.at(l);
                    probForks.at(2 * l + 1) = -_pathMetric_LLR.at(l);
                    for (size_t beta = 0; beta < group_size; beta++) {
                        c_lambda[beta + left_right * group_size] = (uint8_t)(llr_p[beta] < 0);
                        probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                    }
                    probForks.at(2 * l + 1) = probForks.at(2 * l);

                    for (size_t beta = 0; beta < sub_code_length; beta++) {
                        if (abs(minns[l]) > abs(llr_p[beta])) {
                            minns[l] = llr_p[beta];
                            pos[l] = beta;
                        }
                    }

                    if (c_lambda[pos[l] + left_right * group_size] == 0) {
                        probForks.at(2 * l + 1) += exp(-minns[l]);
                        probForks.at(2 * l + 1) -= exp(minns[l]);
                    }
                    else {
                        probForks.at(2 * l) += exp(minns[l]);
                        probForks.at(2 * l) -= exp(-minns[l]);
                    }

                    probabilities.push_back(probForks.at(2 * l));
                    probabilities.push_back(probForks.at(2 * l + 1));

                    i++;

                }
            }

            uint32_t  rho = _list_size;

            if ((2 * i) < _list_size)
                rho = (uint32_t)2 * i;

            for (uint32_t l = 0; l < 2 * _list_size; ++l)
                contForks.at(l) = 0;

            // 从大到小排序
            std::sort(probabilities.begin(), probabilities.end(), std::greater<double>());

            double threshold = probabilities.at((unsigned long)(rho - 1));
            uint32_t num_paths_continued = 0;

            // 选择存活的路径
            for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                if (probForks.at(l) > threshold) {

                    contForks.at(l) = 1;
                    num_paths_continued++;

                }
                if (num_paths_continued == rho) {
                    break;
                }

            }

            // 小概率事件：存在度量值相等的情况。极端----所有度量值都一样，此时找不到大于门限的路径，则等于门限的路径也加入。
            if (num_paths_continued < rho) {

                for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                    if (probForks.at(l) == threshold) {

                        contForks.at(l) = 1;
                        num_paths_continued++;

                    }

                    if (num_paths_continued == rho)
                        break;

                }

            }

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0)
                    continue;

                // 需要舍弃的路径
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    killPath(l);

            }


            for (unsigned l = 0; l < _list_size; ++l) {

                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    continue;

                uint8_t* c_lambda = getArrayPointer_C(layer, l);

                if (contForks.at(2 * l) == 1 && contForks.at(2 * l + 1) == 1) {

                    // 路径l为判为0的路径
                    c_lambda[pos[l] + left_right * group_size] = 0;

                    // 复制路径l到l_p
                    uint32_t l_p = clonePath(l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l_p);
                    c_lambda[pos[l] + left_right * group_size] = 1;

                    // l路径复制到l_p
                    std::copy(_arrayPointer_Info.at(l), _arrayPointer_Info.at(l) + begin, _arrayPointer_Info.at(l_p));

                    _pathMetric_LLR.at(l) = -probForks.at(2 * l);
                    _pathMetric_LLR.at(l_p) = -probForks.at(2 * l + 1);

                }
                else {

                    // 译为0的路径存活
                    if (contForks.at(2 * l) == 1) {

                        c_lambda[pos[l] + left_right * group_size] = 0;

                        _pathMetric_LLR.at(l) = -probForks.at(2 * l);

                    }
                    // 译为1的路径存活
                    else {

                        c_lambda[pos[l] + left_right * group_size] = 1;

                        _pathMetric_LLR.at(l) = -probForks.at(2 * l + 1);

                    }

                }

            }


            for (uint32_t l = 0; l < _list_size; ++l) {

                // 如果路径激活
                if (_activePath.at(l) == 0)
                    continue;
                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint8_t* c_0 = _arrayPointer_Info.at(l);

                for (size_t beta = 0; beta < group_size; beta++) {
                    // 用于后续硬判断，c_0*G*G=c_0,此时c_0是c_0*G
                    c_0[begin + beta] = c_lambda[beta + left_right * group_size];
                }


                //_thread_pool.enqueue([c_0, sub_code_layer_num, sub_code_length, begin] {
                //    // 码字硬判成结果，既再乘一个G矩阵
                //    for (uint8_t iteration = 0; iteration < sub_code_layer_num; ++iteration) {
                //        uint32_t  group_size = (uint32_t)(1 << iteration);
                //        for (uint32_t j = 0; j < group_size; j += 1) {
                //            for (uint32_t i = 0; i < sub_code_length; i += 2 * group_size) {
                //                c_0[begin + i + j] =
                //                    (uint8_t)((c_0[begin + i + j] + c_0[begin + i + j + group_size]) % 2);
                //            }
                //        }
                //    }
                //    });

                for (uint8_t iteration = 0; iteration < sub_code_layer_num; ++iteration) {
                    uint32_t  group_size = (uint32_t)(1 << iteration);
                    for (uint32_t j = 0; j < group_size; j += 1) {
                        for (uint32_t i = 0; i < sub_code_length; i += 2 * group_size) {
                            c_0[begin + i + j] =
                                (uint8_t)((c_0[begin + i + j] + c_0[begin + i + j + group_size]) % 2);
                        }
                    }
                }

            }

            recursivelyUpdateC(layer, parent);

        }
        else if (_nodes_type[cnt].type == NodeType::REP) {
            int begin = _nodes_type[cnt].begin;
            int end = _nodes_type[cnt].end;
            int sub_code_length = end - begin + 1;
            int sub_code_layer_num = log2(sub_code_length);
            int parent = end / (pow(2, sub_code_layer_num));
            int left_right = parent % 2;
            int layer = _n - sub_code_layer_num;
            recursivelyCalcLLR(layer, parent);
            std::vector<double>  probForks((unsigned long)(2 * _list_size));
            std::vector<double> probabilities;
            std::vector<uint8_t>  contForks((unsigned long)(2 * _list_size));
            uint32_t  i = 0;
            uint32_t group_size = (1 << (_n - layer));

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0) {

                    probForks.at(2 * l) = NAN;
                    probForks.at(2 * l + 1) = NAN;

                }
                else {

                    // 获取结果的似然值
                    double* llr_p = getArrayPointer_LLR(layer, l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l);

                    // 首先是个重复码
                    double sum = 0;
                    for (size_t beta = 0; beta < group_size; beta++) {
                        sum += llr_p[beta];
                    }
                    // 计算PM
                    probForks.at(2 * l) = -_pathMetric_LLR.at(l);
                    probForks.at(2 * l + 1) = -_pathMetric_LLR.at(l);
                    for (size_t beta = 0; beta < group_size; beta++) {
                        c_lambda[beta + left_right * group_size] = (uint8_t)(sum < 0);
                        probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                    }

                    probabilities.push_back(probForks.at(2 * l));
                    probabilities.push_back(probForks.at(2 * l + 1));

                    i++;

                }
            }

            uint32_t  rho = _list_size;

            if ((2 * i) < _list_size)
                rho = (uint32_t)2 * i;

            for (uint32_t l = 0; l < 2 * _list_size; ++l)
                contForks.at(l) = 0;

            // 从大到小排序
            std::sort(probabilities.begin(), probabilities.end(), std::greater<double>());

            double threshold = probabilities.at((unsigned long)(rho - 1));
            uint32_t num_paths_continued = 0;

            // 选择存活的路径
            for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                if (probForks.at(l) > threshold) {

                    contForks.at(l) = 1;
                    num_paths_continued++;

                }
                if (num_paths_continued == rho) {
                    break;
                }

            }

            // 小概率事件：存在度量值相等的情况。极端----所有度量值都一样，此时找不到大于门限的路径，则等于门限的路径也加入。
            if (num_paths_continued < rho) {

                for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                    if (probForks.at(l) == threshold) {

                        contForks.at(l) = 1;
                        num_paths_continued++;

                    }

                    if (num_paths_continued == rho)
                        break;

                }

            }

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0)
                    continue;

                // 需要舍弃的路径
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    killPath(l);

            }


            for (unsigned l = 0; l < _list_size; ++l) {

                // 没选上
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    continue;

                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint8_t tmp = c_lambda[0 + left_right * group_size];

                // 都存活了
                if (contForks.at(2 * l) == 1 && contForks.at(2 * l + 1) == 1) {

                    // 复制路径l到l_p
                    uint32_t l_p = clonePath(l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l_p);

                    for (size_t beta = 0; beta < group_size; beta++) {
                        c_lambda[beta + left_right * group_size] = 1 - tmp;
                    }

                    // l路径复制到l_p
                    std::copy(_arrayPointer_Info.at(l), _arrayPointer_Info.at(l) + begin, _arrayPointer_Info.at(l_p));

                    _pathMetric_LLR.at(l) = -probForks.at(2 * l);
                    _pathMetric_LLR.at(l_p) = -probForks.at(2 * l + 1);

                }
                else {

                    // 译为0的路径存活
                    if (contForks.at(2 * l) == 1) {

                        _pathMetric_LLR.at(l) = -probForks.at(2 * l);

                    }
                    // 译为1的路径存活
                    else {

                        for (size_t beta = 0; beta < group_size; beta++) {
                            c_lambda[beta + left_right * group_size] = 1 - tmp;
                        }
                        _pathMetric_LLR.at(l) = -probForks.at(2 * l + 1);

                    }

                }

            }


            for (uint32_t l = 0; l < _list_size; ++l) {

                // 如果路径激活
                if (_activePath.at(l) == 0)
                    continue;
                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint8_t* c_0 = _arrayPointer_Info.at(l);

                for (size_t beta = 0; beta < group_size - 1; beta++) {
                    c_0[begin + beta] = 0;
                }
                c_0[begin + group_size - 1] = c_lambda[0 + left_right * group_size];

            }

            recursivelyUpdateC(layer, parent);

        }
        else if (_nodes_type[cnt].type == NodeType::SPC) {
            int begin = _nodes_type[cnt].begin;
            int end = _nodes_type[cnt].end;
            int sub_code_length = end - begin + 1;
            int sub_code_layer_num = log2(sub_code_length);
            int parent = end / (pow(2, sub_code_layer_num));
            int left_right = parent % 2;
            int layer = _n - sub_code_layer_num;
            recursivelyCalcLLR(layer, parent);
            std::vector<double>  probForks((unsigned long)(2 * _list_size));
            std::vector<double> probabilities;
            std::vector<uint8_t>  contForks((unsigned long)(2 * _list_size));
            uint32_t  i = 0;
            std::vector<int> pos(_list_size);
            std::vector<double> minns(_list_size, std::numeric_limits<double>::max());
            uint32_t group_size = (1 << (_n - layer));
            std::vector<size_t> pos_min3(_list_size);
            std::vector <size_t> pos_min2(_list_size);
            std::vector <size_t> pos_min1(_list_size);
            std::vector<double> check(_list_size, 1);

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0) {

                    probForks.at(2 * l) = NAN;
                    probForks.at(2 * l + 1) = NAN;

                }
                else {

                    // 获取结果的似然值
                    double* llr_p = getArrayPointer_LLR(layer, l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l);

                    // 首先是个奇偶校验码
                    std::priority_queue<std::pair<double, size_t>, std::vector<std::pair<double, size_t>>, Cmp> minn;
                    for (size_t beta = 0; beta < group_size; beta++) {
                        check[l] *= llr_p[beta];
                        minn.push({ std::abs(llr_p[beta]) ,beta });
                        if (minn.size() > 3)minn.pop();
                    }
                    pos_min3[l] = minn.top().second; minn.pop();
                    pos_min2[l] = minn.top().second; minn.pop();
                    pos_min1[l] = minn.top().second; minn.pop();

                    // 计算PM
                    probForks.at(2 * l) = -_pathMetric_LLR.at(l);
                    probForks.at(2 * l + 1) = -_pathMetric_LLR.at(l);

                    if (check[l] > 0) {
                        // 过了奇偶校验
                        for (size_t beta = 0; beta < group_size; beta++) {
                            c_lambda[beta + left_right * group_size] = (uint8_t)(llr_p[beta] < 0);
                            probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                        }
                        // 修改两个
                        probForks.at(2 * l + 1) = probForks.at(2 * l);
                        probForks.at(2 * l + 1) += log(1 + exp(-(1 - 2 * c_lambda[pos_min1[l] + left_right * group_size]) * llr_p[pos_min1[l]]));
                        probForks.at(2 * l + 1) += log(1 + exp(-(1 - 2 * c_lambda[pos_min2[l] + left_right * group_size]) * llr_p[pos_min2[l]]));

                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[pos_min1[l] + left_right * group_size]) * llr_p[pos_min1[l]]));
                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[pos_min2[l] + left_right * group_size]) * llr_p[pos_min2[l]]));
                    }
                    else {
                        for (size_t beta = 0; beta < group_size; beta++) {
                            if (beta != pos_min1[l]) {
                                c_lambda[beta + left_right * group_size] = (uint8_t)(llr_p[beta] < 0);
                                probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                            }
                            else {
                                c_lambda[beta + left_right * group_size] = 1 - (uint8_t)(llr_p[beta] < 0);
                                probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                            }
                        }
                        // 修改两个
                        probForks.at(2 * l + 1) = probForks.at(2 * l);
                        probForks.at(2 * l + 1) += log(1 + exp(-(1 - 2 * c_lambda[pos_min3[l] + left_right * group_size]) * llr_p[pos_min3[l]]));
                        probForks.at(2 * l + 1) += log(1 + exp(-(1 - 2 * c_lambda[pos_min2[l] + left_right * group_size]) * llr_p[pos_min2[l]]));

                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[pos_min3[l] + left_right * group_size]) * llr_p[pos_min3[l]]));
                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[pos_min2[l] + left_right * group_size]) * llr_p[pos_min2[l]]));
                    }




                    probabilities.push_back(probForks.at(2 * l));
                    probabilities.push_back(probForks.at(2 * l + 1));

                    i++;

                }
            }

            uint32_t  rho = _list_size;

            if ((2 * i) < _list_size)
                rho = (uint32_t)2 * i;

            for (uint32_t l = 0; l < 2 * _list_size; ++l)
                contForks.at(l) = 0;

            // 从大到小排序
            std::sort(probabilities.begin(), probabilities.end(), std::greater<double>());

            double threshold = probabilities.at((unsigned long)(rho - 1));
            uint32_t num_paths_continued = 0;

            // 选择存活的路径
            for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                if (probForks.at(l) > threshold) {

                    contForks.at(l) = 1;
                    num_paths_continued++;

                }
                if (num_paths_continued == rho) {
                    break;
                }

            }

            // 小概率事件：存在度量值相等的情况。极端----所有度量值都一样，此时找不到大于门限的路径，则等于门限的路径也加入。
            if (num_paths_continued < rho) {

                for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                    if (probForks.at(l) == threshold) {

                        contForks.at(l) = 1;
                        num_paths_continued++;

                    }

                    if (num_paths_continued == rho)
                        break;

                }

            }

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0)
                    continue;

                // 需要舍弃的路径
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    killPath(l);

            }


            for (unsigned l = 0; l < _list_size; ++l) {

                // 没选上
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    continue;

                uint8_t* c_lambda = getArrayPointer_C(layer, l);

                // 都存活了
                if (contForks.at(2 * l) == 1 && contForks.at(2 * l + 1) == 1) {

                    // 复制路径l到l_p
                    uint32_t l_p = clonePath(l);
                    uint8_t* c_lambda1 = getArrayPointer_C(layer, l_p);

                    if (check[l] > 0) {
                        for (size_t beta = 0; beta < group_size; beta++) {
                            c_lambda1[beta + left_right * group_size] = c_lambda[beta + left_right * group_size];
                        }
                        c_lambda1[pos_min1[l] + left_right * group_size] = 1 - c_lambda[pos_min1[l] + left_right * group_size];
                        c_lambda1[pos_min2[l] + left_right * group_size] = 1 - c_lambda[pos_min2[l] + left_right * group_size];
                    }
                    else {
                        for (size_t beta = 0; beta < group_size; beta++) {
                            c_lambda1[beta + left_right * group_size] = c_lambda[beta + left_right * group_size];
                        }
                        c_lambda1[pos_min1[l] + left_right * group_size] = 1 - c_lambda[pos_min1[l] + left_right * group_size];
                        c_lambda1[pos_min2[l] + left_right * group_size] = 1 - c_lambda[pos_min2[l] + left_right * group_size];
                        c_lambda1[pos_min3[l] + left_right * group_size] = 1 - c_lambda[pos_min3[l] + left_right * group_size];
                    }

                    // l路径复制到l_p
                    std::copy(_arrayPointer_Info.at(l), _arrayPointer_Info.at(l) + begin, _arrayPointer_Info.at(l_p));

                    _pathMetric_LLR.at(l) = -probForks.at(2 * l);
                    _pathMetric_LLR.at(l_p) = -probForks.at(2 * l + 1);

                }
                else {

                    // 译为0的路径存活
                    if (contForks.at(2 * l) == 1) {

                        _pathMetric_LLR.at(l) = -probForks.at(2 * l);

                    }
                    // 译为1的路径存活
                    else {

                        if (check[l] > 0) {
                            c_lambda[pos_min1[l] + left_right * group_size] = 1 - c_lambda[pos_min1[l] + left_right * group_size];
                            c_lambda[pos_min2[l] + left_right * group_size] = 1 - c_lambda[pos_min2[l] + left_right * group_size];
                        }
                        else {
                            c_lambda[pos_min1[l] + left_right * group_size] = 1 - c_lambda[pos_min1[l] + left_right * group_size];
                            c_lambda[pos_min2[l] + left_right * group_size] = 1 - c_lambda[pos_min2[l] + left_right * group_size];
                            c_lambda[pos_min3[l] + left_right * group_size] = 1 - c_lambda[pos_min3[l] + left_right * group_size];
                        }

                        _pathMetric_LLR.at(l) = -probForks.at(2 * l + 1);

                    }

                }

            }


            for (uint32_t l = 0; l < _list_size; ++l) {
                // 如果路径激活
                if (_activePath.at(l) == 0)
                    continue;
                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint8_t* c_0 = _arrayPointer_Info.at(l);

                for (size_t beta = 0; beta < group_size; beta++) {
                    c_0[begin + beta] = c_lambda[beta + left_right * group_size];
                }
                /*拿个线程硬判，计算(c_0*G)*G*/
                _thread_pool.enqueue([c_0, sub_code_layer_num, sub_code_length, begin] {
                    // 码字硬判成结果，既再乘一个G矩阵
                    for (uint8_t iteration = 0; iteration < sub_code_layer_num; ++iteration) {
                        uint32_t  group_size = (uint32_t)(1 << iteration);
                        for (uint32_t j = 0; j < group_size; j += 1) {
                            for (uint32_t i = 0; i < sub_code_length; i += 2 * group_size) {
                                c_0[begin + i + j] =
                                    (uint8_t)((c_0[begin + i + j] + c_0[begin + i + j + group_size]) % 2);
                            }
                        }
                    }
                    });

                /*for (uint8_t iteration = 0; iteration < sub_code_layer_num; ++iteration) {
                    uint32_t  group_size = (uint32_t)(1 << iteration);
                    for (uint32_t j = 0; j < group_size; j += 1) {
                        for (uint32_t i = 0; i < sub_code_length; i += 2 * group_size) {
                            c_0[begin + i + j] =
                                (uint8_t)((c_0[begin + i + j] + c_0[begin + i + j + group_size]) % 2);
                        }
                    }
                }*/
                /*std::cout << 'stop';*/

            }

            recursivelyUpdateC(layer, parent);
        }
        else if (_nodes_type[cnt].type == NodeType::PCR) {
            int begin = _nodes_type[cnt].begin;
            int end = _nodes_type[cnt].end;
            int sub_code_length = end - begin + 1;
            int sub_code_layer_num = log2(sub_code_length);
            int parent = end / (pow(2, sub_code_layer_num));
            int left_right = parent % 2;
            int layer = _n - sub_code_layer_num;
            recursivelyCalcLLR(layer, parent);
            std::vector<double>  probForks((unsigned long)(2 * _list_size));
            std::vector<double> probabilities;
            std::vector<uint8_t>  contForks((unsigned long)(2 * _list_size));
            uint32_t  i = 0;
            std::vector <size_t> pos_min3(_list_size);
            std::vector <size_t> pos_min2(_list_size);
            std::vector <size_t> pos_min1(_list_size);
            /* std::vector<int> pos(_list_size);
             std::vector<double> minns(_list_size, std::numeric_limits<double>::max());*/
            uint32_t group_size = (1 << (_n - layer));

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0) {

                    probForks.at(2 * l) = NAN;
                    probForks.at(2 * l + 1) = NAN;

                }
                else {

                    double* llr_p = getArrayPointer_LLR(layer, l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l);
                    std::vector<double> sum(4, 0);

                    std::priority_queue<std::pair<double, size_t>, std::vector<std::pair<double, size_t>>, Cmp> minn;

                    for (size_t beta = 0; beta < group_size; beta += 4) {

                        sum[0] += llr_p[beta];
                        sum[1] += llr_p[beta + 1];
                        sum[2] += llr_p[beta + 2];
                        sum[3] += llr_p[beta + 3];

                    }

                    minn.push({ std::abs(sum[0]) ,0 });
                    minn.push({ std::abs(sum[1]) ,1 });
                    minn.push({ std::abs(sum[2]) ,2 });
                    minn.push({ std::abs(sum[3]) ,3 });
                    minn.pop();
                    pos_min3[l] = minn.top().second; minn.pop();
                    pos_min2[l] = minn.top().second; minn.pop();
                    pos_min1[l] = minn.top().second; minn.pop();

                    double check = sum[0] * sum[1] * sum[2] * sum[3];

                    probForks.at(2 * l) = -_pathMetric_LLR.at(l);
                    probForks.at(2 * l + 1) = -_pathMetric_LLR.at(l);

                    for (size_t beta = 0; beta < group_size; beta += 4) {

                        c_lambda[beta + left_right * group_size] = (uint8_t)(sum[0] < 0);
                        c_lambda[beta + left_right * group_size + 1] = (uint8_t)(sum[1] < 0);
                        c_lambda[beta + left_right * group_size + 2] = (uint8_t)(sum[2] < 0);
                        c_lambda[beta + left_right * group_size + 3] = (uint8_t)(sum[3] < 0);

                        if (check < 0) {
                            c_lambda[beta + left_right * group_size + pos_min1[l]] = (uint8_t)(sum[pos_min1[l]] > 0);
                        }

                        probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                        probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + 1]) * llr_p[beta + 1]));
                        probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + 2]) * llr_p[beta + 2]));
                        probForks.at(2 * l) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + 3]) * llr_p[beta + 3]));

                        /*if (check < 0) {
                            probForks.at(2 * l) -= log(1 + exp((1 - 2 * c_lambda[beta + left_right * group_size + pos_min1[l]]) * llr_p[beta + pos_min1[l]]));
                        }*/

                        probForks.at(2 * l + 1) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size]) * llr_p[beta]));
                        probForks.at(2 * l + 1) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + 1]) * llr_p[beta + 1]));
                        probForks.at(2 * l + 1) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + 2]) * llr_p[beta + 2]));
                        probForks.at(2 * l + 1) -= log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + 3]) * llr_p[beta + 3]));


                        // 反转两个保持check
                        probForks.at(2 * l + 1) += log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + pos_min2[l]]) * llr_p[beta + pos_min2[l]]));
                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[beta + left_right * group_size + pos_min2[l]]) * llr_p[beta + pos_min2[l]]));

                        probForks.at(2 * l + 1) += log(1 + exp(-(1 - 2 * c_lambda[beta + left_right * group_size + pos_min3[l]]) * llr_p[beta + pos_min3[l]]));
                        probForks.at(2 * l + 1) -= log(1 + exp((1 - 2 * c_lambda[beta + left_right * group_size + pos_min3[l]]) * llr_p[beta + pos_min3[l]]));

                    }

                    probabilities.push_back(probForks.at(2 * l));
                    probabilities.push_back(probForks.at(2 * l + 1));
                    i++;

                }

            }

            uint32_t  rho = _list_size;

            if ((2 * i) < _list_size)
                rho = (uint32_t)2 * i;

            for (uint32_t l = 0; l < 2 * _list_size; ++l)
                contForks.at(l) = 0;

            // 从大到小排序
            std::sort(probabilities.begin(), probabilities.end(), std::greater<double>());
            double threshold = probabilities.at((unsigned long)(rho - 1));
            uint32_t num_paths_continued = 0;

            // 选择存活的路径
            for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                if (probForks.at(l) > threshold) {

                    contForks.at(l) = 1;
                    num_paths_continued++;

                }
                if (num_paths_continued == rho) {
                    break;
                }

            }

            // 小概率事件：存在度量值相等的情况。极端----所有度量值都一样，此时找不到大于门限的路径，则等于门限的路径也加入。
            if (num_paths_continued < rho) {

                for (uint32_t l = 0; l < 2 * _list_size; ++l) {

                    if (probForks.at(l) == threshold) {

                        contForks.at(l) = 1;
                        num_paths_continued++;

                    }

                    if (num_paths_continued == rho)
                        break;

                }

            }

            for (unsigned l = 0; l < _list_size; ++l) {

                if (_activePath.at(l) == 0)
                    continue;

                // 需要舍弃的路径
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    killPath(l);

            }


            for (unsigned l = 0; l < _list_size; ++l) {

                // 没选上
                if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
                    continue;

                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint8_t tmp1 = c_lambda[0 + left_right * group_size + pos_min2[l]];
                uint8_t tmp2 = c_lambda[0 + left_right * group_size + pos_min3[l]];

                // 都存活了
                if (contForks.at(2 * l) == 1 && contForks.at(2 * l + 1) == 1) {

                    // 复制路径l到l_p
                    uint32_t l_p = clonePath(l);
                    uint8_t* c_lambda = getArrayPointer_C(layer, l_p);

                    for (size_t beta = 0; beta < group_size; beta += 4) {
                        c_lambda[beta + left_right * group_size + pos_min2[l]] = 1 - tmp1;
                        c_lambda[beta + left_right * group_size + pos_min3[l]] = 1 - tmp2;
                    }

                    // l路径复制到l_p
                    std::copy(_arrayPointer_Info.at(l), _arrayPointer_Info.at(l) + begin, _arrayPointer_Info.at(l_p));

                    _pathMetric_LLR.at(l) = -probForks.at(2 * l);
                    _pathMetric_LLR.at(l_p) = -probForks.at(2 * l + 1);

                }
                else {

                    // 译为0的路径存活
                    if (contForks.at(2 * l) == 1) {

                        _pathMetric_LLR.at(l) = -probForks.at(2 * l);

                    }
                    // 译为1的路径存活
                    else {

                        for (size_t beta = 0; beta < group_size; beta += 4) {
                            c_lambda[beta + left_right * group_size + pos_min2[l]] = 1 - tmp1;
                            c_lambda[beta + left_right * group_size + pos_min3[l]] = 1 - tmp2;
                        }
                        _pathMetric_LLR.at(l) = -probForks.at(2 * l + 1);

                    }

                }

            }


            for (uint32_t l = 0; l < _list_size; ++l) {

                // 如果路径激活
                if (_activePath.at(l) == 0)
                    continue;
                uint8_t* c_lambda = getArrayPointer_C(layer, l);
                uint8_t* c_0 = _arrayPointer_Info.at(l);

                uint8_t x1 = c_lambda[left_right * group_size];
                uint8_t x2 = c_lambda[left_right * group_size + 1];
                uint8_t x3 = c_lambda[left_right * group_size + 2];
                uint8_t x4 = c_lambda[left_right * group_size + 3];

                uint8_t u1 = x1 ^ x2 ^ x3;
                uint8_t u2 = x2 ^ x4;
                uint8_t u3 = x3 ^ x4;
                uint8_t u4 = x4;

                for (size_t beta = 0; beta < group_size - 3; beta++) {
                    c_0[begin + beta] = 0;
                }
                c_0[begin + group_size - 3] = u2;
                c_0[begin + group_size - 2] = u3;
                c_0[begin + group_size - 1] = u4;

            }

            recursivelyUpdateC(layer, parent);
        }
        else if (_nodes_type[cnt].type == NodeType::RPC) {

        }
        else if (_nodes_type[cnt].type == NodeType::NONE) {
            for (uint32_t phi = _nodes_type[cnt].begin; phi <= _nodes_type[cnt].end; phi++) {
                // 计算LLR
                if (_llr_based_computation)
                    recursivelyCalcLLR(_n, phi);
                else
                    recursivelyCalcP(_n, phi);

                // 译码+复制路径
                if (_frozen_bits.at(phi) == 1)
                    continuePaths_FrozenBit(phi);
                else
                    continuePaths_UnfrozenBit(phi);

                if ((phi % 2) == 1) {
                    recursivelyUpdateC(_n, phi);
                }
            }
        }
    }
    uint32_t l = findMostProbablePath((bool)_crc_size);
    uint8_t* c_0 = _arrayPointer_Info.at(l);
    std::vector<uint8_t> deocded_info_bits(_info_length);
    for (uint32_t beta = 0; beta < _info_length; ++beta) {
        deocded_info_bits.at(beta) = c_0[_channel_order_ascending.at(_block_length - _info_length + beta)];
    }
    _thread_pool.wait_for_all_tasks();

    if (!_memory) {
        for (uint32_t s = 0; s < _list_size; ++s) {
            delete[] _arrayPointer_Info.at(s);
            for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {

                if (_llr_based_computation)
                    delete[] _arrayPointer_LLR.at(lambda).at(s);
                else
                    delete[] _arrayPointer_P.at(lambda).at(s);
                delete[] _arrayPointer_C.at(lambda).at(s);
            }
        }
    }
    return deocded_info_bits;
}

void PolarCode::initializeDataStructures() {

    while (_inactivePathIndices.size()) {
        _inactivePathIndices.pop();
    };
    _activePath.resize(_list_size);

    if (!_memory) {

        if (_llr_based_computation) {
            _pathMetric_LLR.resize(_list_size);
            _arrayPointer_LLR.resize(_n + 1);
            for (int i = 0; i < _n + 1; ++i)
                _arrayPointer_LLR.at(i).resize(_list_size);
        }
        else {
            _arrayPointer_P.resize(_n + 1);
            for (int i = 0; i < _n + 1; ++i)
                _arrayPointer_P.at(i).resize(_list_size);
        }

        _arrayPointer_C.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i)
            _arrayPointer_C.at(i).resize(_list_size);

        _arrayPointer_Info.resize(_list_size);

        _pathIndexToArrayIndex.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i)
            _pathIndexToArrayIndex.at(i).resize(_list_size);

        _inactiveArrayIndices.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i) {
            while (_inactiveArrayIndices.at(i).size()) {
                _inactiveArrayIndices.at(i).pop();
            };
        }

        _arrayReferenceCount.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i)
            _arrayReferenceCount.at(i).resize(_list_size);

        for (uint32_t s = 0; s < _list_size; ++s) {
            _arrayPointer_Info.at(s) = new uint8_t[_block_length]();
            for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {
                if (_llr_based_computation) {
                    _arrayPointer_LLR.at(lambda).at(s) = new double[(1 << (_n - lambda))]();
                }
                else {
                    _arrayPointer_P.at(lambda).at(s) = new double[2 * (1 << (_n - lambda))]();
                }
                _arrayPointer_C.at(lambda).at(s) = new uint8_t[2 * (1 << (_n - lambda))]();
                _arrayReferenceCount.at(lambda).at(s) = 0;
                _inactiveArrayIndices.at(lambda).push(s);
            }
        }
    }
    else if (_start == false) {

        if (_llr_based_computation) {
            _pathMetric_LLR.resize(_list_size);
            _arrayPointer_LLR.resize(_n + 1);
            for (int i = 0; i < _n + 1; ++i)
                _arrayPointer_LLR.at(i).resize(_list_size);
        }
        else {
            _arrayPointer_P.resize(_n + 1);
            for (int i = 0; i < _n + 1; ++i)
                _arrayPointer_P.at(i).resize(_list_size);
        }
        _start = true;

        _arrayPointer_C.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i)
            _arrayPointer_C.at(i).resize(_list_size);

        _arrayPointer_Info.resize(_list_size);

        _pathIndexToArrayIndex.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i)
            _pathIndexToArrayIndex.at(i).resize(_list_size);

        _inactiveArrayIndices.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i) {
            while (_inactiveArrayIndices.at(i).size()) {
                _inactiveArrayIndices.at(i).pop();
            };
        }

        _arrayReferenceCount.resize(_n + 1);
        for (int i = 0; i < _n + 1; ++i)
            _arrayReferenceCount.at(i).resize(_list_size);

        for (uint32_t s = 0; s < _list_size; ++s) {
            _arrayPointer_Info.at(s) = new uint8_t[_block_length]();
            for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {
                if (_llr_based_computation) {
                    _arrayPointer_LLR.at(lambda).at(s) = new double[(1 << (_n - lambda))]();
                }
                else {
                    _arrayPointer_P.at(lambda).at(s) = new double[2 * (1 << (_n - lambda))]();
                }
                _arrayPointer_C.at(lambda).at(s) = new uint8_t[2 * (1 << (_n - lambda))]();
                _arrayReferenceCount.at(lambda).at(s) = 0;
                _inactiveArrayIndices.at(lambda).push(s);
            }
        }

    }
    else {

        for (uint32_t s = 0; s < _list_size; ++s) {

            for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {
                _arrayReferenceCount.at(lambda).at(s) = 0;
                _inactiveArrayIndices.at(lambda).push(s);
            }
        }
    }


    for (uint32_t l = 0; l < _list_size; ++l) {
        _activePath.at(l) = 0;
        _inactivePathIndices.push(l);
        if (_llr_based_computation) {
            _pathMetric_LLR.at(l) = 0;
        }
    }
}

uint32_t PolarCode::assignInitialPath() {

    uint32_t  l = _inactivePathIndices.top();
    _inactivePathIndices.pop();
    _activePath.at(l) = 1;
    // Associate arrays with path index
    for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {
        uint32_t  s = _inactiveArrayIndices.at(lambda).top();
        _inactiveArrayIndices.at(lambda).pop();
        _pathIndexToArrayIndex.at(lambda).at(l) = s;
        _arrayReferenceCount.at(lambda).at(s) = 1;
    }
    return l;
}

uint32_t PolarCode::clonePath(uint32_t l) {
    uint32_t l_p = _inactivePathIndices.top();
    _inactivePathIndices.pop();
    _activePath.at(l_p) = 1;

    if (_llr_based_computation)
        _pathMetric_LLR.at(l_p) = _pathMetric_LLR.at(l);

    for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {
        uint32_t s = _pathIndexToArrayIndex.at(lambda).at(l);
        _pathIndexToArrayIndex.at(lambda).at(l_p) = s;
        _arrayReferenceCount.at(lambda).at(s)++;
    }
    return l_p;
}

void PolarCode::killPath(uint32_t l) {

    /// 说明：
    /// 删除路径需要：1.表明这条路径被kill；2.度量值PM置0；3.路径映射的内存被映射的数量减1；4.如果内存被映射的数量减到0，则加入到未使用编号中去

    // 1.
    _activePath.at(l) = 0;
    _inactivePathIndices.push(l);

    // 2.
    if (_llr_based_computation)
        _pathMetric_LLR.at(l) = 0;

    for (uint32_t lambda = 0; lambda < _n + 1; ++lambda) {

        // 3.
        uint32_t s = _pathIndexToArrayIndex.at(lambda).at(l);
        _arrayReferenceCount.at(lambda).at(s)--;

        // 4.
        if (_arrayReferenceCount.at(lambda).at(s) == 0)
            _inactiveArrayIndices.at(lambda).push(s);

    }

}

double* PolarCode::getArrayPointer_P(uint32_t lambda, uint32_t  l) {
    uint32_t  s = _pathIndexToArrayIndex.at(lambda).at(l);
    uint32_t s_p;
    if (_arrayReferenceCount.at(lambda).at(s) == 1) {
        s_p = s;
    }
    else {
        s_p = _inactiveArrayIndices.at(lambda).top();
        _inactiveArrayIndices.at(lambda).pop();

        //copy
        std::copy(_arrayPointer_P.at(lambda).at(s), _arrayPointer_P.at(lambda).at(s) + (1 << (_n - lambda + 1)), _arrayPointer_P.at(lambda).at(s_p));
        std::copy(_arrayPointer_C.at(lambda).at(s), _arrayPointer_C.at(lambda).at(s) + (1 << (_n - lambda + 1)), _arrayPointer_C.at(lambda).at(s_p));

        _arrayReferenceCount.at(lambda).at(s)--;
        _arrayReferenceCount.at(lambda).at(s_p) = 1;
        _pathIndexToArrayIndex.at(lambda).at(l) = s_p;
    }
    return _arrayPointer_P.at(lambda).at(s_p);
}

/// 注意：只记录自己节点的LLR,同层之前的节点的数据已经不需要了
double* PolarCode::getArrayPointer_LLR(uint32_t lambda, uint32_t  l) {

    /// 说明：
    /// 获取第l条路径在lambda层所对应的地址对应的编号s
    /// 第l条路径的第lambda层可能是和其它路径的lambda层公用一个内存的(复制路径时，很多数据是一样的)
    /// 所以使用一个编号s代替那块内存
    uint32_t  s = _pathIndexToArrayIndex.at(lambda).at(l);

    // sp代替新的内存编号
    uint32_t s_p;

    /// 说明：
    /// _arrayReferenceCount用于说明，有多少路径共同使用了，存放第lambda层LLR值/码字存放的地址
    /// 如果只有一条路径使用这块内存，则直接返回
    if (_arrayReferenceCount.at(lambda).at(s) == 1) {

        s_p = s;

    }
    // 如果有多条路径
    else {

        // 从_inactiveArrayIndices中激活一个新的地址编号
        s_p = _inactiveArrayIndices.at(lambda).top();
        _inactiveArrayIndices.at(lambda).pop();

        // 将LLR值/码字复制入新的内存
        std::copy(_arrayPointer_C.at(lambda).at(s), _arrayPointer_C.at(lambda).at(s) + (1 << (_n - lambda + 1)),
            _arrayPointer_C.at(lambda).at(s_p));

        /// 说明：
        /// 如果LLR有内存，对应的码字也需要新内存
        std::copy(_arrayPointer_LLR.at(lambda).at(s), _arrayPointer_LLR.at(lambda).at(s) + (1 << (_n - lambda)),
            _arrayPointer_LLR.at(lambda).at(s_p));

        // s块内存公用数量减一
        _arrayReferenceCount.at(lambda).at(s)--;

        // sp块内存公用数量置一
        _arrayReferenceCount.at(lambda).at(s_p) = 1;

        // 映射
        _pathIndexToArrayIndex.at(lambda).at(l) = s_p;

    }

    return _arrayPointer_LLR.at(lambda).at(s_p);
}

/// 注意：C中存储了两个节点的码字，分别是（兄弟节点，自己）或者（自己，兄弟节点），在recursivelyUpdateC便于往上迭代
uint8_t* PolarCode::getArrayPointer_C(uint32_t lambda, uint32_t  l) {

    // 变量含义同getArrayPointer_LLR
    uint32_t  s = _pathIndexToArrayIndex.at(lambda).at(l);
    uint32_t s_p;


    /// 说明：
    /// 如果前面执行过getArrayPointer_LLR，则此时进入==1的情况
    /// 不会重复执行copy操作
    if (_arrayReferenceCount.at(lambda).at(s) == 1) {

        s_p = s;

    }
    else {

        s_p = _inactiveArrayIndices.at(lambda).top();
        _inactiveArrayIndices.at(lambda).pop();

        //copy
        if (_llr_based_computation)
            std::copy(_arrayPointer_LLR.at(lambda).at(s), _arrayPointer_LLR.at(lambda).at(s) + (1 << (_n - lambda)), _arrayPointer_LLR.at(lambda).at(s_p));
        else
            std::copy(_arrayPointer_P.at(lambda).at(s), _arrayPointer_P.at(lambda).at(s) + (1 << (_n - lambda + 1)), _arrayPointer_P.at(lambda).at(s_p));

        std::copy(_arrayPointer_C.at(lambda).at(s), _arrayPointer_C.at(lambda).at(s) + (1 << (_n - lambda + 1)), _arrayPointer_C.at(lambda).at(s_p));

        _arrayReferenceCount.at(lambda).at(s)--;
        _arrayReferenceCount.at(lambda).at(s_p) = 1;
        _pathIndexToArrayIndex.at(lambda).at(l) = s_p;


    }

    return _arrayPointer_C.at(lambda).at(s_p);
}

void PolarCode::recursivelyCalcP(uint32_t lambda, uint32_t phi) {
    if (lambda == 0)
        return;
    uint32_t psi = phi >> 1;
    if ((phi % 2) == 0)
        recursivelyCalcP(lambda - 1, psi);

    double sigma = 0.0f;
    for (uint32_t l = 0; l < _list_size; ++l) {
        if (_activePath.at(l) == 0)
            continue;
        double* p_lambda = getArrayPointer_P(lambda, l);
        double* p_lambda_1 = getArrayPointer_P(lambda - 1, l);

        uint8_t* c_lambda = getArrayPointer_C(lambda, l);
        for (uint32_t beta = 0; beta < (1 << (_n - lambda)); ++beta) {
            if ((phi % 2) == 0) {
                p_lambda[2 * beta] = 0.5f * (p_lambda_1[2 * (2 * beta)] * p_lambda_1[2 * (2 * beta + 1)]
                    + p_lambda_1[2 * (2 * beta) + 1] * p_lambda_1[2 * (2 * beta + 1) + 1]);
                p_lambda[2 * beta + 1] = 0.5f * (p_lambda_1[2 * (2 * beta) + 1] * p_lambda_1[2 * (2 * beta + 1)]
                    + p_lambda_1[2 * (2 * beta)] * p_lambda_1[2 * (2 * beta + 1) + 1]);
            }
            else {
                uint8_t  u_p = c_lambda[2 * beta];
                p_lambda[2 * beta] = 0.5f * p_lambda_1[2 * (2 * beta) + (u_p % 2)] * p_lambda_1[2 * (2 * beta + 1)];
                p_lambda[2 * beta + 1] = 0.5f * p_lambda_1[2 * (2 * beta) + ((u_p + 1) % 2)] * p_lambda_1[2 * (2 * beta + 1) + 1];
            }
            sigma = std::max(sigma, p_lambda[2 * beta]);
            sigma = std::max(sigma, p_lambda[2 * beta + 1]);


        }
    }

    for (uint32_t l = 0; l < _list_size; ++l) {
        if (sigma == 0) // Typically happens because of undeflow
            break;
        if (_activePath.at(l) == 0)
            continue;
        double* p_lambda = getArrayPointer_P(lambda, l);
        for (uint32_t beta = 0; beta < (1 << (_n - lambda)); ++beta) {
            p_lambda[2 * beta] = p_lambda[2 * beta] / sigma;
            p_lambda[2 * beta + 1] = p_lambda[2 * beta + 1] / sigma;
        }
    }
}

void PolarCode::recursivelyCalcLLR(uint32_t lambda, uint32_t phi) {
    // 到达0层，这一层的LLR就是接收到的LLR，不需要做额外计算
    if (lambda == 0)
        return;

    // 父节点编号
    uint32_t psi = phi >> 1;

    /// 说明：
    /// 如果是右节点，需求父节点计算LLR值，所以需要父节点计算完成后，获取到所需要的似然值。
    /// 显然第一次进入lambda，phi函数且为右节点时，phi-1，phi必然是第一次进入，也是唯一一次进入
    /// 如果是左节点，则右节点已经计算过了，不需要再计算了
    if ((phi % 2) == 0)
        recursivelyCalcLLR(lambda - 1, psi);


    // _list_size条路径依次计算
    for (uint32_t l = 0; l < _list_size; ++l) {

        // 如果路径激活
        if (_activePath.at(l) == 0)
            continue;

        // 获取第l条路径的第lambda层的LLR数组的指针
        double* llr_lambda = getArrayPointer_LLR(lambda, l);

        // 获取第l条路径的第lambda-1层的LLR数组的指针，其在recursivelyCalcLLR(lambda - 1, psi)中完成计算，用于f公式计算
        double* llr_lambda_1 = getArrayPointer_LLR(lambda - 1, l);

        // 获取第l条路径的第lambda层的码字数组的指针，用于g公式计算
        uint8_t* c_lambda = getArrayPointer_C(lambda, l);

        uint32_t group_size = (1 << (_n - lambda));

        for (uint32_t beta = 0; beta < group_size; ++beta) {

            // 如果为右节点
            if ((phi % 2) == 0) {

                if (40 > std::max(std::abs(llr_lambda_1[beta]), std::abs(llr_lambda_1[beta + group_size]))) {

                    llr_lambda[beta] = std::log((exp(llr_lambda_1[beta] + llr_lambda_1[beta + group_size]) + 1) /
                        (exp(llr_lambda_1[beta]) + exp(llr_lambda_1[beta + group_size])));

                }
                else {
                    /// 近似公式，防止数据溢出
                    llr_lambda[beta] = (double)(1 - 2 * (llr_lambda_1[beta] < 0)) *
                        (1 - 2 * (llr_lambda_1[beta + group_size] < 0)) *
                        std::min(std::abs(llr_lambda_1[beta]), std::abs(llr_lambda_1[beta + group_size]));

                }

            }
            // 否则为左节点
            else {

                uint8_t  u_p = c_lambda[beta];
                llr_lambda[beta] = (1 - 2 * u_p) * llr_lambda_1[beta] + llr_lambda_1[beta + group_size];

            }
        }
    }
}

void PolarCode::recursivelyUpdateC(uint32_t lambda, uint32_t phi) {

    // 父节点编号
    uint32_t psi = phi >> 1;

    for (uint32_t l = 0; l < _list_size; ++l) {

        if (_activePath.at(l) == 0)
            continue;

        // 当前节点的码字的内存
        uint8_t* c_lambda = getArrayPointer_C(lambda, l);

        // 父节点的码字的内存
        uint8_t* c_lambda_1 = getArrayPointer_C(lambda - 1, l);

        // 往上递归
        uint32_t group_size = (1 << (_n - lambda));
        for (uint32_t beta = 0; beta < group_size; ++beta) {

            c_lambda_1[beta + (psi % 2) * (group_size * 2)] = (uint8_t)((c_lambda[beta] + c_lambda[beta + group_size]) % 2);
            c_lambda_1[beta + group_size + (psi % 2) * (group_size * 2)] = c_lambda[beta + group_size];

        }
    }
    if ((psi % 2) == 1)
        recursivelyUpdateC((uint32_t)(lambda - 1), psi);

}

void PolarCode::continuePaths_FrozenBit(uint32_t phi) {

    for (uint32_t l = 0; l < _list_size; ++l) {

        if (_activePath.at(l) == 0)
            continue;

        uint8_t* c_m = getArrayPointer_C(_n, l);

        // frozen value assumed to be zero
        c_m[(phi % 2)] = 0;

        if (_llr_based_computation) {

            double* llr_p = getArrayPointer_LLR(_n, l);

            _pathMetric_LLR.at(l) += log(1 + exp(-llr_p[0]));

        }

        _arrayPointer_Info.at(l)[phi] = 0;

    }

}

void PolarCode::continuePaths_UnfrozenBit(uint32_t phi) {

    // l条路径，每次增到2l条，再排回l条
    std::vector<double>  probForks((unsigned long)(2 * _list_size));
    std::vector<double> probabilities;
    std::vector<uint8_t>  contForks((unsigned long)(2 * _list_size));

    uint32_t  i = 0;

    for (unsigned l = 0; l < _list_size; ++l) {

        if (_activePath.at(l) == 0) {

            probForks.at(2 * l) = NAN;
            probForks.at(2 * l + 1) = NAN;

        }
        else {

            if (_llr_based_computation) {

                // 获取结果的似然值
                double* llr_p = getArrayPointer_LLR(_n, l);
                // pm = pm + ln(1+exp(-(1-2u)*llr))

                // 判为0的
                probForks.at(2 * l) = -(_pathMetric_LLR.at(l) + log(1 + exp(-llr_p[0])));

                // 判为1
                probForks.at(2 * l + 1) = -(_pathMetric_LLR.at(l) + log(1 + exp(llr_p[0])));

            }
            else {

                double* p_m = getArrayPointer_P(_n, l);
                probForks.at(2 * l) = p_m[0];
                probForks.at(2 * l + 1) = p_m[1];

            }

            probabilities.push_back(probForks.at(2 * l));
            probabilities.push_back(probForks.at(2 * l + 1));

            i++;

        }
    }

    uint32_t  rho = _list_size;

    if ((2 * i) < _list_size)
        rho = (uint32_t)2 * i;

    for (uint32_t l = 0; l < 2 * _list_size; ++l)
        contForks.at(l) = 0;

    // 从大到小排序
    std::sort(probabilities.begin(), probabilities.end(), std::greater<double>());

    double threshold = probabilities.at((unsigned long)(rho - 1));
    uint32_t num_paths_continued = 0;

    // 选择存活的路径
    for (uint32_t l = 0; l < 2 * _list_size; ++l) {

        if (probForks.at(l) > threshold) {

            contForks.at(l) = 1;
            num_paths_continued++;

        }
        if (num_paths_continued == rho) {
            break;
        }

    }

    // 小概率事件：存在度量值相等的情况。极端----所有度量值都一样，此时找不到大于门限的路径，则等于门限的路径也加入。
    if (num_paths_continued < rho) {

        for (uint32_t l = 0; l < 2 * _list_size; ++l) {

            if (probForks.at(l) == threshold) {

                contForks.at(l) = 1;
                num_paths_continued++;

            }

            if (num_paths_continued == rho)
                break;

        }

    }

    for (unsigned l = 0; l < _list_size; ++l) {

        if (_activePath.at(l) == 0)
            continue;

        // 需要舍弃的路径
        if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
            killPath(l);

    }


    for (unsigned l = 0; l < _list_size; ++l) {

        if (contForks.at(2 * l) == 0 && contForks.at(2 * l + 1) == 0)
            continue;

        uint8_t* c_m = getArrayPointer_C(_n, l);

        // 都存活了
        if (contForks.at(2 * l) == 1 && contForks.at(2 * l + 1) == 1) {

            // 路径l为判为0的路径
            c_m[(phi % 2)] = 0;

            // 复制路径l到l_p
            uint32_t l_p = clonePath(l);
            c_m = getArrayPointer_C(_n, l_p);
            c_m[(phi % 2)] = 1;

            // l路径复制到l_p
            std::copy(_arrayPointer_Info.at(l), _arrayPointer_Info.at(l) + phi, _arrayPointer_Info.at(l_p));

            /// 说明：
            /// _arrayPointer_Info放最终结果，c_m放的是当前phi为的译码结果，是暂时变量
            _arrayPointer_Info.at(l)[phi] = 0;
            _arrayPointer_Info.at(l_p)[phi] = 1;

            if (_llr_based_computation) {
                double* llr_p = getArrayPointer_LLR(_n, l);
                _pathMetric_LLR.at(l) += log(1 + exp(-llr_p[0]));
                llr_p = getArrayPointer_LLR(_n, l_p);
                _pathMetric_LLR.at(l_p) += log(1 + exp(llr_p[0]));
            }

        }
        else {

            // 译为0的路径存活
            if (contForks.at(2 * l) == 1) {

                c_m[(phi % 2)] = 0;
                _arrayPointer_Info.at(l)[phi] = 0;

                if (_llr_based_computation) {

                    double* llr_p = getArrayPointer_LLR(_n, l);
                    _pathMetric_LLR.at(l) += log(1 + exp(-llr_p[0]));

                }

            }
            // 译为1的路径存活
            else {

                c_m[(phi % 2)] = 1;
                _arrayPointer_Info.at(l)[phi] = 1;

                if (_llr_based_computation) {

                    double* llr_p = getArrayPointer_LLR(_n, l);
                    _pathMetric_LLR.at(l) += log(1 + exp(llr_p[0]));

                }

            }

        }

    }

}

uint32_t PolarCode::findMostProbablePath(bool check_crc) {

    uint32_t  l_p = 0;
    double p_p1 = 0;
    double p_llr = std::numeric_limits<double>::max();
    _path_with_crc_pass = false;
    uint32_t l;
    for (l = 0; l < _list_size; ++l) {

        if (_activePath.at(l) == 0)
            continue;

        if (_system_polar)
            info_return(_arrayPointer_Info.at(l));

        if ((check_crc) && (!crc_check(_arrayPointer_Info.at(l))))
            continue;

        _path_with_crc_pass = true;


        if (_pathMetric_LLR.at(l) < p_llr) {

            p_llr = _pathMetric_LLR.at(l);
            l_p = l;

        }

    }

    if (_path_with_crc_pass)
        return l_p;
    else
        return findMostProbablePath(false);

}

void PolarCode::create_bit_rev_order() {
    for (uint32_t i = 0; i < _block_length; ++i) {
        uint32_t to_be_reversed = i;
        _bit_rev_order.at(i) = (uint32_t)((to_be_reversed & 1) << (_n - 1));
        for (uint8_t j = (uint8_t)(_n - 1); j; --j) {
            to_be_reversed >>= 1;
            _bit_rev_order.at(i) += (to_be_reversed & 1) << (j - 1);
        }
    }
}

std::vector<std::vector<double>> PolarCode::get_bler_quick(std::vector<double> ebno_vec,
    std::vector<uint32_t> list_size_vec) {

    //初始化---------------------------------------------
    int max_err = 10000;
    int max_runs = 10000;

    std::vector<std::vector<double>> bler;
    std::vector<std::vector<double>> num_err;
    std::vector<std::vector<double>> num_run;

    bler.resize(list_size_vec.size());
    num_err.resize((list_size_vec.size()));
    num_run.resize(list_size_vec.size());

    for (uint32_t l = 0; l < list_size_vec.size(); ++l) {
        bler.at(l).resize(ebno_vec.size(), 0);
        num_err.at(l).resize(ebno_vec.size(), 0);
        num_run.at(l).resize(ebno_vec.size(), 0);
    }

    std::vector<uint8_t> coded_bits;
    std::vector<double> bpsk(_block_length);
    std::vector<double> received_signal(_block_length, 0);
    std::vector<uint8_t> info_bits(_info_length, 0);

    double N_0 = 1.0;
    //    double sigma_sqrt_pi = std::sqrt(N_0 * 3.1415f);

    std::vector<double> noise(_block_length, 0);

    std::normal_distribution<double> gauss_dist(0.0f, N_0);
    std::default_random_engine generator;
    generator.seed(static_cast<unsigned long>(std::time(0)));

    //    std::vector<double> p0(_block_length), p1(_block_length);
    std::vector<double> llr(_block_length);

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    // 初始化结束-------------------------------------------------------------------------------------


    // 仿真开始---------------------------------------------------------------------------------------
    for (int run = 0; run < max_runs; ++run) {

        // log
        if ((run % (max_runs / 100)) == 0) {
            std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            std::cout << "Running iteration " << run << "; time elapsed = " << duration / 1000 / 1000 << " seconds"
                "; percent complete = " << (100 * run) / max_runs << "." << std::endl;
        }

        // 每一百次换一次信息
        if ((run % 100) == 0) {
            for (uint32_t i = 0; i < _info_length; ++i) {
                /*info_bits.at(i) = (uint8_t)(rand() % 2);*/
                info_bits.at(i) = (uint8_t)(1);
            }
        }

        // 噪声
        for (uint32_t i = 0; i < _block_length; ++i) {
            noise.at(i) = (double)gauss_dist(generator);
        }

        // 码字
        if (_system_polar)
            coded_bits = sys_encode(info_bits);
        else
            coded_bits = encode(info_bits);

        // bpsk
        for (uint32_t i = 0; i < _block_length; ++i) {
            bpsk.at(i) = 2.0f * ((double)coded_bits.at(i)) - 1.0f;
        }

        // 不同路径数量
        for (uint32_t l_index = 0; l_index < list_size_vec.size(); ++l_index) {

            std::vector<bool> prev_decoded(0);
            prev_decoded.resize(ebno_vec.size(), false);

            // 不同信噪比
            for (unsigned i_ebno = 0; i_ebno < ebno_vec.size(); ++i_ebno) {

                // max_err限制
                if (num_err.at(l_index).at(i_ebno) > max_err)
                    continue;

                // block总数
                num_run.at(l_index).at(i_ebno)++;

                // trick
                bool run_sim = true;
                for (unsigned i_ebno2 = 0; i_ebno2 < i_ebno; ++i_ebno2) {
                    if (prev_decoded.at(i_ebno2)) {
                        //  This is a hack to speed up simulations -- it assumes that this run will be decoded
                        // correctly since it was decoded correctly for a lower EbNo
                        run_sim = false;
                    }
                }
                if (!run_sim) {
                    continue;
                }

                // 信号
                double snr_sqrt_linear = std::pow(10.0f, ebno_vec.at(i_ebno) / 20)
                    * std::sqrt(((double)_info_length) / ((double)(_block_length)));
                for (uint32_t i = 0; i < _block_length; ++i) {
                    received_signal.at(i) = snr_sqrt_linear * bpsk.at(i) + std::sqrt(N_0 / 2) * noise.at(i);
                }

                // 似然值
                for (uint32_t i = 0; i < _block_length; ++i) {
                    //                    p0.at(i) = exp(-(received_signal.at(i) + snr_sqrt_linear )*(received_signal.at(i) + snr_sqrt_linear )/N_0)/sigma_sqrt_pi;
                    //                    p1.at(i) = exp(-(received_signal.at(i) - snr_sqrt_linear )*(received_signal.at(i) - snr_sqrt_linear )/N_0)/sigma_sqrt_pi;
                    llr.at(i) = -4 * received_signal.at(i) * snr_sqrt_linear / N_0;
                }

                // 译码
                // std::vector<uint8_t> decoded_info_bits = polar_code.decode_scl_p1(p1, p0, list_size);
                std::vector<uint8_t> decoded_info_bits = decode_scl_llr(llr, list_size_vec.at(l_index));

                bool err = false;
                for (uint32_t i = 0; i < _info_length; ++i) {
                    if (info_bits.at(i) != decoded_info_bits.at(i)) {
                        err = true;
                        break;
                    }
                }

                if (err)
                    num_err.at(l_index).at(i_ebno)++;
                else
                    prev_decoded.at(i_ebno) = true;

            } // EbNo Loop End

        } // List_size loop end

    } // run loop end

    for (unsigned l_index = 0; l_index < list_size_vec.size(); ++l_index) {
        for (unsigned i_ebno = 0; i_ebno < ebno_vec.size(); ++i_ebno) {
            bler.at(l_index).at(i_ebno) = num_err.at(l_index).at(i_ebno) / num_run.at(l_index).at(i_ebno);
        }
    }

    return bler;

}

