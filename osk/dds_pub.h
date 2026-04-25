#pragma once
#include "simdds.h"
#include "dds_types.h"
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <string>

// Publishes StateMsg on a named DDS topic.
// Call init() in Block::initialize() and publish() in Block::report().
//
// Topic naming convention: "sim.<model_name>.state"
//   e.g. target instance named "target" → topic "sim.target.state"
class SimPublisher {
public:
    void init(const std::string& topic_name) {
        using namespace eprosima::fastdds::dds;

        auto* part = SimDDS::get().participant();

        TypeSupport type(new StateMsgType());
        type.register_type(part);

        auto* topic = SimDDS::get().get_or_create_topic(topic_name, type.get_type_name());

        auto* pub = part->create_publisher(PUBLISHER_QOS_DEFAULT);

        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.reliability().kind  = RELIABLE_RELIABILITY_QOS;
        wqos.history().kind      = KEEP_LAST_HISTORY_QOS;
        wqos.history().depth     = 1;

        writer_ = pub->create_datawriter(topic, wqos);
        topic_name_ = topic_name;
    }

    void publish(const StateMsg& msg) {
        if (writer_) writer_->write(const_cast<StateMsg*>(&msg));
    }

    const std::string& topic_name() const { return topic_name_; }

private:
    eprosima::fastdds::dds::DataWriter* writer_ = nullptr;
    std::string topic_name_;
};
