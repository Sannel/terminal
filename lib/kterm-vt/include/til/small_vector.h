// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// KTerm port: replaced MSVC-specific small_vector with std::vector alias.
// Small-buffer optimisation can be added later; correctness comes first.

#pragma once

#include <vector>

namespace til // Terminal Implementation Library
{
    template<typename T, size_t /*N*/ = 0>
    using small_vector = std::vector<T>;
}
