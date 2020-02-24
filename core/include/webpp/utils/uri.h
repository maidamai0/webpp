#ifndef WEBPP_UTILS_URI_H
#define WEBPP_UTILS_URI_H

#include "../validators/validators.h"
#include "casts.h"
#include "charset.h"
#include "ipv4.h"
#include "ipv6.h"
#include "strings.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace webpp {

    namespace is {
        /**
         * @brief check if scheme is correct or not
         * @param _scheme
         * @return
         */
        [[nodiscard]] bool scheme(std::string_view const& _scheme) noexcept;

    } // namespace is

    /**
     * @brief this function will decode parts of uri
     * @details this function is almost the same as "decodeURIComponent" in
     * javascript
     */
    template <std::size_t N>
    [[nodiscard]] std::optional<std::string>
    decode_uri_component(std::string_view const& encoded_str,
                         charset_t<N> const& allowed_chars) noexcept {

        int digits_left = 2;
        unsigned char decoded_char = 0;
        // FIXME: decoding is assigned but never used; check if the algorithm is
        // correct
        bool decoding = false;
        std::string res;
        for (const auto c : encoded_str) {
            if (decoding && digits_left) {
                decoded_char <<= 4;
                if (c >= '0' && c <= '9') { // DIGITS
                    decoded_char += c - '0';
                } else if (c >= 'A' && c <= 'F') { // UPPER_HEX
                    decoded_char += c - 'A' + 10;
                } else if (c >= 'a' && c <= 'f') { // LOWER_HEX
                    decoded_char += c - 'a' + 10;
                } else {
                    return std::nullopt; // not encoded well
                }
                --digits_left;

                if (digits_left == 0) {
                    decoding = false;
                    res.push_back(static_cast<char>(decoded_char));
                }
            } else if (c == '%') {
                decoding = true;

                // resetting:
                digits_left = 2;
                decoded_char = 0;
            } else {
                if (!allowed_chars.contains(c))
                    return std::nullopt; // bad chars
                res.push_back(c);
            }
        }
        return std::move(res);
    }

    /**
     * This method encodes the given URI element.
     * What we are calling a "URI element" is any part of the URI
     * which is a sequence of characters that:
     * - may be percent-encoded
     * - if not percent-encoded, are in a restricted set of characters
     *
     * @param[in] element
     *     This is the element to encode.
     *
     * @param[in] allowedCharacters
     *     This is the set of characters that do not need to
     *     be percent-encoded.
     *
     * @return
     *     The encoded element is returned.
     *
     *
     * @details this function is almost the same as "encodeURIComponent" in
     * javascript
     */
    template <std::size_t N>
    [[nodiscard]] std::string
    encode_uri_component(const std::string_view& element,
                         const webpp::charset_t<N>& allowedCharacters) {
        auto make_hex_digit = [](unsigned int value) {
            if (value < 10) {
                return static_cast<char>(value + '0');
            } else {
                return static_cast<char>(value - 10 + 'A');
            }
        };

        std::string encodedElement;
        for (auto c : element) {
            if (allowedCharacters.contains(c)) {
                encodedElement.push_back(c);
            } else {
                encodedElement.push_back('%');
                encodedElement.push_back(
                    make_hex_digit(static_cast<unsigned int>(c) >> 4u));
                encodedElement.push_back(
                    make_hex_digit(static_cast<unsigned int>(c) & 0x0Fu));
            }
        }
        return encodedElement;
    }

    /**
     * This class represents a Uniform Resource Identifier (URI),
     * as defined in RFC 3986 (https://tools.ietf.org/html/rfc3986).
     *
     * Most URIs will never change in their life time (at least in webpp
     * project) and they mostly used to get details of the URL we have as a
     * string; so I decided that probably half of the calculations can be done
     * at compile time; so fo that point, I reimplemented the URI class with
     *  and string_view in mind.
     *
     * RFC: https://tools.ietf.org/html/rfc3986
     *
     *    foo://example.com:8042/over/there?name=ferret#nose
     *    \_/   \______________/\_________/ \_________/ \__/
     *     |           |            |            |        |
     *  scheme     authority       path        query   fragment
     *     |   _____________________|__
     *    / \ /                        \
     *    urn:example:animal:ferret:nose
     *
     *  [protocol"://"[username[":"password]"@"]hostname[":"port]"/"?][path]["?"querystring]["#"fragment]
     */
    template <typename StringTypeRaw = std::string>
    class basic_uri {

        using StringType = std::conditional_t<
            std::is_empty_v<StringTypeRaw>, std::string,
            std::conditional_t<
                std::is_array_v<StringTypeRaw> ||
                    std::is_convertible_v<StringTypeRaw, char const* const> ||
                    std::is_const_v<StringTypeRaw>,
                std::string_view, StringTypeRaw>>;

        static_assert(std::is_same_v<StringType, std::string> ||
                          std::is_same_v<StringType, std::string_view>,
                      "The specified string type for URI is not supported.");

      public:
        /**
         * source:
         * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI
         */
        static constexpr auto ALLOWED_CHARACTERS_IN_URI = charset(
            ALPHA, DIGIT,
            charset_t<20>{';', ',', '/', '?', ':', '@', '&',  '=', '+', '$',
                          '-', '_', '.', '!', '~', '*', '\'', '(', ')', '#'});
        /**
         * This is the character set corresponds to the second part
         * of the "scheme" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986).
         */
        static constexpr auto SCHEME_NOT_FIRST =
            charset(ALPHA, DIGIT, charset_t<3>{'+', '-', '.'});

        /**
         * This is the character set corresponds to the "unreserved" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986).
         */
        static constexpr auto UNRESERVED =
            charset(ALPHA, DIGIT, charset_t<4>{'-', '.', '_', '~'});

        /**
         * This is the character set corresponds to the "sub-delims" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986).
         */
        static constexpr auto SUB_DELIMS = charset_t<11>(
            '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=');

        /**
         * This is the character set corresponds to the "userinfo" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986),
         * leaving out "pct-encoded".
         */
        static constexpr auto USER_INFO_NOT_PCT_ENCODED =
            charset(UNRESERVED, SUB_DELIMS, charset_t<1>{':'});

        /**
         * This is the character set corresponds to the last part of
         * the "IPvFuture" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986).
         */
        static constexpr auto IPV_FUTURE_LAST_PART =
            charset(UNRESERVED, SUB_DELIMS, charset_t<1>{':'});

        /**
         * This is the character set corresponds to the "reg-name" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986),
         * leaving out "pct-encoded".
         */
        static constexpr auto REG_NAME_NOT_PCT_ENCODED =
            charset(UNRESERVED, SUB_DELIMS);

        /**
         * This is the character set corresponds to the "pchar" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986),
         * leaving out "pct-encoded".
         */
        static constexpr auto PCHAR_NOT_PCT_ENCODED =
            charset(UNRESERVED, SUB_DELIMS, webpp::charset_t<2>{':', '@'});

        /**
         * This is the character set corresponds to the "query" syntax
         * and the "fragment" syntax
         * specified in RFC 3986 (https://tools.ietf.org/html/rfc3986),
         * leaving out "pct-encoded".
         */
        static constexpr auto QUERY_OR_FRAGMENT_NOT_PCT_ENCODED =
            charset(PCHAR_NOT_PCT_ENCODED, charset_t<2>{'/', '?'});

      private:
        /**
         * This is the whole url (if we need to own the uri ourselves)
         */
        StringType data;

        mutable std::size_t scheme_end = std::string_view::npos,
                            authority_start = std::string_view::npos,
                            user_info_end = std::string_view::npos,
                            port_start = std::string_view::npos,
                            authority_end = std::string_view::npos,
                            query_start = std::string_view::npos,
                            fragment_start = std::string_view::npos;

        /**
         * scheme    :    start=0       end=[0]
         * user_info :    start=[1]     end=[2]
         * host      :    start=[2|1]   end=[3|4|...]
         * port      :    start=[3]     end=[4]
         * path      :    start=[4]     end=[5]
         * query     :    start=[5]     end=[6]
         * fragment  :    start=[6]     end=[...]
         *
         * port_end == authority_end == path_start
         */

        /**
         * parse the scheme
         * this method will fill the "authority_start" and "scheme_end" vars
         */
        void parse_scheme() const noexcept {
            if (scheme_end != std::string_view::npos)
                return; // It's already parsed

            auto _data = this->string_view();

            // extracting scheme
            if (_data.starts_with("//")) {
                authority_start = 2;
                scheme_end = data.size(); // so we don't have to check again
                return;
            } else if (const auto colon = _data.find(':');
                       colon != std::string_view::npos) {
                auto __scheme = _data.substr(0, colon);
                if (ALPHA.contains(__scheme[0]) &&
                    __scheme.substr(1).find_first_not_of(
                        SCHEME_NOT_FIRST.string_view())) {
                    scheme_end = colon;

                    if (_data.substr(colon + 1, 2) == "//") {
                        authority_start = colon + 3;
                    } else {
                        // it should be a URN or an invalid URI at this point
                        authority_start = data.size();
                    }
                    return;
                }
            }

            scheme_end = authority_start = data.size();
        }

        /**
         * parse user info
         */
        void parse_user_info() const noexcept {
            if (user_info_end != std::string_view::npos)
                return; // It's already parsed

            parse_scheme(); // to get "authority_start"

            if (authority_start == data.size()) {
                user_info_end = data.size();
                return; // there's no user_info_end without authority_start
            }

            parse_path(); // to get "authority_end"

            auto _data = (std::is_same_v<StringType, std::string_view>
                              ? data
                              : std::string_view(data));

            user_info_end =
                _data.substr(authority_start, authority_end - authority_start)
                    .find_first_of('@');
            if (user_info_end == std::string_view::npos) {
                user_info_end = data.size();
            } else {
                user_info_end += authority_start;
            }
        }

        /**
         * parse path
         * this will make sure that the "authority_end" variable is filled
         */
        void parse_path() const noexcept {
            if (authority_end != std::string_view::npos)
                return; // It's already parsed

            parse_scheme(); // to get "authority_start"
            parse_query();  // to get "query_start"

            auto _data = this->string_view();

            auto starting_point =
                authority_start != data.size()
                    ? authority_start
                    : (scheme_end != data.size() &&
                               scheme_end != std::string_view::npos
                           ? scheme_end
                           : 0);
            authority_end =
                _data.substr(starting_point, query_start - starting_point)
                    .find_first_of('/');
            if (authority_end == std::string_view::npos) {
                authority_end = data.size();
            } else {
                authority_end += starting_point;
            }
        }

        /**
         * parse port
         * this makes sure that the "port_start" variable is filled
         */
        void parse_port() const noexcept {
            if (port_start != std::string_view::npos)
                return; // It's already parsed

            parse_user_info(); // to get "authority_start" and "user_info_end"

            if (authority_start == data.size()) {
                port_start = data.size();
                return; // there's no user_info_end without authority_start
            }

            parse_path(); // to get "authority_end"

            auto _data = this->string_view();

            auto starting_point =
                user_info_end != data.size() ? user_info_end : authority_start;
            port_start =
                _data.substr(starting_point, authority_end - starting_point)
                    .find_last_of(':');
            if (port_start == std::string_view::npos) {
                port_start = data.size(); // there's no port
            } else {
                port_start += starting_point;
                auto str_view = _data.substr(port_start + 1,
                                             authority_end - (port_start + 1));
                if (!is::digit(str_view)) {
                    port_start = data.size();
                }
            }
        }

        //        /**
        //         * parsing the authority parts of the basic_uri
        //         */
        //        void parse_authority_full() const noexcept {
        //
        //            parse_scheme();
        //            parse_path();
        //
        //            auto _data = (std::is_same_v<StringType, std::string_view>
        //                              ? data
        //                              : std::string_view(data))
        //                             .substr(authority_start, authority_end);
        //
        //            auto _authority_start = data.find("//");
        //            if (_authority_start != std::string_view::npos) {
        //                _data.remove_prefix(_authority_start + 2);
        //            }
        //            auto path_start = _data.find('/');
        //
        //            if (_authority_start != std::string_view::npos &&
        //            path_start != 0) {
        //
        //                auto port_start = _data.find(":", 0, path_start);
        //
        //                // extracting user info
        //                if (auto delim = _data.find("@", 0, path_start);
        //                    delim != std::string_view::npos) {
        //
        //                    pieces[_user_info] = delim;
        //                    _data.remove_prefix(delim + 1);
        //                }
        //
        //                // extracting host
        //
        //                /* IP future versions can be specified like this:
        //                 * (RFC 3986)
        //                 *
        //                 * IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
        //                 * IPvFuture = "v" 1*HEXDIG "." 1*( unreserved /
        //                 sub-delims
        //                 * / ":" )
        //                 */
        //                if (_data.starts_with('[')) { // IP Literal
        //                    if (_data.size() > 2 &&
        //                        _data[1] == 'v') { // IPv Future Number
        //                        if (auto dot_delim = _data.find('.');
        //                            dot_delim != std::string_view::npos) {
        //
        //                            auto ipvf_version = _data.substr(2,
        //                            dot_delim); if
        //                            (HEXDIG.contains(ipvf_version)) {
        //
        //                                if (auto ipvf_end = _data.find(']');
        //                                    ipvf_end !=
        //                                    std::string_view::npos) { auto
        //                                    ipvf =
        //                                        _data.substr(dot_delim + 1,
        //                                        ipvf_end);
        //                                    if
        //                                    (IPV_FUTURE_LAST_PART.contains(ipvf))
        //                                    {
        //                                        // returning the ipvf and it's
        //                                        // version
        //                                        _host = _data.substr(1,
        //                                        ipvf_end + 1);
        //                                    }
        //                                } else {
        //                                    // totally not a URI
        //                                    return;
        //                                }
        //                            }
        //                        }
        //                    } else if (_data.size() >= 5) { // IPv6
        //
        //                        if (auto ipv6_end = _data.find(']');
        //                            ipv6_end != std::string_view::npos) {
        //
        //                            if (auto ipv6_view = _data.substr(1,
        //                            ipv6_end);
        //                                is::ipv6(ipv6_view)) {
        //                                // TODO: probably use
        //                                std::variant<ipv6,
        //                                // ipv4, string>
        //                                _host = ipv6_view;
        //                                _data.remove_prefix(ipv6_end + 1);
        //                            }
        //                        } else {
        //                            return; // totally not a valid URI
        //                        }
        //                    }
        //                } else { // Not IP Literal
        //                    auto port_or_path_start =
        //                        port_start != std::string_view::npos ?
        //                        port_start
        //                                                             :
        //                                                             path_start;
        //                    auto hostname = _data.substr(0,
        //                    port_or_path_start);
        //                    // we're not going to decode hostname here. We'll
        //                    do
        //                    // this in another method because this function is
        //                    //  and will only return const stuff
        //
        //                    // we have our answer but we will check for the
        //                    // correctness of the hostname now
        //                    auto HOSTNAME_CHARS =
        //                        charset(REG_NAME_NOT_PCT_ENCODED,
        //                        charset('%'));
        //                    if (HOSTNAME_CHARS.contains(hostname)) {
        //                        _host = hostname;
        //                        if (port_or_path_start !=
        //                        std::string_view::npos)
        //                            _data.remove_prefix(port_or_path_start);
        //                        else {
        //                            return;
        //                        }
        //                    }
        //                }
        //
        //                // extracting port
        //                if (port_start != std::string_view::npos) {
        //                    auto port_end =
        //                        _data.find_first_not_of(DIGIT.string_view());
        //                    _port = _data.substr(port_start + 1, port_end);
        //                    _data.remove_prefix(port_end);
        //                }
        //            }
        //        }

        /**
         * parse fragment (it finds fragment_start)
         */
        void parse_fragment() const noexcept {
            if (fragment_start != std::string_view::npos)
                return; // It's already parsed

            auto _data = this->string_view();

            fragment_start = _data.find_first_of('#');
            if (fragment_start == std::string_view::npos) {
                fragment_start = data.size();
            }
        }

        /**
         * parse query; it ensures that query_start and fragment_start are
         * changed
         */
        void parse_query() const noexcept {
            if (query_start != std::string_view::npos)
                return; // there's nothing to do

            parse_fragment();

            auto _data = this->string_view();
            query_start = _data.substr(0, fragment_start).find_first_of('?');
            if (query_start == std::string_view::npos) {
                query_start = data.size();
            }
        }

        /**
         * parse the host
         */
        void parse_host() const noexcept {
            parse_user_info(); // to get "authority_start" and "user_info_end"
            parse_port(); // to get "port_start" and hopefully "authority_end"
            parse_path(); // to make sure we have "authority_end"
        }

        /**
         * Remove the cache and make sure calling the functions will cause
         * re-parsing the uri.
         */
        inline void unparse() const noexcept {
            scheme_end = authority_start = user_info_end = port_start =
                authority_end = query_start = fragment_start =
                    std::string_view::npos;
        }

        /**
         * Replace the specified part with the specified replacement
         */
        void replace_value(std::size_t start, std::size_t len,
                           std::string_view const& replacement) noexcept {
            static_assert(std::is_same_v<StringType, std::string>,
                          "You cannot change a const_uri (string_view is not "
                          "modifiable)");
            if (start == std::string_view::npos ||
                len == std::string_view::npos ||
                (len == 0 && replacement.empty()))
                return;
            std::stringstream _data;
            _data << substr(0, start) << replacement
                  << substr(std::min(data.size(), start + len));
            data = _data.str();
            unparse();
            // TODO: you may want to not unparse everything
        }

      public:
        basic_uri() noexcept {
            static_assert(
                std::is_same_v<StringType, std::string>,
                "There's no point using string_view with empty value as a "
                "const basic_uri. Consider using basic_uri<std::string>() "
                "instead.");
        }

        ~basic_uri() noexcept = default;

        /**
         * @brief parse from string, it will trim the spaces for generality too
         * @param string_view URI string
         */
        explicit basic_uri(StringType const& u) noexcept : data(u) {}

        /**
         * If the user uses this
         * @param u
         */
        explicit basic_uri(StringType&& u) noexcept : data(std::move(u)) {}

        basic_uri(basic_uri const&) = default;

        basic_uri(basic_uri&&) noexcept = default;

        // assignment operators
        basic_uri& operator=(basic_uri const& u) = default;

        basic_uri& operator=(basic_uri&& u) noexcept = default;

        basic_uri& operator=(StringType const& u) noexcept {
            data = u;
            unparse();
            return *this;
        }

        basic_uri& operator=(StringType&& u) noexcept {
            data = std::move(u);
            unparse();
            return *this;
        }

        bool operator==(const basic_uri& other) const noexcept {
            return str() == other.str();
        }

        bool operator==(std::string_view const& u) const noexcept {
            return str() == u;
        }

        bool operator!=(std::string_view const& u) const noexcept {
            return !operator==(u);
        }

        bool operator!=(const basic_uri& other) const noexcept {
            return !operator==(other);
        }

        /**
         * Get a part of the uri
         * @param start
         * @param len
         * @return std::string_view
         */
        [[nodiscard]] std::string_view
        substr(std::size_t const& start = 0,
               std::size_t const& len = std::string_view::npos) const noexcept {
            if (len == 0)
                return {};
            if constexpr (std::is_same_v<StringType, std::string_view>) {
                return data.substr(start, len);
            } else {
                return std::string_view(data).substr(start, len);
            }
        }

        /**
         * @brief this function is the same as "encodeURI" in javascript
         */
        [[nodiscard]] std::string encoded_uri() noexcept {
            return encode_uri_component<ALLOWED_CHARACTERS_IN_URI.size()>(
                str(), ALLOWED_CHARACTERS_IN_URI);
        }

        /**
         * @brief this function is the same as "decodeURI" in javascript
         * @return this function will return an optional<string> object. it will
         * be nullopt when the uri is not valid and has invalid characters
         */
        [[nodiscard]] auto decoded_uri() noexcept {
            return decode_uri_component<ALLOWED_CHARACTERS_IN_URI.size()>(
                str(), ALLOWED_CHARACTERS_IN_URI);
        }

        /**
         * @brief check if the specified uri has a scheme or not
         */
        [[nodiscard]] bool has_scheme() const noexcept {
            parse_scheme();
            return scheme_end != data.size() || scheme_end == 0;
        }

        /**
         * @brief scheme
         * @return get scheme
         */
        [[nodiscard]] std::string_view scheme() const noexcept {
            parse_scheme();
            return scheme_end == data.size() ? std::string_view()
                                             : substr(0, scheme_end);
        }

        /**
         * @brief set scheme
         * @param _scheme
         * @throws logic_error if uri is const
         */
        basic_uri& scheme(std::string_view __scheme) {
            if (__scheme.ends_with(':'))
                __scheme.remove_suffix(1);
            if (!is::scheme(__scheme))
                throw std::invalid_argument(
                    "The specified scheme is not valid");
            parse_scheme();
            if (scheme_end != data.size()) {
                replace_value(0,
                              __scheme.empty() &&
                                      data.size() > scheme_end + 1 &&
                                      data[scheme_end] == ':'
                                  ? scheme_end + 1
                                  : scheme_end,
                              __scheme);
            } else {
                // the URI doesn't have a scheme now, we have to put it in the
                // right place
                auto scheme_colon =
                    __scheme.empty() ? "" : std::string(__scheme) + ':';
                if (authority_start != data.size()) {
                    replace_value(0, 0,
                                  scheme_colon +
                                      (std::string_view(data).starts_with("//")
                                           ? ""
                                           : "//"));
                } else {
                    // It's a URN (or URN like URI)
                    replace_value(0, 0, scheme_colon);
                }
            }
            return *this;
        }

        /**
         * @brief clear scheme from uri
         */
        basic_uri& clear_scheme() noexcept { return scheme({}); }

        /**
         * @brief checks if the uri has user info or not
         */
        [[nodiscard]] bool has_user_info() const noexcept {
            parse_user_info();
            return user_info_end != data.size() &&
                   authority_start != data.size();
        }

        /**
         * @brief get the user info or an empty value
         */
        [[nodiscard]] std::string_view user_info() const noexcept {
            parse_user_info();
            return (user_info_end == data.size() ||
                    authority_start == data.size())
                       ? std::string_view()
                       : substr(authority_start,
                                user_info_end - authority_start);
        }

        /**
         * @brief decode user_info and return it as a string
         */
        [[nodiscard]] auto user_info_decoded() const noexcept {
            return decode_uri_component(user_info(), USER_INFO_NOT_PCT_ENCODED);
        }

        /**
         * @brief set the user info if it's possible
         */
        basic_uri& user_info(std::string_view const& info) noexcept {
            parse_user_info();
            auto encoded_info =
                encode_uri_component(info, USER_INFO_NOT_PCT_ENCODED);
            if (user_info_end == data.size() ||
                authority_start == data.size()) {
                // the URI already has user info, I just have to replace it
                replace_value(authority_start, user_info_end - authority_start,
                              encoded_info);
            } else {
                // I don't know where is it, let's find it

                if (authority_start == data.size()) {
                    // this URI doesn't have authority in it, but I'm going to
                    // insert authority into it because the user is obviously
                    // demanding it; of course after this, the URI won't be a
                    // valid URI until the user sets at lease scheme too.

                    parse_scheme(); // to get "scheme_end"
                    if (scheme_end == data.size()) {
                        // there's no scheme either
                        replace_value(0, 0,
                                      std::string("//") + encoded_info + "@");
                    } else {
                        // there's scheme and we have to put it after that
                        replace_value(scheme_end + 1, 0,
                                      std::string("//") + encoded_info + "@");
                    }
                } else {
                    // there's authority start but there's no user_info_end
                    replace_value(authority_start, 0, encoded_info + "@");
                }
            }
            return *this;
        }

        /**
         * @brief clears the user info if exists
         */
        basic_uri& clear_user_info() noexcept { return user_info({}); }

        /**
         * @brief return host as an string_view
         * @return string_view
         */
        [[nodiscard]] std::string_view host() const noexcept {
            parse_host();
            if (authority_start == data.size()) {
                // there will not be a host without the authority_start
                return {};
            }

            std::size_t start, len;

            // you know I could do this in one line of code, but I did this
            // because I don't want you to curse me :)

            // we have authority_start, let's check user_info and port too
            if (user_info_end == data.size()) {
                // there's no user info
                start = authority_start;
            } else {
                // there's a user info
                start = user_info_end;
            }

            if (port_start != data.size()) {
                // but there's a port
                len = port_start - start;
            } else {
                // there's no port either
                if (authority_end != data.size()) {
                    // there's a path
                    len = authority_end - start;
                } else {
                    // there's no path either
                    len = data.size() - 1; // till the end
                }
            }

            return substr(start, len);
        }

        /**
         * @brief returns const_ipv4/const_ipv6/hostname; if the URI doesn't
         * include a valid ip/hostname you'll get an empty string. this method
         * will only return the hostname/ip if it's in the correct format and
         * doesn't include invalid syntax.
         * @return
         */
        [[nodiscard]] std::variant<ipv4, ipv6, std::string_view>
        host_structured() const noexcept {
            auto _host = host();
            if (is::ipv4(_host))
                return ipv4(_host);
            if (is::ipv6(_host))
                return ipv6(_host);
            return _host;
        }

        /**
         * @brief get the decoded version of hostname/ip of the uri or an empty
         * string if the specified URI does not include a hostname/ip or its
         * hostname has the wrong character encodings.
         * @return string
         */
        [[nodiscard]] auto host_decoded() const noexcept {
            return decode_uri_component(host(), REG_NAME_NOT_PCT_ENCODED);
        }

        /**
         * @brief this method will check if the hostname/ip exists in the uri or
         * not.
         * @return true if it find a hostname/ip in the uri
         */
        [[nodiscard]] bool has_host() const noexcept { return !host().empty(); }

        /**
         * @brief set the hostname/ip in the uri if possible
         */
        basic_uri& host(std::string_view const& new_host) noexcept {
            parse_host();

            // todo: are you sure it can handle punycode as well?
            std::string encoded_host =
                encode_uri_component(new_host, REG_NAME_NOT_PCT_ENCODED);
            if ((!new_host.starts_with('[') || !new_host.ends_with(']')) &&
                is::ipv6(new_host)) {
                encoded_host = '[' + encoded_host + ']';
            }

            if (authority_start == data.size()) {
                // there's no authority start

                if (encoded_host.empty())
                    return *this; // there's nothing to do here. It's already
                                  // what the user wants

                if (scheme_end == data.size()) {
                    // there's no scheme either, so we just have to add to the
                    // beginning of the string
                    replace_value(0, 0, std::string("//") + encoded_host);
                    return *this;
                } else {
                    // there's a scheme
                    replace_value(scheme_end, 0,
                                  std::string("//") + encoded_host);
                    return *this;
                }
            }

            std::size_t start, finish;

            // you know I could do this in one line of code, but I did this
            // because I don't want you to curse me :)

            // we have authority_start, let's check user_info and port too
            if (user_info_end == data.size()) {
                // there's no user info
                if (scheme_end == data.size()) {
                    start = 0;
                    if (!new_host.empty() && !encoded_host.starts_with("//")) {
                        encoded_host = "//" + encoded_host;
                    }
                } else {
                    start = authority_start;
                }
            } else {
                // there's a user info
                start = user_info_end;
            }

            if (port_start != data.size()) {
                // but there's a port
                finish = port_start;
            } else {
                // there's no port either
                if (authority_end != data.size()) {
                    // there's a path
                    finish = authority_end;
                } else {
                    // there's no path either
                    finish = data.size() - 1; // till the end
                }
            }

            replace_value(start, finish - start, encoded_host);

            return *this;
        }

        /**
         * @brief get the hostname/ipv4/ipv6 from the URI. if the URI doesn't
         * include a hostname/ip or its hostname/ip is not in a valid shape, it
         * will return an empty string
         * @return string/ipv4/ipv6
         * @default empty string
         */
        [[nodiscard]] std::variant<ipv4, ipv6, std::string>
        host_structured_decoded() const noexcept {
            if (auto _host_structured = host_structured();
                std::holds_alternative<std::string_view>(_host_structured))
                return decode_uri_component(
                    std::get<std::string_view>(_host_structured),
                    REG_NAME_NOT_PCT_ENCODED);
            else
                return _host_structured;
        }

        /**
         * @brief clear host part from URI
         * @return
         */
        basic_uri& clear_host() noexcept { return host({}); }

        /**
         * Check if the specified host is an IP address or not
         * This method check if the specified host is a IPv4 Address or has a
         * valid syntax for either IPv6 or Future version of IP addresses which
         * also starts with "[" and ends with "]" with an unknown syntax (as of
         * writing this code obv)
         * @return an indication of weather or not the specified Hostname is a
         * valid IP address or not
         */
        [[nodiscard]] bool is_ip() const noexcept {
            auto _host = host();
            return is::ipv4(_host) ||
                   (_host.starts_with('[') && _host.ends_with(']'));
        }

        /**
         * Get the host and split it by dot separator. TLD (Top Level Domain)
         * will be the last one and Second Level Domain will be the one before
         * that and the rest will be subdomains.
         */
        [[nodiscard]] std::vector<std::string> domains() const noexcept {
            auto _host = host();
            if (_host.empty() || is_ip())
                return {};
            std::vector<std::string> subs;
            for (;;) {
                auto dot = _host.find('.');
                auto sub = _host.substr(0, dot);
                if (sub.empty())
                    break;
                subs.emplace_back(std::move(sub));
                if (dot == std::string_view::npos)
                    break;
                _host.remove_prefix(dot + 1);
            }
            return subs;
        }

        /**
         * Get the TLD (top level domain) or sometimes called extension
         */
        [[nodiscard]] std::string_view top_level_domain() const noexcept {
            auto _host = host();
            if (_host.empty() || is_ip())
                return {};
            auto dot = _host.find_last_of('.');
            return _host.substr(dot != std::string_view::npos ? dot + 1 : 0);
        }

        /**
         * Set the TLD (Top Level Domain) in the uri
         * @param tld
         * @return
         */
        basic_uri& top_level_domain(std::string_view const& tld) noexcept {
            auto _host = host();
            if (_host.empty()) {
                // I've already written that code. Yay, I'm so happy
                static_cast<void>(host(tld));
            } else if (!is_ip() && !is::ip(tld)) {
                // cannot put an ip address as a tld, user should use set host
                // instead of this method.
                auto dot = _host.find_last_of('.');
                auto start = dot != std::string_view::npos ? dot + 1 : 0;
                static_cast<void>(host(std::string(_host.substr(0, start)) +
                                       std::string(tld)));
            }
            return *this;
        }

        /**
         * Check if the specified uri has a top level domain (TLD) or not
         * @return an indication of weather or not the URI has TLD or not
         */
        [[nodiscard]] bool has_top_level_domain() const noexcept {
            return !top_level_domain().empty();
        }

        /**
         * Get the second level domain out of the host
         */
        [[nodiscard]] std::string_view second_level_domain() const noexcept {
            auto _host = host();
            if (_host.empty() || is_ip())
                return {};
            auto last_dot = _host.find_last_of('.');
            if (last_dot == std::string_view::npos)
                return {};
            auto bef_last_dot = _host.find_last_of('.', last_dot - 1);
            auto start =
                bef_last_dot == std::string_view::npos ? 0 : bef_last_dot + 1;
            auto sld = _host.substr(start, last_dot - start);
            return sld;
        }

        /**
         * Set the second level domain to the specified string.
         * Attention: this method will only work if Top Level Domain already
         * exists
         * @param sld
         */
        basic_uri& second_level_domain(std::string_view const& sld) noexcept {
            auto _host = host();
            if (_host.empty() || is_ip())
                return *this;

            auto last_dot = _host.find_last_of('.');
            if (last_dot == std::string_view::npos) {
                // we have to insert it at the beginning of the host string

                // there's nothing to do it's empty
                if (!sld.empty()) {
                    static_cast<void>(
                        host(std::string(sld) + '.' + std::string(_host)));
                }
            } else {
                auto bef_last_dot = _host.find_last_of('.', last_dot - 1);
                auto start = bef_last_dot == std::string_view::npos
                                 ? 0
                                 : bef_last_dot + 1;
                if (!sld.empty())
                    static_cast<void>(host(
                        std::string(_host.substr(0, start)) + std::string(sld) +
                        std::string(_host.substr(last_dot))));
                else
                    static_cast<void>(
                        host(std::string(_host.substr(last_dot + 1))));
            }
            return *this;
        }

        /**
         * This method will remove the Second Level Domain and also any
         * Sub-Domains if there are any.
         */
        basic_uri& clear_second_level_domain() noexcept {
            return second_level_domain({});
        }

        /**
         * Check if the specified uri has a second level domain or not.
         * @return An indication of weather or not the URI has SLD or not
         */
        [[nodiscard]] bool has_second_level_domain() const noexcept {
            return !second_level_domain().empty();
        }

        /**
         * Get the sub-domain (with sub-sub-...-sub-domain)
         * @return
         */
        [[nodiscard]] std::string_view subdomains() const noexcept {
            auto _host = host();
            if (_host.empty() || is_ip())
                return {};
            auto last_dot = _host.find_last_of('.');
            if (last_dot == std::string_view::npos)
                return {};
            auto bef_last_dot = _host.find_last_of('.', last_dot - 1);
            if (bef_last_dot == std::string_view::npos)
                return {};
            return _host.substr(0, bef_last_dot);
        }

        /**
         * Set the sub-domain part of the host name
         * Attention: this method will only work if Top Level Domain and Second
         * Level Domain already exists
         * @param sds
         */
        basic_uri& subdomains(std::string_view const& sds) noexcept {
            auto _host = host();
            if (_host.empty() || is_ip())
                return *this;
            auto last_dot = _host.find_last_of('.');
            if (last_dot == std::string_view::npos)
                return *this;
            auto bef_last_dot = _host.find_last_of('.', last_dot - 1);
            if (bef_last_dot == std::string_view::npos)
                return *this;
            if (sds.empty()) // special check for when we want to remove the SDS
                bef_last_dot++;
            static_cast<void>(host(std::string(sds) +
                                   std::string(_host.substr(bef_last_dot))));
            return *this;
        }

        /**
         * Remove the sub-domains if exists. This method should not have
         * side-effects if there's no sub-domain
         */
        basic_uri& clear_subdomains() noexcept { return subdomains({}); }

        /**
         * Check if the specified uri has at least one sub-domain or not
         * @return an indication of weather or not the URI has subdomains or not
         * TODO: we could be smarter here
         */
        [[nodiscard]] bool has_subdomains() const noexcept {
            return !subdomains().empty();
        }

        /**
         * Get the default port for the specified scheme
         * TODO: make this function a free function
         */
        [[nodiscard]] uint16_t default_port() const noexcept {
            auto _scheme = scheme();
            if (_scheme == "http")
                return 80u;
            if (_scheme == "https")
                return 443u;
            if (_scheme == "ftp")
                return 21u;
            if (_scheme == "ssh")
                return 22u;
            if (_scheme == "telnet")
                return 23u;
            if (_scheme == "ftps")
                return 990u;

            // TODO: add more protocols here
            return 0u;
        }

        /**
         * @brief port number of the uri;
         * @return port number
         * @default 80
         */
        [[nodiscard]] uint16_t port_uint16() const noexcept {
            if (has_port()) {
                return static_cast<uint16_t>(to_uint(port()));
            }
            return default_port();
        }

        /**
         * Get the port in a string_view format; there's no default value.
         * @return string_view
         */
        [[nodiscard]] std::string_view port() const noexcept {
            parse_port();

            // there's no port
            if (port_start == data.size())
                return {};

            // don't worry authority_end will be the end of the string anyway
            return substr(port_start + 1,
                          (authority_end == data.size() ? authority_end - 1
                                                        : authority_end) -
                              (port_start + 1));
        }

        /**
         * @brief checks if the uri has explicitly specified a port number or
         * not
         * @return bool
         */
        [[nodiscard]] bool has_port() const noexcept {
            parse_port();
            return port_start != data.size();
        }

        /**
         * @brief set port
         * @param new_port
         * @return
         */
        basic_uri& port(uint16_t new_port) noexcept {
            return port(std::to_string(new_port));
        }

        /**
         * Set new port value
         * @param new_port
         */
        basic_uri& port(std::string_view new_port) noexcept {
            if (new_port.starts_with(':'))
                new_port.remove_prefix(1);
            if (!is::digit(new_port))
                throw std::invalid_argument("The specified port is not valid");
            parse_port();
            if (port_start == data.size()) {
                // there's no port, I have to insert it myself:
                parse_host();

                if (authority_end != data.size()) {
                    // found it at the end of the line
                    replace_value(authority_end, 0,
                                  ":" + std::string(new_port));
                } else if (user_info_end != data.size()) {
                    // there's authority and there might be a host
                    replace_value(user_info_end + 1, user_info_end + 1,
                                  ":" + std::string(new_port));
                } else if (authority_start != data.size()) {
                    // there's a authority_start at least
                    replace_value(authority_start + 1, 0,
                                  ":" + std::string(new_port));
                } else {
                    // there's no authority at all.
                    if (scheme_end == data.size()) {
                        // there's no scheme either
                        replace_value(0, 0, "//:" + std::string(new_port));
                    } else {
                        // there's scheme
                        replace_value(scheme_end + 1, 0,
                                      "//:" + std::string(new_port));
                    }
                }
            } else {
                // there's a port and we are going to just replace it
                replace_value(port_start + 1,
                              authority_end == data.size() ? authority_end - 1
                                                           : authority_end,
                              new_port);
            }
            return *this;
        }

        /**
         * @brief clear the port number from the uri and defaults to 80
         * @return self
         */
        basic_uri& clear_port() noexcept { return port({}); }

        /**
         * @brief check if the URI has a path or not
         * @return
         */
        [[nodiscard]] bool has_path() const noexcept {
            parse_path();
            return authority_end != data.size();
        }

        /**
         * @brief get path in non-decoded, string format
         * @return
         */
        [[nodiscard]] std::string_view path() const noexcept {
            if (!has_path())
                return {};
            return substr(authority_end, std::min(query_start, fragment_start) -
                                             authority_end);
        }

        /**
         * @brief decoded path as a string
         * @return
         */
        [[nodiscard]] auto path_decoded() const noexcept {
            return decode_uri_component(
                path(), charset(PCHAR_NOT_PCT_ENCODED, charset('/')));
        }

        /**
         * @brief get the path as the specified type
         * @details this method will returns a vector/list/... of
         * string/string_views
         * this method does not just response to the fact that Container should
         * be an std container, but if string/string_view is presented as a
         * container, it will return the whole path.
         */
        template <typename Container = std::vector<std::string_view>>
        [[nodiscard]] Container path_structured() const noexcept {
            auto _path = path();
            if (_path.empty())
                return {};
            Container container;
            std::size_t slash_start = 0;
            std::size_t last_slash_start = 0;
            auto _path_size = _path.size();
            if (_path.front() == '/')
                container.emplace_back(); // empty string
            do {
                slash_start = _path.find('/', last_slash_start + 1);
                container.emplace_back(_path.data() + last_slash_start + 1,
                                       std::min(slash_start, _path_size) - last_slash_start - 1);
                // if (slash_start != std::string_view::npos)
                // _path.remove_prefix(slash_start + 1);
                // else
                // _path.remove_prefix(_path.size());
                last_slash_start = slash_start;
            } while (slash_start != std::string_view::npos);
            return container;
        }

        /**
         * @brief this one will return a container containing decoded strings of
         * the path.
         * @attention do not use string_view or any alternatives for this method
         * as this method should own its data.
         */
        template <typename Container = std::vector<std::string>>
        [[nodiscard]] Container path_structured_decoded() const noexcept {
            Container container;
            for (auto const& slug : path_structured()) {
                container.push_back(
                    decode_uri_component(slug, PCHAR_NOT_PCT_ENCODED)
                        .value_or((typename Container::value_type)(slug)));
            }
            return container;
        }

        /**
         * @brief set path
         */
        template <
            typename Container,
            typename = std::enable_if_t<std::negation_v<std::is_convertible_v<
                Container::value_type, std::string_view>>>>
        basic_uri& path(const Container& __path) noexcept {
            static_assert(std::is_convertible_v<typename Container::value_type,
                                                std::string_view>,
                          "the specified container is not valid");
            return path(__path.cbegin(), __path.cend());
        }

        /**
         * Set path by begin and end of an iterator
         * @tparam Iter
         * @param _start
         * @param _end
         * @return
         */
        template <typename Iter>
        basic_uri& path(const Iter& _start, const Iter& _end) noexcept {
            std::ostringstream joined_path;
            // TODO: check if we need std::string here
            copy(_start, _end - 1,
                 std::ostream_iterator<std::string_view>(joined_path, "/"));
            joined_path << *std::prev(_end);
            return path(std::string_view(joined_path.str()));
        }

        /**
         * @brief set the path for the uri
         * @param _path
         * @return
         */
        basic_uri& path(std::string_view const& __path) noexcept {
            parse_path();
            auto _encoded_path =
                (__path.starts_with('/') ? "" : "/") +
                encode_uri_component(
                    __path, charset(PCHAR_NOT_PCT_ENCODED, charset('/')));

            replace_value(authority_end, query_start - authority_end,
                          _encoded_path);
            return *this;
        }
        /**
         * @brief clear path from the URI
         * @return
         */
        basic_uri& clear_path() noexcept { return path(std::string_view{}); }

        /**
         * @brief checks if the path is an absolute path or relative path
         * @return
         */
        [[nodiscard]] bool is_absolute() const noexcept {
            return path().starts_with('/');
        }

        /**
         * @brief checks if the path is a relative path or an absolute one
         * @return
         */
        [[nodiscard]] bool is_relative() const noexcept {
            return !is_absolute();
        }

        /**
         * @brief checks if the uri has query or not
         * @return
         */
        [[nodiscard]] bool has_query() const noexcept {
            parse_query();
            return query_start != data.size();
        }

        [[nodiscard]] std::string_view query() const noexcept {
            parse_query();
            if (query_start == data.size())
                return {};
            return substr(query_start + 1, fragment_start - query_start - 1);
        }

        /**
         * @brief set query
         * @param _query
         * @return
         */
        basic_uri& query(std::string_view const& __query) {
            if (!is::query(__query))
                throw std::invalid_argument(
                    "The specified string is not a valid query");

            auto encoded_query =
                (__query.starts_with('?') ? "" : "?") +
                encode_uri_component(__query,
                                     QUERY_OR_FRAGMENT_NOT_PCT_ENCODED);

            parse_query();

            if (query_start != data.size()) {
                // we don't have a query
                if (fragment_start != data.size()) {
                    replace_value(fragment_start, 0, encoded_query);
                } else {
                    parse_path();
                    if (authority_end == data.size()) {
                        // we don't even have authority_end
                        parse_scheme();
                        if (authority_start == data.size()) {
                            // there's no authority_start
                            if (scheme_end == data.size()) {
                                // it's an empty string
                                replace_value(0, 0, "///" + encoded_query);
                            } else {
                                replace_value(scheme_end, 0,
                                              "/" + encoded_query);
                            }
                        } else {
                            replace_value(authority_start, 0,
                                          "/" + encoded_query);
                        }
                    } else {
                        // we have authority_end
                        if (data[authority_end] == '/') {
                            replace_value(authority_end + 1, 0, encoded_query);
                        } else {
                            replace_value(authority_end + 1, 0,
                                          "/" + encoded_query);
                        }
                    }
                }
            } else {
                // we have query
                replace_value(query_start, fragment_start - query_start,
                              encoded_query);
            }
            return *this;
        }

        /**
         *
         * @param queries
         * @return
         */
        template <typename Map>
        basic_uri& query(Map const& _queries) noexcept {
            static_assert(std::is_convertible_v<typename Map::key_type,
                                                std::string_view> &&
                              std::is_convertible_v<typename Map::mapped_type,
                                                    std::string_view>,
                          "The specified map is not valid");
            std::string _query_data;
            bool first = true;
            for (auto it = _queries.cbegin(); it != _queries.cend(); it++) {
                auto name = encode_uri_component(
                    it->first, QUERY_OR_FRAGMENT_NOT_PCT_ENCODED);
                auto value = encode_uri_component(
                    it->second, QUERY_OR_FRAGMENT_NOT_PCT_ENCODED);
                if (name.empty()) // when name is empty, we just don't care
                    continue;
                _query_data = name + (value.empty() ? "" : ("=" + value)) +
                              (std::next(it) != _queries.cend() ? "&" : "");
            }
            query(_query_data);
            return *this;
        }

        /**
         * @brief clear the query section of the URI
         * @return
         */
        basic_uri& clear_query() noexcept { return query({}); }

        /**
         * Get the query in a decoded string format
         * @return optional<string>
         */
        [[nodiscard]] auto query_decoded() const noexcept {
            return decode_uri_component(query(),
                                        QUERY_OR_FRAGMENT_NOT_PCT_ENCODED);
        }

        /**
         * Get the query in as a map<string, string>
         * It's also in a decoded format
         * @return unordered_map<string, string>
         */
        [[nodiscard]] std::map<std::string, std::string>
        query_structured() const noexcept {
            std::map<std::string, std::string> q_structured;
            std::size_t last_and_sep = 0;
            auto _query = query();
            do {
                auto and_sep =
                    _query.find('&', last_and_sep); // find the delimiter
                auto eq_sep =
                    _query.find("=", last_and_sep, and_sep - last_and_sep);
                auto name =
                    _query.substr(last_and_sep + 1, std::min(eq_sep, and_sep));
                last_and_sep = and_sep;
                if (name.empty()) // a name should not be empty
                    continue;
                std::string value;
                if (and_sep !=
                    std::string_view::npos) { // we have a value as well
                    value = _query.substr(eq_sep + 1, and_sep);
                }
                auto d_name = decode_uri_component(
                    name, QUERY_OR_FRAGMENT_NOT_PCT_ENCODED);
                if (d_name)
                    q_structured[d_name.value()] =
                        decode_uri_component(value,
                                             QUERY_OR_FRAGMENT_NOT_PCT_ENCODED)
                            .value_or("");
            } while (last_and_sep != std::string_view::npos);
            return q_structured;
        }

        /**
         * @brief checks if the uri path is normalized or not (contains relative
         * . or .. paths)
         * @return
         */
        [[nodiscard]] bool is_normalized() const noexcept {
            auto __path = path_structured();
            return __path.cend() != std::find_if(__path.cbegin(), __path.cend(),
                                                 [](auto const& p) {
                                                     return p == "." ||
                                                            p == "..";
                                                 });
        }

        /**
         * @details This method applies the "remove_dot_segments" routine talked
         * about in RFC 3986 (https://tools.ietf.org/html/rfc3986) to the path
         * segments of the URI, in order to normalize the path
         * (apply and remove "." and ".." segments).
         */
        basic_uri& normalize_path() noexcept {
            // TODO
            return *this;
        }

        /**
         * @brief get fragment
         */
        [[nodiscard]] std::string_view fragment() const noexcept {
            parse_fragment();
            return substr(fragment_start + 1);
        }

        /**
         * @brief an indication of whether the URI has fragment or not.
         * @return
         */
        [[nodiscard]] bool has_fragment() const noexcept {
            parse_fragment();
            return fragment_start != data.size();
        }

        /**
         * @brief clear the fragment part of the uri
         * @return
         */
        basic_uri& clear_fragment() noexcept { return fragment({}); }

        /**
         * @brief checks if the URI is a relative reference
         * @return
         */
        [[nodiscard]] bool is_relative_reference() const noexcept {
            return !has_scheme();
        }

        /**
         * This method returns an indication of whether or not the URI includes
         * any element that is part of the authority URI.
         * @return bool
         */
        [[nodiscard]] bool has_authority() const noexcept {
            return has_host() || has_user_info() || has_port();
        }

        /**
         * Get the string representation of the uri
         * @return string
         */
        [[nodiscard]] StringType const& str() const noexcept { return data; }

        /**
         * Get a string_view version of the uri
         * @return std::string_view
         */
        [[nodiscard]] std::string_view string_view() const noexcept {
            if constexpr (std::is_same_v<StringType, std::string_view>) {
                return data;
            } else {
                return std::string_view(data);
            }
        }

        /**
         * This method resolves the given relative reference, based on the given
         * base URI, returning the resolved target URI.
         *
         * @param[in] relative_uri
         *     This describes how to get to the target starting at the base.
         *
         * @return
         *     The resolved target URI is returned.
         *
         * @note
         *     It only makes sense to call this method on an absolute URI
         *     (in which I mean, the base URI should be absolute,
         *     as in IsRelativeReference() should return false).
         */
        [[nodiscard]] basic_uri resolve(const basic_uri& relative_uri) const
            noexcept {
            // Resolve the reference by following the algorithm
            // from section 5.2.2 in
            // RFC 3986 (https://tools.ietf.org/html/rfc3986).
            basic_uri target;
            if (relative_uri.has_scheme()) {
                target = relative_uri;
                target.normalize_path();
            } else {
                target.scheme(scheme());
                target.fragment(relative_uri.fragment());
                if (relative_uri.has_host()) {
                    target.host(relative_uri.host());
                    target.port(relative_uri.port());
                    target.user_info(relative_uri.user_info());
                    target.path(relative_uri.path());
                    target.query(relative_uri.query());
                    target.normalize_path();
                } else {
                    target.host(host());
                    target.user_info(user_info());
                    target.port(port());
                    if (!relative_uri.has_path()) {
                        target.path(path());
                        if (relative_uri.has_query()) {
                            target.query(relative_uri.query());
                        } else {
                            target.query(query());
                        }
                    } else {
                        target.query(relative_uri._query);
                        // RFC describes this as:
                        // "if (R.path starts-with "/") then"
                        if (relative_uri.is_absolute()) {
                            target.path(relative_uri._path);
                            target.normalize_path();
                        } else {
                            // RFC describes this as:
                            // "T.path = merge(Base.path, R.path);"
                            target.path(path());
                            auto target_path = target.path_structured();
                            auto relative_uri_path =
                                relative_uri.path_structured();
                            if (target_path.size() > 1) {
                                target_path.pop_back();
                            }
                            std::copy(relative_uri_path.cbegin(),
                                      relative_uri_path.cend(),
                                      std::back_inserter(target_path));
                            target.path(target_path);
                            target.normalize_path();
                        }
                    }
                }
            }

            return target;
        }

        /**
         * Check if the specified URI is in fact a URN
         *
         * URN syntax based on https://tools.ietf.org/html/rfc8141#section-2
         *
         *      namestring    = assigned-name
         *                      [ rq-components ]
         *                      [ "#" f-component ]
         *      assigned-name = "urn" ":" NID ":" NSS
         *      NID           = (alphanum) 0*30(ldh) (alphanum)
         *      ldh           = alphanum / "-"
         *      NSS           = pchar *(pchar / "/")
         *      rq-components = [ "?+" r-component ]
         *                      [ "?=" q-component ]
         *      r-component   = pchar *( pchar / "/" / "?" )
         *      q-component   = pchar *( pchar / "/" / "?" )
         *      f-component   = fragment
         *
         */
        [[nodiscard]] bool is_urn() const noexcept {
            return scheme() == "urn" && authority_start == data.size();
        }

        /**
         * Check if the specified URI is in fact a URL
         */
        [[nodiscard]] bool is_url() const noexcept { return has_host(); }

        /**
         * Check if the specified string is a valid URI or not
         */
        [[nodiscard]] bool is_valid() const noexcept {
            return has_scheme() || has_authority() || has_path() ||
                   has_fragment();
        }

        /**
         * Check if the user info has a username in it or not
         * @return bool
         */
        [[nodiscard]] bool has_username() const noexcept {
            return user_info_end != data.size();
        }

        /**
         * Get the username in the user info if it exists or otherwise an empty
         * string view
         * @return
         */
        [[nodiscard]] std::string_view username() const noexcept {
            auto _userinfo = user_info();
            if (auto colon = _userinfo.find(':');
                colon != std::string_view::npos)
                _userinfo.remove_suffix(_userinfo.size() - colon);
            return _userinfo;
        }

        /**
         * Get the decoded version of the username if it exists
         * @return
         */
        [[nodiscard]] std::string username_decoded() const noexcept {
            return decode_uri_component(username(), USER_INFO_NOT_PCT_ENCODED);
        }

        /**
         * An indication of whether or not the user info has a password
         * @return
         */
        [[nodiscard]] bool has_password() const noexcept {
            return user_info().find(':') != std::string_view::npos;
        }

        /**
         * The password in the user info
         * @return
         */
        [[nodiscard]] std::string_view password() const noexcept {
            auto _user_info = user_info();
            if (auto found = _user_info.find(':');
                found != std::string_view::npos) {
                return _user_info.substr(found + 1);
            }
            return {};
        }

        /**
         * The decoded version of the password
         * @return
         */
        [[nodiscard]] std::string password_decoded() const noexcept {
            return decode_uri_component(password(), USER_INFO_NOT_PCT_ENCODED);
        }
    };

    using const_uri = basic_uri<std::string_view>;
    using uri = basic_uri<std::string>;

    bool operator==(uri const& one, const_uri const& two) noexcept;

    bool operator==(const_uri const& one, uri const& two) noexcept;

} // namespace webpp

#endif // WEBPP_UTILS_URI_H