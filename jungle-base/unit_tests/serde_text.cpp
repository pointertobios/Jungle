#include "jungle/meta.h"
#include "jungle/serde/text.h"
#include "jungle/test/test.h"

#include <array>
#include <optional>
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

struct[[= jungle::serde::customized]] serde_text_test_marked_fields {
    [[= jungle::serde::field]] int kept = 7;
    int dropped = 8;
    [[= jungle::serde::field]] bool enabled = true;
};

template<typename T>
struct serde_text_test_plus_thousand {
    void serialize(const T &value, auto &target) const { target.serialize_integral(value + 1000); }
};

static_assert(jungle::serde::Customizer<serde_text_test_plus_thousand>);

struct serde_text_test_field_customized {
    [[= jungle::serde::customize<serde_text_test_plus_thousand>]] int value = 42;
};

template<typename T>
jungle::ustr serialize_to_text(const T &value) {
    return serialize<TextTarget>(value);
}

static_assert(jungle::meta::has_annotation(^^serde_text_test_marked_fields, jungle::serde::customized));
static_assert(jungle::meta::has_annotation(^^serde_text_test_marked_fields::kept, jungle::serde::field));
static_assert(!jungle::meta::has_annotation(^^serde_text_test_marked_fields::dropped, jungle::serde::field));
static_assert(std::meta::annotations_of(^^serde_text_test_marked_fields).size() == 1);
static_assert(std::meta::annotations_of(^^serde_text_test_marked_fields::kept).size() == 1);
static_assert(std::meta::is_annotation(std::meta::annotations_of(^^serde_text_test_marked_fields)[0]));
static_assert(
    std::meta::constant_of(std::meta::annotations_of (^^serde_text_test_marked_fields)[0])
    == std::meta::reflect_constant(jungle::serde::customized));
static_assert(
    std::meta::constant_of(std::meta::annotations_of(^^serde_text_test_marked_fields::kept)[0])
    == std::meta::reflect_constant(jungle::serde::field));
static_assert(jungle::meta::has_template_annotation<
              ^^serde_text_test_field_customized::value, ^^jungle::serde::customize>());

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
        serialize_to_text(std::vector<int>{1, 2, 3}).view() == "[1,2,3,]"sv,
        "ranges should include each serialized element");
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
            == "serde_text_test_outer{number:7,ratio:2.5,flag:true,color:serde_text_test_color::blue,inner:serde_text_test_inner{value:11,enabled:false,},items:[1,2,3,],}"sv,
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

struct serde_empty_struct {};

JUNGLE_SYNC_TEST(text_target_serializes_empty_struct) {
    const auto serialized = serialize_to_text(serde_empty_struct{});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_empty_struct{}"sv,
        "empty structs should produce type name followed by empty braces");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_empty_range) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::vector<int>{}).view() == "[]"sv,
        "empty vector should produce empty brackets with no trailing comma");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::vector<double>{}).view() == "[]"sv,
        "empty vector of floating-point should also produce empty brackets");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_integer_edge_cases) {
    JUNGLE_SYNC_ASSERT(serialize_to_text(0).view() == "0"sv, "zero should serialize as '0'");
    JUNGLE_SYNC_ASSERT(serialize_to_text(-1).view() == "-1"sv, "negative one should serialize as '-1'");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(-42).view() == "-42"sv, "negative numbers should include minus sign");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(9223372036854775807LL).view() == "9223372036854775807"sv,
        "large positive int64 should serialize correctly");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_various_integer_types) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(uint8_t{255}).view() == "255"sv, "uint8_t max should serialize correctly");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(uint16_t{65535}).view() == "65535"sv, "uint16_t should serialize correctly");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(uint32_t{42}).view() == "42"sv, "uint32_t should serialize correctly");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(uint64_t{0}).view() == "0"sv, "uint64_t zero should serialize correctly");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(int8_t{-128}).view() == "-128"sv, "int8_t min should serialize correctly");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_floating_point_edge_cases) {
    JUNGLE_SYNC_ASSERT(serialize_to_text(0.0).view() == "0"sv, "float zero should serialize as '0'");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(-3.14).view() == "-3.14"sv, "negative float should include minus sign");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(0.5).view() == "0.5"sv, "float between 0 and 1 should serialize correctly");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_nested_ranges) {
    const auto serialized = serialize_to_text(std::vector<std::vector<int>>{{1, 2}, {3, 4, 5}, {}});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "[[1,2,],[3,4,5,],[],]"sv,
        "nested vectors should recursively serialize, including empty inner vectors");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_std_array) {
    const auto serialized = serialize_to_text(std::array<int, 4>{10, 20, 30, 40});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "[10,20,30,40,]"sv,
        "std::array should serialize identically to vector of same elements");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_optional_with_value) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::optional<int>{42}).view() == "optional##42"sv,
        "optional with value should emit nonnull prefix then the value");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::optional<bool>{false}).view() == "optional##false"sv,
        "optional bool with value should serialize correctly");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_optional_nullopt) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::optional<int>{}).view() == "optional##nullopt"sv,
        "empty optional should emit nullopt sentinel with no value");
    JUNGLE_SYNC_ASSERT(
        serialize_to_text(std::optional<double>{}).view() == "optional##nullopt"sv,
        "empty optional of any type should emit the same nullopt sentinel");
    return {};
}

struct serde_optional_struct {
    int id;
    std::optional<int> maybe_score;
};

JUNGLE_SYNC_TEST(text_target_serializes_optional_inside_object) {
    const auto with_value = serialize_to_text(serde_optional_struct{.id = 1, .maybe_score = 100});
    JUNGLE_SYNC_ASSERT(
        with_value.view() == "serde_optional_struct{id:1,maybe_score:optional##100,}"sv,
        "optional field with value should nest inside class serialization");

    const auto without_value = serialize_to_text(serde_optional_struct{.id = 2, .maybe_score = {}});
    JUNGLE_SYNC_ASSERT(
        without_value.view() == "serde_optional_struct{id:2,maybe_score:optional##nullopt,}"sv,
        "nullopt field should also nest correctly inside class serialization");
    return {};
}

JUNGLE_SYNC_TEST(text_target_serializes_optional_of_range) {
    const auto with_vec = serialize_to_text(std::optional<std::vector<int>>{std::vector{7, 8, 9}});
    JUNGLE_SYNC_ASSERT(
        with_vec.view() == "optional##[7,8,9,]"sv,
        "optional<vector> with value should serialize vector inside optional wrapper");

    const auto null_vec = serialize_to_text(std::optional<std::vector<int>>{});
    JUNGLE_SYNC_ASSERT(
        null_vec.view() == "optional##nullopt"sv, "empty optional<vector> should emit nullopt");
    return {};
}

struct[[= jungle::serde::customized]] serde_text_test_mixed_annotations {
    [[= jungle::serde::field]] int normal = 1;
    [[= jungle::serde::field]][[= jungle::serde::customize<serde_text_test_plus_thousand>]] int boosted =
        100;
    int skipped = 999;
    [[= jungle::serde::field]] bool active = true;
};

JUNGLE_SYNC_TEST(text_target_handles_mixed_annotations) {
    const auto serialized = serialize_to_text(serde_text_test_mixed_annotations{});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_mixed_annotations{normal:1,boosted:1100,active:true,}"sv,
        "mix of [[=field]] and [[=customized<...>]] should coexist, skipped fields should be absent");
    return {};
}

struct[[= jungle::serde::customized]] serde_text_test_reordered_fields {
    int first = 1;
    [[= jungle::serde::field]] int middle = 2;
    [[= jungle::serde::field]] int last = 3;
};

JUNGLE_SYNC_TEST(text_target_preserves_declaration_order_for_marked_fields) {
    const auto serialized = serialize_to_text(serde_text_test_reordered_fields{});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_reordered_fields{middle:2,last:3,}"sv,
        "marked fields should appear in declaration order, skipping unmarked fields");
    return {};
}

struct[[= jungle::serde::customized]] serde_text_test_all_marked {
    [[= jungle::serde::field]] int a = 10;
    [[= jungle::serde::field]] bool b = false;
    [[= jungle::serde::field]] double c = 3.0;
};

JUNGLE_SYNC_TEST(text_target_all_fields_marked_behaves_like_default) {
    const auto customized = serialize_to_text(serde_text_test_all_marked{});

    JUNGLE_SYNC_ASSERT(
        customized.view() == "serde_text_test_all_marked{a:10,b:false,c:3,}"sv,
        "when all members are marked [[=field]], output should match the default behavior");
    return {};
}

struct serde_text_test_all_private {
    constexpr serde_text_test_all_private(int x, int y)
            : m_x(x)
            , m_y(y) {}

    int sum() const { return m_x + m_y; }

private:
    int m_x;
    int m_y;
};

JUNGLE_SYNC_TEST(text_target_serializes_all_private_members) {
    const auto serialized = serialize_to_text(serde_text_test_all_private{3, 7});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_all_private{m_x:3,m_y:7,}"sv,
        "unchecked access context should reach private members even when no public members exist");
    return {};
}

struct serde_text_test_with_range_member {
    int tag;
    std::vector<int> items;
};

JUNGLE_SYNC_TEST(text_target_serializes_class_with_range_member) {
    const auto serialized =
        serialize_to_text(serde_text_test_with_range_member{.tag = 5, .items = {10, 20, 30}});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "serde_text_test_with_range_member{tag:5,items:[10,20,30,],}"sv,
        "range members inside a class should serialize recursively");
    return {};
}

}  // namespace
