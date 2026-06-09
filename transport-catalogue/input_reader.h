#pragma once
#include <string_view>
#include <string>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace input_reader {
    struct CommandDescription {
        std::string command;
        std::string id;
        std::string description;

        explicit operator bool() const {
            return !command.empty();
        }

        bool operator!() const {
            return !operator bool();
        }
    };

    class InputReader {
    public:
        /* Parse string into structure and save in result in commands_ */
        void ParseLine(std::string_view line);

        void ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const;

    private:
        std::vector<CommandDescription> commands_;
    };

}