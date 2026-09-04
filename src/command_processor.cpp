#include "cache_server/command_processor.hpp"
#include "cache_server/store.hpp"

// constructor
CommandProcessor::CommandProcessor(Store& store):store_(store){};

// given a command, execute it on the store
std::string CommandProcessor::execute(const Command& command) {
    switch(command.type) {
        case (CommandType::SET):
            store_.set(command.key, command.value.value());
            return "OK";
        
        case (CommandType::GET):
            if (auto return_str = store_.get(command.key); return_str != std::nullopt) {
                return "VALUE " + return_str.value();
            }
            return "NOT_FOUND";
        
        case (CommandType::DELETE):
            if (store_.remove(command.key)) {
                return "INTEGER 1";
            }
            return "INTEGER 0";
        
        case (CommandType::EXISTS):
            if (store_.exists(command.key)) {
                return "INTEGER 1";
            }
            return "INTEGER 0";
        
        case (CommandType::EXPIRE):
            Store::ExpireResult result = store_.expire(command.key, command.lifetime.value());
            switch (result) {
                case (Store::ExpireResult::SUCCESS):
                    return "INTEGER 1";

                case Store::ExpireResult::KEY_NOT_FOUND:
                    return "INTEGER 0";

                case Store::ExpireResult::INVALID_DURATION:
                    return "ERROR invalid duration";
            }
            break;
    }

    return "";
}