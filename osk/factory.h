#pragma once
#include "block.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>

// ModelFactory — maps a type string to a Block constructor.
//
// Register your model type once (typically in main.cpp):
//   ModelFactory::reg<SpringMassDamper>("SpringMassDamper");
//
// Then create instances by name:
//   Block* b = ModelFactory::create("SpringMassDamper");

class ModelFactory {
public:
    using Creator = std::function<Block*()>;

    template <typename T>
    static void reg(const std::string& typeName) {
        table()[typeName] = []() -> Block* { return new T(); };
    }

    static Block* create(const std::string& typeName) {
        auto& t = table();
        auto  it = t.find(typeName);
        if (it == t.end())
            throw std::runtime_error("ModelFactory: unknown type '" + typeName + "'");
        return it->second();
    }

private:
    static std::unordered_map<std::string, Creator>& table() {
        static std::unordered_map<std::string, Creator> t;
        return t;
    }
};
