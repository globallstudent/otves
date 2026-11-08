#pragma once

namespace otves {

// The field enum and value variant of an element type, so ops, the model store and the document
// are written once for every type. Each type specializes it next to its own definition, with
// validate(), get_field() and set_field() beside it.
template <typename T>
struct ElementTraits;

}  // namespace otves
