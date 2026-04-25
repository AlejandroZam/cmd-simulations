#include "simdds.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/topic/TopicDescription.hpp>
#include <stdexcept>
#include <cstdio>

using namespace eprosima::fastdds::dds;

SimDDS& SimDDS::get() {
    static SimDDS instance;
    return instance;
}

void SimDDS::init(int domain) {
    if (participant_) return;

    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    // Synchronous in-process delivery: write() immediately delivers to matching
    // readers in the same process without going through transport or discovery.
    pqos.properties().properties().emplace_back(
        "fastdds.intraprocess_delivery", "full");

    participant_ = DomainParticipantFactory::get_instance()
                      ->create_participant(domain, pqos);
    if (!participant_)
        throw std::runtime_error("SimDDS: failed to create DomainParticipant");

    std::printf("[SimDDS] Initialized on domain %d (intraprocess delivery)\n", domain);
}

void SimDDS::shutdown() {
    if (!participant_) return;
    topics_.clear();
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
    participant_ = nullptr;
    std::printf("[SimDDS] Shutdown\n");
}

eprosima::fastdds::dds::DomainParticipant* SimDDS::participant() {
    if (!participant_)
        throw std::runtime_error("SimDDS: call init() before using participant");
    return participant_;
}

eprosima::fastdds::dds::Topic* SimDDS::get_or_create_topic(
        const std::string& topic_name,
        const std::string& type_name)
{
    auto it = topics_.find(topic_name);
    if (it != topics_.end()) return it->second;

    auto* desc = participant_->lookup_topicdescription(topic_name);
    if (desc) {
        auto* topic = dynamic_cast<Topic*>(desc);
        topics_[topic_name] = topic;
        return topic;
    }

    auto* topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT);
    if (!topic)
        throw std::runtime_error("SimDDS: failed to create topic: " + topic_name);

    topics_[topic_name] = topic;
    return topic;
}
