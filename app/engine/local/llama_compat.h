/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: llama_compat.h
** holds shims for fixing calls between llama.cpp versions
**
****************************************************************************/

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