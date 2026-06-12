#ifndef SOLTRACE_BBOX_CALCULATOR_H
#define SOLTRACE_BBOX_CALCULATOR_H

// #include "types.h"

#include <native_runner_types.hpp>

namespace SolTrace::EmbreeRunner
{
    bool get_bounds(const SolTrace::NativeRunner::TElement *st_element,
                    glm::vec3& min_coord_global,
                    glm::vec3& max_coord_global);

} // namespace SolTrace::EmbreeRunner

#endif
