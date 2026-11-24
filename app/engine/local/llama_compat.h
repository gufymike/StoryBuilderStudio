// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once
#include <llama.h>

namespace llama_compat {

inline llama_sampler *init_penalties(int32_t n_vocab,
                                     int32_t last_n,
                                     float repeat,
                                     float freq,
                                     float present)
{
    auto call = [&](auto fn) {
        if constexpr (requires { fn(last_n, repeat, freq, present); }) {
            return fn(last_n, repeat, freq, present);
        } else {
            return fn(n_vocab, last_n, repeat, freq, present);
        }
    };
    return call(llama_sampler_init_penalties);
}

} // namespace llama_compat