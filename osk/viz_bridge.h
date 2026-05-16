#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>

// Header-only singleton UDP publisher for real-time visualization.
//
// Mirrors the SimDDS singleton pattern. Models call VizBridge::get().send()
// in their report() method; main.cpp calls init() once before the MC loop
// and shutdown() at the end.
//
// If init() was never called (sockfd_ == -1), all send calls are no-ops,
// so the model code is safe to call unconditionally.
//
// Wire format: 72-byte little-endian datagram (VizPacket POD).
//   struct VizPacket { uint8 msg_type; uint8 entity_id; uint16 run_id;
//                      uint32 padding; double t,px,py,pz,vx,vy,vz,range; }
// Python unpack: struct.unpack('<BBHIdddddddd', data)
class VizBridge {
public:
    static VizBridge& get() {
        static VizBridge inst;
        return inst;
    }

    static constexpr uint8_t MSG_STATE     = 0;
    static constexpr uint8_t MSG_RUN_START = 1;
    static constexpr uint8_t MSG_RUN_END   = 2;

    static constexpr uint8_t ENTITY_MISSILE = 0;
    static constexpr uint8_t ENTITY_TARGET  = 1;

    void init(const char* host, uint16_t port) {
        sockfd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) return;
        std::memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family      = AF_INET;
        addr_.sin_port        = htons(port);
        addr_.sin_addr.s_addr = ::inet_addr(host);
        int sndbuf = 1 << 18;   // 256 KB send buffer
        ::setsockopt(sockfd_, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    }

    void shutdown() {
        if (sockfd_ >= 0) { ::close(sockfd_); sockfd_ = -1; }
    }

    void send(uint8_t entity_id, uint16_t run_id,
              double t,
              double px, double py, double pz,
              double vx, double vy, double vz,
              double range = 0.0)
    {
        if (sockfd_ < 0) return;
        Packet p;
        p.msg_type  = MSG_STATE;
        p.entity_id = entity_id;
        p.run_id    = run_id;
        p.padding   = 0;
        p.t = t; p.px = px; p.py = py; p.pz = pz;
        p.vx = vx; p.vy = vy; p.vz = vz; p.range = range;
        ::sendto(sockfd_, &p, sizeof(p), MSG_DONTWAIT,
                 reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_));
    }

    void sendEvent(uint8_t msg_type, uint16_t run_id) {
        if (sockfd_ < 0) return;
        Packet p{};
        p.msg_type = msg_type;
        p.run_id   = run_id;
        ::sendto(sockfd_, &p, sizeof(p), MSG_DONTWAIT,
                 reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_));
    }

private:
    VizBridge()  = default;
    ~VizBridge() { shutdown(); }

    struct __attribute__((packed)) Packet {
        uint8_t  msg_type;
        uint8_t  entity_id;
        uint16_t run_id;
        uint32_t padding;
        double   t, px, py, pz, vx, vy, vz, range;
    };
    static_assert(sizeof(Packet) == 72, "VizBridge::Packet must be 72 bytes");

    int         sockfd_ = -1;
    sockaddr_in addr_{};
};
