#pragma once
#include "transport_catalogue.h"
#include <string_view>
#include <iostream>

namespace stat_reader {
    void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);
}

