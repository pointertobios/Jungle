#include "jungle/meta.h"
#include "jungle/serde/text.h"
#include "jungle/test/test.h"

#include <string_view>
#include <vector>

namespace {

using jungle::serde::serialize;
using jungle::serde::TextTarget;
using namespace std::string_view_literals;

enum class serde_text_test_color { red, green, blue };

struct serde_text_test_inner {
    int value;
    bool enabled;
};

struct serde_text_test_outer {
    int number;
    double ratio;
    bool flag;
    serde_text_test_color color;
    serde_text_test_inner inner;
    std::vector<int> items;
};

struct serde_text_test_with_private {
    int public_value;

    constexpr serde_text_test_with_private(int public_value, int private_value)
            : public_value(public_value)
            , private_value(private_value) {}

private:
    int private_value;
};

struct[[= jungle::serde::customize]] serde_text_test_marked_fields {
    [[= jungle::serde::field]] int kept = 7;
    int dropped = 8;
    [[= jungle::serde::field]] bool enabled = true;
};

template<typename T>
struct serde_text_test_plus_thousand {
    template<jungle::serde::SerializeTargetImpl Target>
        requires std::integral<T>
    void serialize(T value, Target &target) const {
        target.serialize_integral(value + 1000);
    }
};

static_assert(jungle::serde::Customizer<serde_text_test_plus_thousand>);

struct serde_text_test_field_customized {
    [[= jungle::serde::customized<serde_text_test_plus_thousand>]] int value = 42;
};

template<typename T>
jungle::ustr serialize_to_text(const T &value) {
    return serialize<TextTarget>(value);
}

static_assert(jungle::meta::has_annotation(^^serde_text_test_marked_fields, jungle::serde::customize));
static_assert(jungle::meta::has_annotation(^^serde_text_test_marked_fields::kept, jungle::serde::field));
static_assert(!jungle::meta::has_annotation(^^serde_text_test_marked_fields::dropped, jungle::serde::field));
static_assert(std::meta::annotations_of(^^serde_text_test_marked_fields).size() == 1);
static_assert(std::meta::annotations_of(^^serde_text_test_marked_fields::kept).size() == 1);
static_assert(std::meta::is_annotation(std::meta::annotations_of(^^serde_text_test_marked_fields)[0]));
static_assert(
    std::meta::constant_of(std::meta::annotations_of(^^serde_text_test_marked_fields)[0])
    == std::meta::reflect_constant(jungle::serde::customize));
static_assert(
    std::meta::constant_of(std::meta::annotations_of(^^serde_text_test_marked_fields::kept)[0])
    == std::meta::reflect_constant(jungle::serde::field));
static_assert(jungle::meta::has_template_annotation<^^serde_text_test_field_customized::value>(
    ^^jungle::serde::customized));

JUNGLE_SYNC_TEST(text_target_serializes_direct_supported_categories) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(true).view() == "true"sv, "bool values should use literal true/false");
    JUNGLE_SYNC_ASSERT(serialize_to_text(42).view() == "42"sv, "integral values should format as decimal");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(2.5).view() == "2.5"sv,
        "floating-point values should be forwarded to the target formatter");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(serde_text_test_color::green).view() == "serde_text_test_color::green"sv,
        "enum values should use their reflected enumerator name");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::vector<int>{1, 2, 3}).view() == "int[1,2,3,]"sv,
        "ranges should include the reflected element type and each serialized element");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_nested_objects) {
    const auto serialized = serialize_to_text(
        serde_text_test_outer{
            .number = 7,
            .ratio = 2.5,
            .flag = true,
            .color = serde_text_test_color::blue,
            .inner = serde_text_test_inner{.value = 11, .enabled = false},
            .items = {1, 2, 3}});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "serde_text_test_outer{number:7,ratio:2.5,flag:true,color:serde_text_test_color::blue,inner:serde_text_test_inner{value:11,enabled:false,},items:int[1,2,3,],}"sv,
        "objects should serialize every reflected non-static data member in declaration order");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_private_members_via_unchecked_reflection) {
    const auto serialized = serialize_to_text(serde_text_test_with_private{4, 9});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_with_private{public_value:4,private_value:9,}"sv,
        "SerializeTarget should observe private members because serde walks members with unchecked access");
    return {};
}

JUNGLE_SYNC_TEST(text_target_uses_placeholder_name_for_unnamed_types) {
    const auto serialized = [] {
        struct {
            int count;
            bool ready;
        } value{.count = 3, .ready = true};
        return serialize_to_text(value);
    }();

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "<unnamed>{count:3,ready:true,}"sv,
        "types without an identifier should serialize with the unnamed placeholder");
    return {};
}

JUNGLE_SYNC_TEST(text_target_class_level_customize_only_keeps_marked_fields) {
    const auto serialized = serialize_to_text(serde_text_test_marked_fields{});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_marked_fields{kept:7,enabled:true,}"sv,
        "[[=serde::customize]] should limit serialization to members explicitly marked with [[=serde::field]]");
    return {};
}

JUNGLE_SYNC_TEST(text_target_field_customizer_controls_field_output) {
    const auto serialized = serialize_to_text(serde_text_test_field_customized{});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_field_customized{value:1042,}"sv,
        "[[=serde::customized<...>]] should delegate field serialization to the customizer");
    return {};
}

}  // namespace