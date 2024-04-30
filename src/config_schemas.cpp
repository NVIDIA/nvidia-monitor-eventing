#include "eventing_main.hpp"

namespace eventing
{

std::shared_ptr<json_schema::JsonSchema> accessorCheckerSchema()
{
    using namespace json_schema;
    using namespace nlohmann;
    return schema(
        types(json::value_t::object),
        anyOf(
            schema(requiredProperties("not_equal"),
                   properties(
                       literal(false),
                       property("not_equal", types(json::value_t::string)))),

            schema(requiredProperties("equal"),
                   properties(literal(false),
                              property("equal", types(json::value_t::string)))),

            schema(
                requiredProperties("bitmap"),
                properties(literal(false),
                           property("bitmap", types(json::value_t::string)))),

            schema(
                requiredProperties("bitmask"),
                properties(literal(false),
                           property("bitmask", types(json::value_t::string))))

                ));
}

std::shared_ptr<json_schema::JsonSchema> dataAccessorSchema()
{
    // marcinw:TODO: checkers allowed only in "accessor" and "event_trigger"
    // accessors
    using namespace json_schema;
    using namespace nlohmann;
    auto checkerSchema = accessorCheckerSchema();
    auto dbusAccessorProperties =
        schema(requiredProperties("type", "object", "interface", "property"),
               properties(literal(false), property("type", values("DBUS")),
                          property("object", types(json::value_t::string)),
                          property("interface", types(json::value_t::string)),
                          property("property", types(json::value_t::string)),
                          property("name", types(json::value_t::string)),
                          property("check", checkerSchema)));
    auto coreApiAccessorProperties = schema(
        requiredProperties("type", "property"),
        properties(literal(false), property("type", values("DeviceCoreAPI")),
                   property("property", types(json::value_t::string)),
                   property("device_id", types(json::value_t::string)),
                   property("check", checkerSchema),
                   property("name", types(json::value_t::string))));
    auto cmdLineAccessorProperties =
        schema(requiredProperties("type", "executable", "arguments"),
               properties(literal(false), property("type", values("CMDLINE")),
                          property("executable", types(json::value_t::string)),
                          property("arguments", types(json::value_t::string)),
                          property("check", checkerSchema),
                          property("name", types(json::value_t::string))));
    auto directAccessorProperties =
        schema(requiredProperties("type", "field"),
               properties(literal(false), property("type", values("DIRECT")),
                          property("field", values("CurrentDeviceName")),
                          property("check", checkerSchema),
                          property("name", types(json::value_t::string))));
    return schema(types(nlohmann::json::value_t::object),
                  anyOf(dbusAccessorProperties, coreApiAccessorProperties,
                        cmdLineAccessorProperties, directAccessorProperties));
}

std::shared_ptr<json_schema::JsonSchema> eventNodeRedfishJsonSchema()
{
    using namespace json_schema;
    using namespace nlohmann;
    auto messageArgsSchema = schema(
        types(nlohmann::json::value_t::object),
        requiredProperties("patterns", "parameters"),
        properties(
            literal(false),
            property("patterns",
                     schema(types(nlohmann::json::value_t::array),
                            items(types(nlohmann::json::value_t::string)))),
            property("parameters", schema(types(nlohmann::json::value_t::array),
                                          items(dataAccessorSchema())))));
    return schema(types(json::value_t::object),
                  requiredProperties("message_id", "message_args"),
                  properties(literal(false),
                             property("message_id",
                                      types(nlohmann::json::value_t::string)),
                             property("message_args", messageArgsSchema),
                             property("origin_of_condition",
                                      schema(types(json::value_t::string)))));
}

std::shared_ptr<json_schema::JsonSchema> eventNodeJsonSchema()
{
    using namespace json_schema;
    using namespace nlohmann;
    auto accessorSchema = dataAccessorSchema();
    auto emptyObjectSchema =
        schema(types(json::value_t::object), properties(literal(false)));
    return schema(
        types(json::value_t::object),
        requiredProperties("action", "value_as_count", "redfish",
                           "event_counter_reset", "trigger_count", "resolution",
                           "severity", "accessor", "event_trigger",
                           "device_type", "event", "error_id"),
        properties(
            literal(false), // Requirement for all the properties not listed
                            // here (cannot be met, which means only the listed
                            // are allowed)
            property("event", types(json::value_t::string)),
            property("error_id", types(json::value_t::string)),
            property("device_type", types(json::value_t::string)),
            property(
                "category",
                schema(types(json::value_t::array),
                       items(schema(
                           types(json::value_t::string),
                           values("association", "power_rail", "erot_control",
                                  "pin_status", "interface_status",
                                  "protocol_status", "firmware_status"))))),
            property("sub_type", types(json::value_t::string)),
            property("event_trigger", anyOf(accessorSchema, emptyObjectSchema)),
            property("accessor", accessorSchema),
            property("recovery", accessorSchema),
            property("severity", schema(types(json::value_t::string),
                                        values("OK", "Critical", "Warning"))),
            property("resolution", types(json::value_t::string)),
            property("trigger_count",
                     schema(types(json::value_t::number_unsigned,
                                  json::value_t::number_integer),
                            minimum(-1))),
            property("event_counter_reset", types(json::value_t::object)),
            property("redfish", eventNodeRedfishJsonSchema()),
            property("telemetries", schema(types(json::value_t::array),
                                           items(accessorSchema))),
            property("action", types(json::value_t::string)),
            property("value_as_count", types(json::value_t::boolean))));
}

std::shared_ptr<json_schema::JsonSchema> eventInfoJsonSchema()
{
    using namespace json_schema;
    auto nodeSchema = eventNodeJsonSchema();
    return schema(types(nlohmann::json::value_t::object),
                  properties(schema(types(nlohmann::json::value_t::array),
                                    items(nodeSchema))));
}

std::shared_ptr<json_schema::JsonSchema> datSchema()
{
    using namespace json_schema;
    // marcinw:TODO: define shema for 'dat.json'
    return literal(true);
}

} // namespace eventing
