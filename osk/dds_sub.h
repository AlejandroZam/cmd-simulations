#pragma once
#include "simdds.h"
#include "dds_types.h"
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <string>

// Subscribes to StateMsg on a named DDS topic.
// Call init() in Block::initialize() and take() in Block::update().
//
// take() returns the latest published value (zero-order hold).
// Returns false if no data has been published yet.
class SimSubscriber {
public:
    void init(const std::string& topic_name) {
        using namespace eprosima::fastdds::dds;

        auto* part = SimDDS::get().participant();

        TypeSupport type(new StateMsgType());
        type.register_type(part);

        auto* topic = SimDDS::get().get_or_create_topic(topic_name, type.get_type_name());

        auto* sub = part->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind  = RELIABLE_RELIABILITY_QOS;
        rqos.history().kind      = KEEP_LAST_HISTORY_QOS;
        rqos.history().depth     = 1;

        reader_ = sub->create_datareader(topic, rqos);
        topic_name_ = topic_name;
    }

    // Returns true and fills msg with the latest value.
    // Returns false if no data is available yet (e.g. first timestep).
    bool take(StateMsg& msg) {
        if (!reader_) return false;
        eprosima::fastdds::dds::SampleInfo info;
        return reader_->take_next_sample(&msg, &info) ==
               eprosima::fastdds::dds::RETCODE_OK && info.valid_data;
    }

    const std::string& topic_name() const { return topic_name_; }

private:
    eprosima::fastdds::dds::DataReader* reader_ = nullptr;
    std::string topic_name_;
};
