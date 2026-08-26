#include "SaveParser.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: inspect_save <GTASAsf...>\n";
        return 2;
    }
    gtasa::SaveParser parser;
    auto r = parser.parseFile(argv[1]);
    if (!r.ok) {
        std::cerr << "PARSE_ERROR: " << r.error << "\n";
        return 1;
    }
    std::cout << "PARSE_OK\n";
    std::cout << "PICKUPS=0x" << std::hex << r.pickupsOffset
              << " TAGS=0x" << r.tagsOffset
              << " STUNTJUMPS=0x" << r.stuntJumpsOffset << std::dec << "\n";
    std::cout << "tags=" << r.summary.tagsCompleted << '/' << r.summary.tagsTotal << '\n';
    std::cout << "snapshots=" << r.summary.snapshotsCompleted << '/' << r.summary.snapshotsTotal << '\n';
    std::cout << "horseshoes=" << r.summary.horseshoesCompleted << '/' << r.summary.horseshoesTotal << '\n';
    std::cout << "oysters=" << r.summary.oystersCompleted << '/' << r.summary.oystersTotal << '\n';
    std::cout << "stunt_jumps=" << r.summary.stuntJumpsCompleted << '/' << r.summary.stuntJumpsTotal << '\n';
    std::cout << "missing=" << r.missing.size() << '\n';
    return 0;
}
