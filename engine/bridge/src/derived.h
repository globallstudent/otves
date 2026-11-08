#pragma once

#include <optional>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/model.h>
#include <otves/takeoff/rules.h>

#include "engine_generated.h"

// What the engine derives from the model for the app: plan outlines and the take-off. All are
// rebuilt whole; the session sends them only when the model or the content changed.
namespace otves::bridge {

using OutlinesOffset =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<schema::WallOutline>>>;
using LinesOffset =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<schema::QuantityLine>>>;

using IssueOffsets = std::vector<flatbuffers::Offset<schema::Issue>>;

// Every wall's joined outline, level by level, so walls on different floors never meet. Join
// issues go to `issues`.
[[nodiscard]] OutlinesOffset encode_outlines(flatbuffers::FlatBufferBuilder& builder,
                                             const Model& model, IssueOffsets& issues);

using OpeningOutlinesOffset =
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<schema::OpeningOutline>>>;

// Every opening whose wall exists, cut face to face through it, sorted by id.
[[nodiscard]] OpeningOutlinesOffset encode_openings(flatbuffers::FlatBufferBuilder& builder,
                                                    const Model& model);

struct TakeoffOffsets {
  LinesOffset lines;  // null when no content is loaded
  schema::ErrorCode error = schema::ErrorCode::None;
  flatbuffers::Offset<flatbuffers::String> message;
};

// The material lines of the content's rules, or the reason they could not run. Opening issues,
// found while measuring, go to `issues`.
[[nodiscard]] TakeoffOffsets encode_takeoff(flatbuffers::FlatBufferBuilder& builder,
                                            const Model& model,
                                            const std::optional<takeoff::RuleSet>& rules,
                                            IssueOffsets& issues);

}  // namespace otves::bridge
