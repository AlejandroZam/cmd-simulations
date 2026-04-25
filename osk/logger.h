#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <stdexcept>

// Lightweight per-model output logger.
// Usage:
//   logger.open("output/smd1", {"t","pos","vel"}, LogFormat::CSV);
//   logger.write({State::t, pos, vel});
//   logger.close();

enum class LogFormat { CSV, BIN };

class Logger {
public:
    Logger() = default;
    ~Logger() { close(); }

    void open(const std::string& basePath, const std::vector<std::string>& cols, LogFormat fmt) {
        fmt_  = fmt;
        cols_ = cols;
        if (fmt_ == LogFormat::CSV) {
            std::string path = basePath + ".csv";
            f_ = std::fopen(path.c_str(), "w");
            if (!f_) throw std::runtime_error("Logger: cannot open " + path);
            for (std::size_t i = 0; i < cols_.size(); ++i) {
                std::fprintf(f_, "%s%s", cols_[i].c_str(), i + 1 < cols_.size() ? "," : "\n");
            }
        } else {
            std::string path = basePath + ".bin";
            bf_.open(path, std::ios::binary | std::ios::out);
            if (!bf_) throw std::runtime_error("Logger: cannot open " + path);
            uint32_t n = static_cast<uint32_t>(cols_.size());
            bf_.write(reinterpret_cast<const char*>(&n), sizeof(n));
            for (auto& c : cols_) {
                uint32_t len = static_cast<uint32_t>(c.size());
                bf_.write(reinterpret_cast<const char*>(&len), sizeof(len));
                bf_.write(c.data(), len);
            }
        }
        open_ = true;
    }

    void write(const std::vector<double>& vals) {
        if (!open_) return;
        if (fmt_ == LogFormat::CSV) {
            for (std::size_t i = 0; i < vals.size(); ++i)
                std::fprintf(f_, "%.9g%s", vals[i], i + 1 < vals.size() ? "," : "\n");
        } else {
            bf_.write(reinterpret_cast<const char*>(vals.data()),
                      static_cast<std::streamsize>(vals.size() * sizeof(double)));
        }
    }

    void close() {
        if (!open_) return;
        if (fmt_ == LogFormat::CSV && f_) { std::fclose(f_); f_ = nullptr; }
        if (fmt_ == LogFormat::BIN  && bf_.is_open()) bf_.close();
        open_ = false;
    }

    bool isOpen() const { return open_; }

private:
    LogFormat              fmt_  = LogFormat::CSV;
    std::vector<std::string> cols_;
    bool                   open_ = false;
    FILE*                  f_    = nullptr;
    std::ofstream          bf_;
};
