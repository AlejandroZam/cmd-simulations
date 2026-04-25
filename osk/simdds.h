#pragma once
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <string>
#include <unordered_map>

// Singleton DDS domain participant shared by all simulation models.
//
// Usage:
//   SimDDS::get().init();          // call once in main() before creating models
//   SimDDS::get().shutdown();      // call once in main() after deleting models
//
// SimPublisher and SimSubscriber call get_or_create_topic() internally —
// topics with the same name are created once and reused.
class SimDDS {
public:
    static SimDDS& get();

    void init(int domain = 0);
    void shutdown();

    eprosima::fastdds::dds::DomainParticipant* participant();

    // Returns an existing topic by name or creates a new one.
    // Type must already be registered with the participant before calling.
    eprosima::fastdds::dds::Topic* get_or_create_topic(
            const std::string& topic_name,
            const std::string& type_name);

private:
    SimDDS() = default;
    ~SimDDS() = default;

    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    std::unordered_map<std::string, eprosima::fastdds::dds::Topic*> topics_;
};
