#include "Report.hpp"

#include <iomanip>
#include <sstream>

namespace gtasa {
namespace {

std::string escapeJson(const std::string& s) {
    std::ostringstream out;
    for (const unsigned char ch : s) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(ch) << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    return out.str();
}

const char* jsonBool(bool b) { return b ? "true" : "false"; }

void writeBlock(std::ostringstream& out, const char* key, const BlockInfo& b) {
    out << "\"" << key << "\":{";
    out << "\"found\":" << jsonBool(b.found)
        << ",\"name_offset\":" << b.nameOffset
        << ",\"payload_offset\":" << b.payloadOffset
        << ",\"payload_skip\":" << b.payloadSkip
        << ",\"length_prefixed\":" << jsonBool(b.lengthPrefixed) << '}';
}

} // namespace

std::string parseResultJson(const ParseResult& r, bool includeObjects) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    out << '{';
    out << "\"schema\":1";
    out << ",\"ok\":" << jsonBool(r.ok);
    out << ",\"save\":\"" << escapeJson(r.saveName) << "\"";
    out << ",\"error\":\"" << escapeJson(r.error) << "\"";
    out << ",\"header\":{";
    out << "\"parsed\":" << jsonBool(r.header.parsed)
        << ",\"magic\":\"" << escapeJson(r.header.magicHex) << "\""
        << ",\"version_a\":" << r.header.versionA
        << ",\"version_b\":" << r.header.versionB
        << ",\"versions_match\":" << jsonBool(r.header.versionFieldsMatch)
        << ",\"last_mission\":\"" << escapeJson(r.header.lastMissionKey) << "\""
        << ",\"checksum_status\":\"" << escapeJson(r.header.checksumStatus) << "\""
        << ",\"stored_checksum\":\"" << escapeJson(r.header.storedChecksumHex) << "\""
        << ",\"computed_112_checksum\":\"" << escapeJson(r.header.computedChecksumHex) << "\"}";
    out << ",\"blocks\":{";
    writeBlock(out, "PICKUPS", r.pickupsBlock); out << ',';
    writeBlock(out, "TAGS", r.tagsBlock); out << ',';
    writeBlock(out, "STUNTJUMPS", r.stuntJumpsBlock); out << '}';
    out << ",\"summary\":{";
    out << "\"tags\":{" << "\"completed\":" << r.summary.tagsCompleted << ",\"total\":" << r.summary.tagsTotal << "},";
    out << "\"snapshots\":{" << "\"completed\":" << r.summary.snapshotsCompleted << ",\"total\":" << r.summary.snapshotsTotal << "},";
    out << "\"horseshoes\":{" << "\"completed\":" << r.summary.horseshoesCompleted << ",\"total\":" << r.summary.horseshoesTotal << "},";
    out << "\"oysters\":{" << "\"completed\":" << r.summary.oystersCompleted << ",\"total\":" << r.summary.oystersTotal << "},";
    out << "\"stunt_jumps\":{" << "\"completed\":" << r.summary.stuntJumpsCompleted << ",\"total\":" << r.summary.stuntJumpsTotal << "}}";
    out << ",\"catalogue_reliable\":{";
    out << "\"snapshots\":" << jsonBool(r.snapshotsCatalogueReliable)
        << ",\"horseshoes\":" << jsonBool(r.horseshoesCatalogueReliable)
        << ",\"oysters\":" << jsonBool(r.oystersCatalogueReliable) << '}';
    out << ",\"catalogue_diagnostics\":\"" << escapeJson(r.catalogueDiagnostics) << "\"";
    out << ",\"scan_diagnostics\":\"" << escapeJson(r.scanDiagnostics) << "\"";
    out << ",\"object_count\":" << r.objects.size();
    out << ",\"missing_count\":" << r.missing.size();
    if (includeObjects) {
        out << ",\"objects\":[";
        for (std::size_t i = 0; i < r.objects.size(); ++i) {
            const auto& c = r.objects[i];
            if (i) out << ',';
            out << '{'
                << "\"type\":\"" << typeKey(c.type) << "\""
                << ",\"id\":" << c.id
                << ",\"completed\":" << jsonBool(c.completed)
                << ",\"found\":" << jsonBool(c.found)
                << ",\"x\":" << c.x
                << ",\"y\":" << c.y
                << ",\"z\":" << c.z
                << ",\"source_index\":" << c.sourceIndex
                << '}';
        }
        out << ']';
    }
    out << '}';
    return out.str();
}

} // namespace gtasa
