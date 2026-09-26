#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <functional>
#include <cstdlib>
#include <atomic>
#include <mutex>

static int N = 5;
static std::vector<uint64_t> LINES;

static std::atomic<uint64_t> g_scanned{0};
static std::atomic<uint64_t> g_valid{0};
static std::atomic<bool>     g_done{false};

// 输出文件 + 锁
static std::ofstream g_fout;
static std::ofstream g_fbin;
static std::mutex    g_out_mutex;

void buildLines() {
    LINES.clear();
    for (int r = 0; r < N; ++r) {
        uint64_t m = 0;
        for (int c = 0; c < N; ++c) m |= (1ull << (r * N + c));
        LINES.push_back(m);
    }
    for (int c = 0; c < N; ++c) {
        uint64_t m = 0;
        for (int r = 0; r < N; ++r) m |= (1ull << (r * N + c));
        LINES.push_back(m);
    }
    for (int d = 0; d < N; ++d) {
        uint64_t m = 0;
        for (int r = 0; r < N; ++r) {
            int c = (r + d) % N;
            m |= (1ull << (r * N + c));
        }
        LINES.push_back(m);
    }
    for (int d = 0; d < N; ++d) {
        uint64_t m = 0;
        for (int r = 0; r < N; ++r) {
            int c = ((d - r) % N + N) % N;
            m |= (1ull << (r * N + c));
        }
        LINES.push_back(m);
    }
}

inline bool isValid(uint64_t mask) {
    for (uint64_t line : LINES) {
        uint64_t v = mask & line;
        if (v == 0 || v == line) return false;
    }
    return true;
}

// 每个线程本地缓冲，攒够一批后加锁一次性写入
void worker(uint64_t start, uint64_t end) {
    const uint64_t BATCH_SCAN = 4096;
    const size_t   BATCH_OUT  = 4096;   // 攒 4096 个结果再写

    std::string txt_buf;   // 本地文本缓冲
    std::vector<uint64_t> bin_buf;
    txt_buf.reserve(BATCH_OUT * (N * N + 1));
    bin_buf.reserve(BATCH_OUT);

    uint64_t local_scan = 0;
    uint64_t local_valid = 0;

    for (uint64_t mask = start; mask < end; ++mask) {
        if (isValid(mask)) {
            // 文本
            for (int bit = 0; bit < N * N; ++bit)
                txt_buf.push_back((mask & (1ull << bit)) ? '1' : '0');
            txt_buf.push_back('\n');
            // 二进制
            bin_buf.push_back(mask);
            ++local_valid;

            if (bin_buf.size() >= BATCH_OUT) {
                std::lock_guard<std::mutex> lk(g_out_mutex);
                g_fout.write(txt_buf.data(), txt_buf.size());
                g_fbin.write(reinterpret_cast<const char*>(bin_buf.data()),
                             bin_buf.size() * sizeof(uint64_t));
                txt_buf.clear();
                bin_buf.clear();
            }
        }

        if (++local_scan >= BATCH_SCAN) {
            g_scanned.fetch_add(local_scan, std::memory_order_relaxed);
            local_scan = 0;
        }
    }

    // 刷剩余
    if (!bin_buf.empty()) {
        std::lock_guard<std::mutex> lk(g_out_mutex);
        g_fout.write(txt_buf.data(), txt_buf.size());
        g_fbin.write(reinterpret_cast<const char*>(bin_buf.data()),
                     bin_buf.size() * sizeof(uint64_t));
    }
    if (local_scan > 0)
        g_scanned.fetch_add(local_scan, std::memory_order_relaxed);
    g_valid.fetch_add(local_valid, std::memory_order_relaxed);
}

std::string maskToGridString(uint64_t mask) {
    std::string res;
    res.reserve(N * (N + 1));
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            int bit = r * N + c;
            res.push_back((mask & (1ull << bit)) ? '1' : '0');
        }
        res.push_back('\n');
    }
    return res;
}

void progressLoop(uint64_t total, std::chrono::steady_clock::time_point t0) {
    using namespace std::chrono;
    const int BAR_WIDTH = 40;
    auto last_time = steady_clock::now();
    uint64_t last_scanned = 0;

    std::cout << "\n[进度] 开始扫描...\n";
    while (!g_done.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(milliseconds(500));

        auto now = steady_clock::now();
        uint64_t scanned = g_scanned.load(std::memory_order_relaxed);

        double elapsed = duration<double>(now - t0).count();
        double dt      = duration<double>(now - last_time).count();
        uint64_t dscan = scanned - last_scanned;

        double speed = (dt > 0.0) ? (double)dscan / dt : 0.0;
        double ratio = (total > 0) ? (double)scanned / (double)total : 0.0;
        if (ratio > 1.0) ratio = 1.0;

        int filled = (int)(ratio * BAR_WIDTH);
        std::string bar(BAR_WIDTH, ' ');
        for (int i = 0; i < filled; ++i) bar[i] = '#';

        double eta = (speed > 0.0) ? (double)(total - scanned) / speed : 0.0;

        std::cout << "\r[进度] [" << bar << "] "
                  << std::fixed << std::setprecision(2) << (ratio * 100.0) << "% "
                  << "(" << scanned << "/" << total << ") "
                  << "速度: " << std::setprecision(2) << (speed / 1e6) << " M/s "
                  << "已用: " << std::setprecision(1) << elapsed << "s "
                  << "剩余: " << std::setprecision(1) << eta << "s   "
                  << std::flush;

        last_time = now;
        last_scanned = scanned;
    }

    uint64_t scanned = g_scanned.load(std::memory_order_relaxed);
    std::string bar(BAR_WIDTH, '#');
    double elapsed = duration<double>(steady_clock::now() - t0).count();
    std::cout << "\r[进度] [" << bar << "] "
              << "100.00% (" << scanned << "/" << total << ") "
              << "已用: " << std::fixed << std::setprecision(1) << elapsed << "s          "
              << std::endl;
}

int main(int argc, char** argv) {
    if (argc >= 2) N = std::atoi(argv[1]);
    if (N < 2 || N > 8) {
        std::cerr << "[错误] 当前实现支持 N 在 [2, 8]\n";
        return 1;
    }

    const int bits = N * N;
    if (bits >= 64) {
        std::cerr << "[错误] N*N 太大，无法用 64 位掩码枚举\n";
        return 1;
    }
    const uint64_t TOTAL_STATES = (1ull << bits);

    buildLines();

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    std::cout << "==========================================" << std::endl;
    std::cout << "  " << N << "x" << N << " 周期排列无" << N << "连同色矩阵验证程序 (C++)" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "[配置] 方阵大小      : " << N << " x " << N << std::endl;
    std::cout << "[配置] 直线总数      : " << LINES.size() << std::endl;
    std::cout << "[配置] 搜索状态总数  : " << TOTAL_STATES << " (2^" << bits << ")" << std::endl;
    std::cout << "[配置] 并行计算线程数: " << num_threads << std::endl;

    // 打开输出文件（文本 + 二进制）
    std::string filename_txt = "valid_matrices.txt";
    std::string filename_bin = "valid_matrices.bin";
    g_fout.open(filename_txt, std::ios::out | std::ios::binary);
    g_fbin.open(filename_bin, std::ios::out | std::ios::binary);
    if (!g_fout.is_open() || !g_fbin.is_open()) {
        std::cerr << "[错误] 无法创建输出文件\n";
        return 1;
    }

    g_fout << "# " << N << "x" << N << " 周期排列无" << N << "连同色矩阵统计结果\n";
    g_fout << "# 格式说明: 每行为一个矩阵的 " << bits << " 位二进制展开（按行优先存储）\n\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    auto progress_start = std::chrono::steady_clock::now();

    std::thread progress_thread(progressLoop, TOTAL_STATES, progress_start);

    std::vector<std::thread> threads;
    uint64_t chunk_size = TOTAL_STATES / num_threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        uint64_t start = (uint64_t)i * chunk_size;
        uint64_t end = (i == num_threads - 1) ? TOTAL_STATES : (uint64_t)(i + 1) * chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto& t : threads) t.join();
    g_done.store(true, std::memory_order_relaxed);
    progress_thread.join();

    auto calc_end_time = std::chrono::high_resolution_clock::now();

    uint64_t total_valid = g_valid.load(std::memory_order_relaxed);

    std::chrono::duration<double> calc_duration = calc_end_time - start_time;

    // 收尾文件
    g_fout.close();
    g_fbin.close();

    std::cout << "\n[计算结果]" << std::endl;
    std::cout << " -> 满足条件的矩阵总数: " << total_valid << " 种" << std::endl;
    std::cout << " -> 占总矩阵比例      : "
              << std::fixed << std::setprecision(4)
              << (double)total_valid / (double)TOTAL_STATES * 100.0 << " %" << std::endl;
    std::cout << " -> 计算耗时          : " << calc_duration.count() << " 秒" << std::endl;
    std::cout << " -> 文本文件: " << filename_txt << " (" 
              << (total_valid * (bits + 1)) / (1024 * 1024) << " MB)" << std::endl;
    std::cout << " -> 二进制文件: " << filename_bin << " ("
              << (total_valid * 8) / (1024 * 1024) << " MB)" << std::endl;

    // 示例：从二进制文件里读回前 3 个合法矩阵展示
    std::cout << "\n[示例展示] 前3个满足条件的矩阵图案:" << std::endl;
    std::ifstream fin(filename_bin, std::ios::in | std::ios::binary);
    int sample_count = 0;
    uint64_t mask;
    while (sample_count < 3 && fin.read(reinterpret_cast<char*>(&mask), sizeof(mask))) {
        std::cout << "---- 示例 " << ++sample_count << " (掩码: 0x"
                  << std::hex << mask << std::dec << ") ----\n";
        std::cout << maskToGridString(mask);
    }

    std::cout << "\n程序运行完成！" << std::endl;
    return 0;
}
