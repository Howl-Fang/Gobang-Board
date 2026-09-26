#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <functional>

// 25格平面的20条直线掩码（5行、5列、5主对角线、5副对角线）
const uint32_t LINES[20] = {
    // 5 行 (Rows)
    (1u<<0) | (1u<<1) | (1u<<2) | (1u<<3) | (1u<<4),
    (1u<<5) | (1u<<6) | (1u<<7) | (1u<<8) | (1u<<9),
    (1u<<10)| (1u<<11)| (1u<<12)| (1u<<13)| (1u<<14),
    (1u<<15)| (1u<<16)| (1u<<17)| (1u<<18)| (1u<<19),
    (1u<<20)| (1u<<21)| (1u<<22)| (1u<<23)| (1u<<24),

    // 5 列 (Columns)
    (1u<<0) | (1u<<5) | (1u<<10)| (1u<<15)| (1u<<20),
    (1u<<1) | (1u<<6) | (1u<<11)| (1u<<16)| (1u<<21),
    (1u<<2) | (1u<<7) | (1u<<12)| (1u<<17)| (1u<<22),
    (1u<<3) | (1u<<8) | (1u<<13)| (1u<<18)| (1u<<23),
    (1u<<4) | (1u<<9) | (1u<<14)| (1u<<19)| (1u<<24),

    // 5 主对角线 (Main Diagonals, slope +1: (r, (r+d)%5))
    (1u<<0) | (1u<<6) | (1u<<12)| (1u<<18)| (1u<<24), // d=0
    (1u<<1) | (1u<<7) | (1u<<13)| (1u<<19)| (1u<<20), // d=1
    (1u<<2) | (1u<<8) | (1u<<14)| (1u<<15)| (1u<<21), // d=2
    (1u<<3) | (1u<<9) | (1u<<10)| (1u<<16)| (1u<<22), // d=3
    (1u<<4) | (1u<<5) | (1u<<11)| (1u<<17)| (1u<<23), // d=4

    // 5 副对角线 (Anti Diagonals, slope -1: (r, (d-r)%5))
    (1u<<0) | (1u<<9) | (1u<<13)| (1u<<17)| (1u<<21), // d=0
    (1u<<1) | (1u<<5) | (1u<<14)| (1u<<18)| (1u<<22), // d=1
    (1u<<2) | (1u<<6) | (1u<<10)| (1u<<19)| (1u<<23), // d=2
    (1u<<3) | (1u<<7) | (1u<<11)| (1u<<15)| (1u<<24), // d=3
    (1u<<4) | (1u<<8) | (1u<<12)| (1u<<16)| (1u<<20)  // d=4
};

// 检查某个矩阵位掩码 mask 是否满足条件
inline bool isValid(uint32_t mask) {
    for (int i = 0; i < 20; ++i) {
        uint32_t v = mask & LINES[i];
        if (v == 0 || v == LINES[i]) {
            return false; // 存在全0或全1的5连线
        }
    }
    return true;
}

// 线程工作函数
void worker(uint32_t start, uint32_t end, std::vector<uint32_t>& local_results) {
    local_results.reserve((end - start) / 4); // 预分配空间提升效率
    for (uint32_t mask = start; mask < end; ++mask) {
        if (isValid(mask)) {
            local_results.push_back(mask);
        }
    }
}

// 辅助函数：将25位整数转为 5x5 矩阵文本字符串
std::string maskToGridString(uint32_t mask) {
    std::string res;
    res.reserve(30);
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 5; ++c) {
            int bit = (r * 5) + c;
            res.push_back((mask & (1u << bit)) ? '1' : '0');
        }
        res.push_back('\n');
    }
    return res;
}

int main() {
    const uint32_t TOTAL_STATES = 1u << 25; // 33,554,432
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    std::cout << "==========================================" << std::endl;
    std::cout << "  5x5 周期排列无5连同色矩阵验证程序 (C++)  " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "[配置] 搜索状态总数 : " << TOTAL_STATES << " (2^25)" << std::endl;
    std::cout << "[配置] 并行计算线程数: " << num_threads << std::endl;

    std::vector<std::thread> threads;
    std::vector<std::vector<uint32_t>> thread_results(num_threads);

    uint32_t chunk_size = TOTAL_STATES / num_threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    // 创建多线程计算
    for (unsigned int i = 0; i < num_threads; ++i) {
        uint32_t start = i * chunk_size;
        uint32_t end = (i == num_threads - 1) ? TOTAL_STATES : (i + 1) * chunk_size;
        threads.emplace_back(worker, start, end, std::ref(thread_results[i]));
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    auto calc_end_time = std::chrono::high_resolution_clock::now();

    // 汇总计算结果
    uint64_t total_valid = 0;
    for (const auto& vec : thread_results) {
        total_valid += vec.size();
    }

    std::chrono::duration<double> calc_duration = calc_end_time - start_time;
    std::cout << "\n[计算结果]" << std::endl;
    std::cout << " -> 满足条件的矩阵总数: " << total_valid << " 种" << std::endl;
    std::cout << " -> 占总矩阵比例      : " 
              << std::fixed << std::setprecision(4) 
              << (double)total_valid / TOTAL_STATES * 100.0 << " %" << std::endl;
    std::cout << " -> 计算耗时          : " 
              << calc_duration.count() << " 秒" << std::endl;

    // 写入文件
    std::string filename_txt = "valid_matrices.txt";
    std::string filename_bin = "valid_matrices.bin";
    std::cout << "\n[文件输出] 正在将矩阵写入文件: " << filename_txt << " ..." << std::endl;

    auto io_start_time = std::chrono::high_resolution_clock::now();

    // 1. 写入文本文件 (包含统计摘要 + 示例可视化 + 25位二进制编码)
    std::ofstream fout(filename_txt, std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        std::cerr << "[错误] 无法创建文件 " << filename_txt << std::endl;
        return 1;
    }

    fout << "# 5x5 周期排列无5连同色矩阵统计结果\n";
    fout << "# 满足条件的矩阵总数: " << total_valid << "\n";
    fout << "# 格式说明: 每行为一个矩阵的25位二进制展开（按行优先存储）\n\n";

    // 使用缓冲提高大文件写入效率
    std::string buffer;
    buffer.reserve(1 << 20); // 1MB buffer

    for (const auto& vec : thread_results) {
        for (uint32_t mask : vec) {
            // 写入25位的二进制串
            for (int bit = 0; bit < 25; ++bit) {
                buffer.push_back((mask & (1u << bit)) ? '1' : '0');
            }
            buffer.push_back('\n');

            if (buffer.size() >= (1 << 20)) {
                fout.write(buffer.data(), buffer.size());
                buffer.clear();
            }
        }
    }
    if (!buffer.empty()) {
        fout.write(buffer.data(), buffer.size());
        buffer.clear();
    }
    fout.close();

    // 2. 写入紧凑二进制文件 (.bin，便于后续快速加载)
    std::ofstream fbin(filename_bin, std::ios::out | std::ios::binary);
    if (fbin.is_open()) {
        for (const auto& vec : thread_results) {
            fbin.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(uint32_t));
        }
        fbin.close();
    }

    auto io_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> io_duration = io_end_time - io_start_time;

    std::cout << " -> 文件写入完毕，耗时: " << io_duration.count() << " 秒" << std::endl;
    std::cout << " -> 文本文件大小 : ~" << (total_valid * 26) / (1024 * 1024) << " MB" << std::endl;
    std::cout << " -> 二进制文件大小: ~" << (total_valid * 4) / (1024 * 1024) << " MB (" << filename_bin << ")" << std::endl;

    // 输出前 3 个符合条件矩阵的可视化示例
    std::cout << "\n[示例展示] 前3个满足条件的矩阵图案:" << std::endl;
    int sample_count = 0;
    for (const auto& vec : thread_results) {
        for (uint32_t mask : vec) {
            std::cout << "---- 示例 " << ++sample_count << " (掩码: 0x" 
                      << std::hex << mask << std::dec << ") ----\n";
            std::cout << maskToGridString(mask);
            if (sample_count >= 3) break;
        }
        if (sample_count >= 3) break;
    }

    std::cout << "\n程序运行完成！" << std::endl;
    return 0;
}
