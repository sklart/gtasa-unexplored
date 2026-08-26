#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace gtasa {
enum class CollectibleType { Tag, Snapshot, Horseshoe, Oyster, StuntJump, Count };
struct Collectible { CollectibleType type{}; int id{}; float x{}, y{}, z{}; bool completed{}; bool found{}; int sourceIndex{}; };
struct ParseSummary { int tagsCompleted{}; int tagsTotal{100}; int snapshotsCompleted{}; int snapshotsTotal{50}; int horseshoesCompleted{}; int horseshoesTotal{50}; int oystersCompleted{}; int oystersTotal{50}; int stuntJumpsCompleted{}; int stuntJumpsTotal{70}; };
struct BlockInfo { bool found{}; std::size_t nameOffset{}; std::size_t payloadOffset{}; std::size_t payloadSkip{}; bool lengthPrefixed{}; };
struct SaveHeader { bool parsed{}; std::string magicHex; int versionA{}; int versionB{}; bool versionFieldsMatch{}; std::string lastMissionKey; std::string checksumStatus; std::string storedChecksumHex; std::string computedChecksumHex; };
struct ParseResult { bool ok{}; std::string error; std::string saveName; SaveHeader header; BlockInfo pickupsBlock, tagsBlock, stuntJumpsBlock; std::size_t pickupsOffset{}; std::size_t tagsOffset{}; std::size_t stuntJumpsOffset{}; ParseSummary summary; bool snapshotsCatalogueReliable{}; bool horseshoesCatalogueReliable{}; bool oystersCatalogueReliable{}; std::string catalogueDiagnostics; std::string scanDiagnostics; std::vector<Collectible> objects; std::vector<Collectible> missing; };
const char* typeKey(CollectibleType type);
}
