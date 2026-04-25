#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <stdexcept>

enum class LogFormat { CSV, BIN };

// Signal-registration logger. Models register every loggable variable once
// (typically in their constructor), then open() filters to the desired subset.
//
// Usage:
//   // In constructor:
//   logger_.addSignal("t",   &State::t);
//   logger_.addSignal("pos", &pos);
//   logger_.addSignal("vel", &vel);
//
//   // In initialize() — selected is loaded from YAML output.signals (empty = all):
//   logger_.open("output/smd1", LogFormat::CSV, {"t", "pos"});
//
//   // In report():
//   if (logger_.isOpen()) logger_.write();
class Logger {
public:
    Logger()  = default;
    ~Logger() { close(); }

    // Register a loggable signal. Call before open().
    void addSignal(const std::string& label, const double* ptr) {
        all_.push_back({label, ptr});
    }

    // Open output file. `selected` is an ordered list of signal labels to log;
    // if empty, all registered signals are logged.
    void open(const std::string& basePath,
              LogFormat fmt,
              const std::vector<std::string>& selected = {})
    {
        close();
        fmt_ = fmt;

        // Build active signal set
        active_.clear();
        if (selected.empty()) {
            active_ = all_;
        } else {
            for (const auto& name : selected) {
                for (const auto& sig : all_) {
                    if (sig.label == name) { active_.push_back(sig); break; }
                }
            }
        }
        if (active_.empty()) return;

        if (fmt_ == LogFormat::CSV) {
            std::string path = basePath + ".csv";
            f_ = std::fopen(path.c_str(), "w");
            if (!f_) throw std::runtime_error("Logger: cannot open " + path);
            for (std::size_t i = 0; i < active_.size(); ++i)
                std::fprintf(f_, "%s%s", active_[i].label.c_str(),
                             i + 1 < active_.size() ? "," : "\n");
        } else {
            std::string path = basePath + ".bin";
            bf_.open(path, std::ios::binary | std::ios::out);
            if (!bf_) throw std::runtime_error("Logger: cannot open " + path);
            // Header: n_cols (uint32), then for each col: len (uint32) + chars
            auto n = static_cast<uint32_t>(active_.size());
            bf_.write(reinterpret_cast<const char*>(&n), sizeof(n));
            for (const auto& sig : active_) {
                auto len = static_cast<uint32_t>(sig.label.size());
                bf_.write(reinterpret_cast<const char*>(&len), sizeof(len));
                bf_.write(sig.label.data(), len);
            }
        }
        open_ = true;
    }

    // Write the current values of all active signals as one row.
    void write() {
        if (!open_) return;
        if (fmt_ == LogFormat::CSV) {
            for (std::size_t i = 0; i < active_.size(); ++i)
                std::fprintf(f_, "%.9g%s", *active_[i].ptr,
                             i + 1 < active_.size() ? "," : "\n");
        } else {
            for (const auto& sig : active_) {
                double v = *sig.ptr;
                bf_.write(reinterpret_cast<const char*>(&v), sizeof(v));
            }
        }
    }

    void close() {
        if (!open_) return;
        if (f_)            { std::fclose(f_); f_ = nullptr; }
        if (bf_.is_open()) { bf_.close(); }
        open_ = false;
    }

    bool isOpen() const { return open_; }

private:
    struct Signal { std::string label; const double* ptr; };

    LogFormat            fmt_   = LogFormat::CSV;
    bool                 open_  = false;
    FILE*                f_     = nullptr;
    std::ofstream        bf_;
    std::vector<Signal>  all_;     // every registered signal
    std::vector<Signal>  active_;  // filtered subset written per row
};
