#pragma once
#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastdds/rtps/common/SerializedPayload.hpp>
#include <cstring>
#include <cstdint>

// 3-D position + velocity state — the universal signal published by any moving model.
struct StateMsg {
    double t  = 0.0;
    double px = 0.0, py = 0.0, pz = 0.0;
    double vx = 0.0, vy = 0.0, vz = 0.0;
};

// TypeSupport for StateMsg. Uses raw memcpy serialization — correct for
// in-process (same-participant) delivery. For cross-process DDS, replace
// with CDR encoding via fastcdr.
class StateMsgType : public eprosima::fastdds::dds::TopicDataType {
public:
    StateMsgType() {
        set_name("StateMsg");
        max_serialized_type_size = static_cast<uint32_t>(sizeof(StateMsg));
        is_compute_key_provided  = false;
    }

    // Intraprocess: construct StateMsg in-place so FastDDS can deliver without
    // going through serialize/deserialize at all.
    bool construct_sample(void* memory) const override {
        new (memory) StateMsg();
        return true;
    }

    bool serialize(
            const void* data,
            eprosima::fastdds::rtps::SerializedPayload_t& payload,
            eprosima::fastdds::dds::DataRepresentationId_t) override
    {
        if (payload.max_size < sizeof(StateMsg)) return false;
        std::memcpy(payload.data, data, sizeof(StateMsg));
        payload.length = sizeof(StateMsg);
        return true;
    }

    bool deserialize(
            eprosima::fastdds::rtps::SerializedPayload_t& payload,
            void* data) override
    {
        if (payload.length < sizeof(StateMsg)) return false;
        std::memcpy(data, payload.data, sizeof(StateMsg));
        return true;
    }

    uint32_t calculate_serialized_size(
            const void*,
            eprosima::fastdds::dds::DataRepresentationId_t) override
    {
        return static_cast<uint32_t>(sizeof(StateMsg));
    }

    void* create_data()          override { return new StateMsg(); }
    void  delete_data(void* data) override { delete static_cast<StateMsg*>(data); }

    bool compute_key(
            eprosima::fastdds::rtps::SerializedPayload_t&,
            eprosima::fastdds::rtps::InstanceHandle_t&, bool) override { return false; }
    bool compute_key(
            const void* const,
            eprosima::fastdds::rtps::InstanceHandle_t&, bool) override { return false; }

    bool is_bounded() const override { return true; }
    bool is_plain(eprosima::fastdds::dds::DataRepresentationId_t) const override { return true; }
};
