// Created by moisrex on 5/25/20.

#ifndef WEBPP_APPLICATION_CONCEPTS_H
#define WEBPP_APPLICATION_CONCEPTS_H

#include "../std/concepts.h"
#include "./request_concepts.h"
#include "./response_concepts.h"
#include "./routes/context.h"

namespace webpp {

    // todo: change this
    template <typename T>
    concept Application = stl::default_initializable<T>/*&& requires(T app) {
        { app(fake_request_type{}) }
        ->Response;
    }*/;

} // namespace webpp

#endif // WEBPP_APPLICATION_CONCEPTS_H
