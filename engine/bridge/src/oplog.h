#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/error.h>
#include <otves/core/op.h>

#include "engine_generated.h"

// The operation log as the job file stores it (docs/adr/0007): each group is a FlatBuffer of its
// own, which only the engine reads.
namespace otves::bridge {

inline constexpr const char* kGroupIdentifier = "OTVG";

// A finished OpGroup buffer with the identifier kGroupIdentifier.
[[nodiscard]] std::vector<std::uint8_t> encode_group(const OpGroup& group);

// kCorruptData when the buffer fails verification, misses a field or holds an unknown value;
// kUnsupportedVersion when a newer engine wrote it.
[[nodiscard]] Result<OpGroup> decode_group(std::span<const std::uint8_t> bytes);

// The group for the response: its index columns and its finished buffer.
[[nodiscard]] flatbuffers::Offset<schema::LogEntry> encode_log_entry(
    flatbuffers::FlatBufferBuilder& builder, const OpGroup& group);

}  // namespace otves::bridge
