#pragma once

#include <string>

#include "cache_server/command.hpp"
#include "cache_server/store.hpp"

// Processor that takes commands and executes them on a store.
class CommandProcessor {
public:
    explicit CommandProcessor(Store& store);

    std::string execute(const Command& command);

private:
    Store& store_;
};