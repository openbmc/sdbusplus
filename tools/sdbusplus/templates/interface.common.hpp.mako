#pragma once
#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>
<%
    def setOfPropertyTypes():
        return set(p.cppTypeParam(interface.name) for p in
                   interface.properties);
%>\

namespace sdbusplus::common::${interface.cppNamespace()}
{

struct ${interface.classname}
{
    static constexpr auto interface = "${interface.name}";

    % for e in interface.enums:
    enum class ${e.name}
    {
        % for v in e.values:
        ${v.name},
        % endfor
    };
    % endfor

    % if interface.properties:
    struct properties_t
    {
        % for p in interface.properties:
        ${p.cppTypeParam(interface.name)} ${p.snake_case}${p.default_value(interface.name)};
        % endfor
    };

    struct property_names
    {
        % for p in interface.properties:
        static constexpr auto ${p.snake_case} = "${p.name}";
        % endfor
    };

    using PropertiesVariant = sdbusplus::utility::dedup_variant_t<
        ${",\n        ".join(sorted(setOfPropertyTypes()))}>;
    % else:
    using properties_t = std::nullopt_t;
    % endif

    % if interface.methods:
    struct method_names
    {
        % for method in interface.methods:
        static constexpr auto ${method.snake_case} = "${method.name}";
        % endfor
    };
    % endif

    % if interface.signals:
    struct signal_names
    {
        % for signal in interface.signals:
        static constexpr auto ${signal.snake_case} = "${signal.name}";
        % endfor
    };
    % endif

    % if interface.enums:
    struct enum_strings
    {
        % for e in interface.enums:
        struct ${e.name} {
            % for v in e.values:
                static constexpr auto ${v.snake_case} = "${interface.name}.${e.name}.${v.name}";
            % endfor
        };
        % endfor
    };
    % endif

    % for p in interface.paths:
        % if p.description:
    /** ${p.description.strip()} */
        % endif
        % if len(p.segments) == 0:
    static constexpr auto ${p.snake_case} = "${p.value}";
        % else:
    struct ${p.snake_case}
    {
        static constexpr auto value = "${p.value}";
            % for s in p.segments:
                % if s.description:
        /** ${s.description.strip()} */
                % endif
        static constexpr auto ${s.snake_case} = "${s.value}";
            % endfor
    };
        % endif
    % endfor
    % for s in interface.service_names:
        % if s.description:
    /** ${s.description.strip()} */
        % endif
    static constexpr auto ${s.snake_case} = "${s.value}";
    % endfor

    % for e in interface.enums:
    /** @brief Convert a string to an appropriate enum value.
     *  @param[in] s - The string to convert in the form of
     *                 "${interface.name}.<value name>"
     *  @return - The enum value.
     *
     *  @note Throws if string is not a valid mapping.
     */
    static ${e.name} convert${e.name}FromString(const std::string& s);

    /** @brief Convert a string to an appropriate enum value.
     *  @param[in] s - The string to convert in the form of
     *                 "${interface.name}.<value name>"
     *  @return - The enum value or std::nullopt
     */
    static std::optional<${e.name}>
        convertStringTo${e.name}(const std::string& s) noexcept;

    /** @brief Convert an enum value to a string.
     *  @param[in] e - The enum to convert to a string.
     *  @return - The string conversion in the form of
     *            "${interface.name}.<value name>"
     */
    static std::string convert${e.name}ToString(${e.name} e);
    % endfor
};

% for e in interface.enums:
/* Specialization of sdbusplus::common::convertForMessage
 * for enum-type ${interface.classname}::${e.name}.
 *
 * This converts from the enum to a constant string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return string representing the name for the enum value.
 */
inline std::string convertForMessage(${interface.classname}::${e.name} e)
{
    return ${interface.classname}::convert${e.name}ToString(e);
}
% endfor

    % for e in interface.enums:

namespace details
{
using namespace std::literals::string_view_literals;

/** String to enum mapping for ${interface.classname}::${e.name} */
inline constexpr std::array mapping${interface.classname}${e.name} = {
    % for v in e.values:
    std::make_tuple(${interface.classname}::enum_strings::${e.name}::${v.snake_case},
                    ${interface.classname}::${e.name}::${v.name} ),
    % endfor
};
} //  namespace details

inline auto ${interface.classname}::convertStringTo${e.name}(const std::string& s) noexcept
    -> std::optional<${e.name}>
{
    auto i = std::find_if(std::begin(details::mapping${interface.classname}${e.name}),
                          std::end(details::mapping${interface.classname}${e.name}),
                          [&s](auto& e){ return s == std::get<0>(e); } );

    if (std::end(details::mapping${interface.classname}${e.name}) == i)
    {
        return std::nullopt;
    }
    else
    {
        return std::get<1>(*i);
    }
}

inline auto ${interface.classname}::convert${e.name}FromString(const std::string& s) -> ${e.name}
{
    auto r = convertStringTo${e.name}(s);

    if (!r)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return *r;
    }
}

inline std::string ${interface.classname}::convert${e.name}ToString(
    ${interface.classname}::${e.name} v)
{
    auto i = std::find_if(std::begin(details::mapping${interface.classname}${e.name}),
                          std::end(details::mapping${interface.classname}${e.name}),
                          [v](auto& e){ return v == std::get<1>(e); });

    if (i == std::end(details::mapping${interface.classname}${e.name}))
    {
        throw std::invalid_argument(std::to_string(static_cast<int>(v)));
    }
    return std::string(std::get<0>(*i));
}
    % endfor

} // sdbusplus::common::${interface.cppNamespace()}

namespace sdbusplus::message::details
{
    % for e in interface.enums:
template <>
struct convert_from_string<common::${interface.cppNamespacedClass()}::${e.name}>
{
    static auto op(const std::string& value) noexcept
    {
        return common::${interface.cppNamespacedClass()}::
            convertStringTo${e.name}(value);
    }
};

template <>
struct convert_to_string<common::${interface.cppNamespacedClass()}::${e.name}>
{
    static std::string
        op(common::${interface.cppNamespacedClass()}::${e.name} value)
    {
        return common::${interface.cppNamespacedClass()}::
            convert${e.name}ToString(value);
    }
};
    % endfor
} // namespace sdbusplus::message::details
