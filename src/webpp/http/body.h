#ifndef WEBPP_BODY_H
#define WEBPP_BODY_H

#include "../std/string_view.h"
#include <string>

/**
 * This class will not load, cache, or parse the body until the user needs it.
 * This means that we need access to the Interface so we can use it to read the
 * data of the body, then we parse it to the thing that the user needs.
 *
 */
namespace webpp {

    template <class Interface>
    class body {
      private:
        Interface _interface;

      public:
        void* const& json() const;

        /**
         * @brief get the whole body as a string
         * @return string_view
         */
        std::string_view string() const;
        std::istream const& stream() const;

        /**
         * Perfect forwarding the read method.
         */
        template <typename... T>
        auto read(T&&... data) const {
            return _interface.read(std::forward<T>(data)...);
        }

        // TODO: add more methods for the images and stuff
    };

}; // namespace webpp

#endif // WEBPP_BODY_H