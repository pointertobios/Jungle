// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/core/asset/serde/serde_jaml.h"
#include "jungle/build_id.h"
#include "jungle/test/test.h"

#include <expected>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using jungle::ustr;
using jungle::core::asset::JamlSource;
using jungle::core::asset::JamlTarget;
using jungle::serde::serialize;
using namespace std::string_view_literals;

enum class serde_jaml_test_color { red, green, blue };

struct serde_jaml_test_inner {
    int value;
    bool enabled;
};

struct serde_jaml_test_outer {
    int number;
    double ratio;
    bool flag;
    serde_jaml_test_color color;
    serde_jaml_test_inner inner;
    std::vector<int> items;
};

struct serde_jaml_test_with_private {
    int public_value;

    constexpr serde_jaml_test_with_private(int public_value, int private_value)
            : public_value{public_value}
            , private_value{private_value} {}

private:
    int private_value;
};

struct[[= jungle::serde::customized]] serde_jaml_test_marked_fields {
    [[= jungle::serde::field]] int kept = 7;
    int dropped = 8;
    [[= jungle::serde::field]] bool enabled = true;
};

template<typename T>
struct serde_jaml_test_plus_thousand {
    void serialize(const T &value, auto &target) const { target.serialize_integral(value + 1000); }
    template<typename U>
    auto deserialize(U &value, auto &source) const
        -> std::expected<void, typename std::remove_cvref_t<decltype(source)>::error_type> {
        if (auto r = source.template deserialize_integral<U>(value); !r) {
            return r;
        }
        value = value - 1000;
        return {};
    }
};

static_assert(jungle::serde::Customizer<serde_jaml_test_plus_thousand>);

struct serde_jaml_test_field_customized {
    [[= jungle::serde::customize<serde_jaml_test_plus_thousand>]] int value = 42;
};

struct serde_empty_struct {};

struct serde_optional_struct {
    int id;
    std::optional<int> maybe_score;
};

struct[[= jungle::serde::customized]] serde_jaml_test_mixed_annotations {
    [[= jungle::serde::field]] int normal = 1;
    [[= jungle::serde::field]][[= jungle::serde::customize<serde_jaml_test_plus_thousand>]] int boosted = 100;
    int skipped = 999;
    [[= jungle::serde::field]] bool active = true;
};

struct[[= jungle::serde::customized]] serde_jaml_test_reordered_fields {
    int first = 1;
    [[= jungle::serde::field]] int middle = 2;
    [[= jungle::serde::field]] int last = 3;
};

struct serde_jaml_test_with_range_member {
    int tag;
    std::vector<int> items;
};

template<typename T>
jungle::ustr serialize_to_jaml(const T &value) {
    auto doc = serialize<JamlTarget>(value);
    // XML 声明头由专门的用例校验；这里返回去除头后的 jaml 正文，便于按正文逐段断言。
    auto text = doc.view();
    if (text.starts_with("<?xml")) {
        if (const auto pi_end = text.find("?>"); pi_end != std::string_view::npos) {
            text = text.substr(pi_end + 2);
            if (!text.empty() && text.front() == '\n') {
                text.remove_prefix(1);
            }
        }
    }
    return ustr{text};
}

// 当前 build_id() 的 32 位小写十六进制。
ustr current_build_id_hex() { return ustr::format("{:032x}", jungle::build_id()); }

// 构造 XML 声明头：jaml_version="<build_id_str><hex>"。
ustr jaml_header_with(std::string_view build_id_str, std::string_view hex) {
    ustr head{"<?xml version=\"1.0\" encoding=\"UTF-8\" jaml_version=\""};
    head.append(build_id_str);
    head.append("<");
    head.append(hex);
    head.append(">\"?>\n");
    return head;
}

// 使用当前构建信息构造的头部。
ustr current_jaml_header() {
    const auto hex = current_build_id_hex();
    return jaml_header_with(jungle::build_id_string().view(), hex.view());
}

// ---- 反序列化测试包装 ----
// JamlSource 要求载荷必须携带有效的 XML 声明头。
// 为让各正文用例保持简洁，这里以同名包装屏蔽 jungle::serde::deserialize：
// 凡从载荷反序列化且载荷未以// "<?xml" 开头，就自动补上当前构建的 jaml_version 头；已带头的载荷则原样转发。
// 注意：本包装仅覆盖“从载荷”反序列化的两个重载；直接操作 source 的调用请显式使用
// jungle::serde::deserialize（否则会与 ADL 找到的真实重载二义）。
// “缺头/无效头一律失败”由专门用例（直接调用jungle::serde::deserialize）覆盖。

inline ustr with_valid_header(const ustr &payload) {
    if (payload.view().starts_with("<?xml")) {
        return payload;
    }
    auto doc = current_jaml_header();
    doc.append(payload);
    return doc;
}

template<typename T, typename Source>
    requires jungle::serde::DeserializeSourceImpl<Source> && std::is_default_constructible_v<T>
auto deserialize(const typename Source::source_type &payload)
    -> std::expected<T, typename Source::error_type> {
    return jungle::serde::deserialize<T, Source>(with_valid_header(payload));
}

template<typename T, typename Source>
    requires jungle::serde::DeserializeSourceImpl<Source>
auto deserialize(const typename Source::source_type &payload, T &value)
    -> std::expected<void, typename Source::error_type> {
    return jungle::serde::deserialize<T, Source>(with_valid_header(payload), value);
}

JUNGLE_SYNC_TEST(serializes_direct_supported_categories) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(true).view() == "<jaml>\n  true\n</jaml>"sv,
        "bool true 应序列化为缩进后的 jaml 文本");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(false).view() == "<jaml>\n  false\n</jaml>"sv,
        "bool false 应序列化为缩进后的 jaml 文本");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(42).view() == "<jaml>\n  42\n</jaml>"sv, "整数应序列化为缩进后的十进制文本");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(2.5).view() == "<jaml>\n  2.5\n</jaml>"sv, "浮点数应序列化为缩进后的十进制文本");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(serde_jaml_test_color::green).view()
            == "<jaml>\n  serde_jaml_test_color::green\n</jaml>"sv,
        "枚举应序列化为反射得到的枚举项名");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(std::vector<int>{1, 2, 3}).view()
            == "<jaml>\n  <list>\n    <item>1</item>\n    <item>2</item>\n    <item>3</item>\n  </list>\n</jaml>"sv,
        "range 应序列化为带缩进的 list/item 元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_nested_objects) {
    const auto serialized = serialize_to_jaml(
        serde_jaml_test_outer{
            .number = 7,
            .ratio = 2.5,
            .flag = true,
            .color = serde_jaml_test_color::blue,
            .inner = serde_jaml_test_inner{.value = 11, .enabled = false},
            .items = {1, 2, 3}});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_outer>\n    <number>7</number>\n    <ratio>2.5</ratio>\n    <flag>true</flag>\n    <color>serde_jaml_test_color::blue</color>\n    <inner>\n      <serde_jaml_test_inner>\n        <value>11</value>\n        <enabled>false</enabled>\n      </serde_jaml_test_inner>\n    </inner>\n    <items>\n      <list>\n        <item>1</item>\n        <item>2</item>\n        <item>3</item>\n      </list>\n    </items>\n  </serde_jaml_test_outer>\n</jaml>"sv,
        "对象应按声明顺序把每个非静态数据成员写成带缩进的同名 XML 元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_private_members_via_unchecked_reflection) {
    const auto serialized = serialize_to_jaml(serde_jaml_test_with_private{4, 9});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_with_private>\n    <public_value>4</public_value>\n    <private_value>9</private_value>\n  </serde_jaml_test_with_private>\n</jaml>"sv,
        "私有成员应通过 unchecked 反射一并序列化");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(uses_placeholder_name_for_unnamed_types) {
    const auto serialized = [] {
        struct {
            int count;
            bool ready;
        } value{.count = 3, .ready = true};
        return serialize_to_jaml(value);
    }();

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <_unnamed>\n    <count>3</count>\n    <ready>true</ready>\n  </_unnamed>\n</jaml>"sv,
        "无标识符类型应使用合法 XML 名 _unnamed");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(class_level_customize_only_keeps_marked_fields) {
    const auto serialized = serialize_to_jaml(serde_jaml_test_marked_fields{});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_marked_fields>\n    <kept>7</kept>\n    <enabled>true</enabled>\n  </serde_jaml_test_marked_fields>\n</jaml>"sv,
        "[[=customized]] 应只序列化带 [[=field]] 的成员");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(field_customizer_controls_field_output) {
    const auto serialized = serialize_to_jaml(serde_jaml_test_field_customized{});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_field_customized>\n    <value>1042</value>\n  </serde_jaml_test_field_customized>\n</jaml>"sv,
        "字段定制器应接管该字段的写出");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_empty_struct) {
    const auto serialized = serialize_to_jaml(serde_empty_struct{});

    JUNGLE_SYNC_ASSERT(
        serialized.view() == "<jaml>\n  <serde_empty_struct>\n  </serde_empty_struct>\n</jaml>"sv,
        "空结构体应写出类型名对应的空元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_empty_range) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(std::vector<int>{}).view() == "<jaml>\n  <list>\n  </list>\n</jaml>"sv,
        "空 vector 应写出空 list 元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_integer_edge_cases) {
    JUNGLE_SYNC_ASSERT(serialize_to_jaml(0).view() == "<jaml>\n  0\n</jaml>"sv, "零应序列化为 0");
    JUNGLE_SYNC_ASSERT(serialize_to_jaml(-42).view() == "<jaml>\n  -42\n</jaml>"sv, "负数应带负号");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(9223372036854775807LL).view() == "<jaml>\n  9223372036854775807\n</jaml>"sv,
        "大整数应完整写出");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_nested_ranges) {
    const auto serialized = serialize_to_jaml(std::vector<std::vector<int>>{{1, 2}, {3, 4, 5}, {}});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <list>\n    <item>\n      <list>\n        <item>1</item>\n        <item>2</item>\n      </list>\n    </item>\n    <item>\n      <list>\n        <item>3</item>\n        <item>4</item>\n        <item>5</item>\n      </list>\n    </item>\n    <item>\n      <list>\n      </list>\n    </item>\n  </list>\n</jaml>"sv,
        "嵌套 range 应递归写成带缩进的 list/item");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_optional_with_value) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(std::optional<int>{42}).view() == "<jaml>\n  42\n</jaml>"sv,
        "有值 optional 应直接写出内部值");
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(std::optional<bool>{false}).view() == "<jaml>\n  false\n</jaml>"sv,
        "有值 optional<bool> 应写出内部 bool");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_optional_nullopt) {
    JUNGLE_SYNC_ASSERT(
        serialize_to_jaml(std::optional<int>{}).view() == "<jaml>\n  <null/>\n</jaml>"sv,
        "空 optional 应写出自闭合 null 元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_optional_inside_object) {
    const auto with_value = serialize_to_jaml(serde_optional_struct{.id = 1, .maybe_score = 100});
    JUNGLE_SYNC_ASSERT(
        with_value.view()
            == "<jaml>\n  <serde_optional_struct>\n    <id>1</id>\n    <maybe_score>100</maybe_score>\n  </serde_optional_struct>\n</jaml>"sv,
        "对象中的有值 optional 字段应写出内部值");

    const auto without_value = serialize_to_jaml(serde_optional_struct{.id = 2, .maybe_score = {}});
    JUNGLE_SYNC_ASSERT(
        without_value.view()
            == "<jaml>\n  <serde_optional_struct>\n    <id>2</id>\n    <maybe_score><null/></maybe_score>\n  </serde_optional_struct>\n</jaml>"sv,
        "对象中的空 optional 字段应写出 null 元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_optional_of_range) {
    const auto with_vec = serialize_to_jaml(std::optional<std::vector<int>>{std::vector{7, 8, 9}});
    JUNGLE_SYNC_ASSERT(
        with_vec.view()
            == "<jaml>\n  <list>\n    <item>7</item>\n    <item>8</item>\n    <item>9</item>\n  </list>\n</jaml>"sv,
        "有值 optional<vector> 应写出内部 list");

    const auto null_vec = serialize_to_jaml(std::optional<std::vector<int>>{});
    JUNGLE_SYNC_ASSERT(
        null_vec.view() == "<jaml>\n  <null/>\n</jaml>"sv, "空 optional<vector> 应写出 null 元素");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(handles_mixed_annotations) {
    const auto serialized = serialize_to_jaml(serde_jaml_test_mixed_annotations{});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_mixed_annotations>\n    <normal>1</normal>\n    <boosted>1100</boosted>\n    <active>true</active>\n  </serde_jaml_test_mixed_annotations>\n</jaml>"sv,
        "[[=field]] 与字段定制器应共存，未标记字段应缺席");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(preserves_declaration_order_for_marked_fields) {
    const auto serialized = serialize_to_jaml(serde_jaml_test_reordered_fields{});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_reordered_fields>\n    <middle>2</middle>\n    <last>3</last>\n  </serde_jaml_test_reordered_fields>\n</jaml>"sv,
        "标记字段应按声明顺序出现");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(serializes_class_with_range_member) {
    const auto serialized =
        serialize_to_jaml(serde_jaml_test_with_range_member{.tag = 5, .items = {10, 20, 30}});

    JUNGLE_SYNC_ASSERT(
        serialized.view()
            == "<jaml>\n  <serde_jaml_test_with_range_member>\n    <tag>5</tag>\n    <items>\n      <list>\n        <item>10</item>\n        <item>20</item>\n        <item>30</item>\n      </list>\n    </items>\n  </serde_jaml_test_with_range_member>\n</jaml>"sv,
        "类中的 range 成员应递归序列化");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_bool) {
    auto r = deserialize<bool, JamlSource>(ustr{"<jaml>true</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == true, "应能从 jaml 文本反序列化 true");

    r = deserialize<bool, JamlSource>(ustr{"<jaml>false</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == false, "应能从 jaml 文本反序列化 false");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_integers) {
    auto r = deserialize<int, JamlSource>(ustr{"<jaml>42</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == 42, "正整数应正确反序列化");
    r = deserialize<int, JamlSource>(ustr{"<jaml>-1</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == -1, "负整数应正确反序列化");
    r = deserialize<int, JamlSource>(ustr{"<jaml>0</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == 0, "零应正确反序列化");

    auto r2 = deserialize<long long, JamlSource>(ustr{"<jaml>9223372036854775807</jaml>"});
    JUNGLE_SYNC_ASSERT(r2.has_value() && *r2 == 9223372036854775807LL, "大 int64 应正确反序列化");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_floating_point) {
    auto r = deserialize<double, JamlSource>(ustr{"<jaml>2.5</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == 2.5, "正浮点数应正确反序列化");
    r = deserialize<double, JamlSource>(ustr{"<jaml>-3.14</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == -3.14, "负浮点数应正确反序列化");
    r = deserialize<double, JamlSource>(ustr{"<jaml>0</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == 0.0, "零浮点数应正确反序列化");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_enum) {
    auto r =
        deserialize<serde_jaml_test_color, JamlSource>(ustr{"<jaml>serde_jaml_test_color::green</jaml>"});
    JUNGLE_SYNC_ASSERT(
        r.has_value() && *r == serde_jaml_test_color::green, "枚举应能从 TypeName::EnumeratorName 还原");
    r = deserialize<serde_jaml_test_color, JamlSource>(ustr{"<jaml>red</jaml>"});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == serde_jaml_test_color::red, "枚举也应能仅用枚举项名还原");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_optional) {
    auto with_value = deserialize<std::optional<int>, JamlSource>(ustr{"<jaml>42</jaml>"});
    JUNGLE_SYNC_ASSERT(
        with_value.has_value() && (*with_value).has_value() && *(*with_value) == 42,
        "有值 optional 应还原内部整数");

    auto nullopt = deserialize<std::optional<int>, JamlSource>(ustr{"<jaml><null/></jaml>"});
    JUNGLE_SYNC_ASSERT(nullopt.has_value() && !(*nullopt).has_value(), "null 元素应还原为空 optional");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_range) {
    auto vec = deserialize<std::vector<int>, JamlSource>(
        ustr{"<jaml><list><item>1</item><item>2</item><item>3</item></list></jaml>"});
    JUNGLE_SYNC_ASSERT(vec.has_value(), "range 反序列化应成功");
    std::vector<int> expected{1, 2, 3};
    JUNGLE_SYNC_ASSERT(*vec == expected, "range 应还原全部元素");

    auto empty_vec = deserialize<std::vector<int>, JamlSource>(ustr{"<jaml><list></list></jaml>"});
    JUNGLE_SYNC_ASSERT(empty_vec.has_value() && (*empty_vec).empty(), "空 list 应还原为空 vector");

    auto self_closed = deserialize<std::vector<int>, JamlSource>(ustr{"<jaml><list/></jaml>"});
    JUNGLE_SYNC_ASSERT(self_closed.has_value() && (*self_closed).empty(), "自闭合 list 也应还原为空 vector");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_nested_ranges) {
    auto nested = deserialize<std::vector<std::vector<int>>, JamlSource>(ustr{
        "<jaml><list><item><list><item>1</item><item>2</item></list></item><item><list><item>3</item><item>4</item><item>5</item></list></item><item><list></list></item></list></jaml>"});
    JUNGLE_SYNC_ASSERT(nested.has_value(), "嵌套 range 反序列化应成功");
    std::vector<std::vector<int>> expected_nested{{1, 2}, {3, 4, 5}, {}};
    JUNGLE_SYNC_ASSERT(*nested == expected_nested, "嵌套 range 应递归还原");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trip_nested_objects) {
    serde_jaml_test_outer original{
        .number = 7,
        .ratio = 2.5,
        .flag = true,
        .color = serde_jaml_test_color::blue,
        .inner = serde_jaml_test_inner{.value = 11, .enabled = false},
        .items = {1, 2, 3}};
    auto text = serialize_to_jaml(original);
    auto restored = deserialize<serde_jaml_test_outer, JamlSource>(ustr{text.view()});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "嵌套对象反序列化应成功");
    auto re_serialized = serialize_to_jaml(*restored);
    JUNGLE_SYNC_ASSERT(re_serialized.view() == text.view(), "序列化再反序列化应得到相同 jaml");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_empty_struct) {
    auto empty = deserialize<serde_empty_struct, JamlSource>(
        ustr{"<jaml><serde_empty_struct></serde_empty_struct></jaml>"});
    JUNGLE_SYNC_ASSERT(empty.has_value(), "空结构体反序列化应成功");

    auto self_closed =
        deserialize<serde_empty_struct, JamlSource>(ustr{"<jaml><serde_empty_struct/></jaml>"});
    JUNGLE_SYNC_ASSERT(self_closed.has_value(), "自闭合空结构体也应反序列化成功");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_marked_fields) {
    auto restored = deserialize<serde_jaml_test_marked_fields, JamlSource>(ustr{
        "<jaml><serde_jaml_test_marked_fields><kept>7</kept><enabled>true</enabled></serde_jaml_test_marked_fields></jaml>"});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "标记字段类反序列化应成功");
    auto re_serialized = serialize_to_jaml(*restored);
    JUNGLE_SYNC_ASSERT(
        re_serialized.view()
            == "<jaml>\n  <serde_jaml_test_marked_fields>\n    <kept>7</kept>\n    <enabled>true</enabled>\n  </serde_jaml_test_marked_fields>\n</jaml>"sv,
        "标记字段类往返应只保留 [[=field]] 成员");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trip_class_with_range_member) {
    serde_jaml_test_with_range_member original{.tag = 5, .items = {10, 20, 30}};
    auto text = serialize_to_jaml(original);
    auto restored = deserialize<serde_jaml_test_with_range_member, JamlSource>(ustr{text.view()});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "带 range 成员的类反序列化应成功");
    auto re_serialized = serialize_to_jaml(*restored);
    JUNGLE_SYNC_ASSERT(re_serialized.view() == text.view(), "带 range 成员的类应能往返");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trip_optional_struct) {
    {
        serde_optional_struct original{.id = 1, .maybe_score = 100};
        auto text = serialize_to_jaml(original);
        auto restored = deserialize<serde_optional_struct, JamlSource>(ustr{text.view()});
        JUNGLE_SYNC_ASSERT(restored.has_value(), "有值 optional 字段反序列化应成功");
        JUNGLE_SYNC_ASSERT(serialize_to_jaml(*restored).view() == text.view(), "有值 optional 字段应能往返");
    }
    {
        serde_optional_struct original{.id = 2, .maybe_score = {}};
        auto text = serialize_to_jaml(original);
        auto restored = deserialize<serde_optional_struct, JamlSource>(ustr{text.view()});
        JUNGLE_SYNC_ASSERT(restored.has_value(), "空 optional 字段反序列化应成功");
        JUNGLE_SYNC_ASSERT(serialize_to_jaml(*restored).view() == text.view(), "空 optional 字段应能往返");
    }
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserializes_optional_of_range) {
    auto with_vec = deserialize<std::optional<std::vector<int>>, JamlSource>(
        ustr{"<jaml><list><item>7</item><item>8</item><item>9</item></list></jaml>"});
    JUNGLE_SYNC_ASSERT(
        with_vec.has_value() && (*with_vec).has_value(), "optional<vector> 有值时应还原内部 vector");
    std::vector<int> expected{7, 8, 9};
    JUNGLE_SYNC_ASSERT(*(*with_vec) == expected, "optional<vector> 内部元素应匹配");

    auto null_vec = deserialize<std::optional<std::vector<int>>, JamlSource>(ustr{"<jaml><null/></jaml>"});
    JUNGLE_SYNC_ASSERT(
        null_vec.has_value() && !(*null_vec).has_value(), "optional<vector> 的 null 应还原为空");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trip_unnamed_type) {
    struct {
        int count;
        bool ready;
    } original{.count = 3, .ready = true};
    auto text = serialize_to_jaml(original);
    auto restored = deserialize<std::remove_cvref_t<decltype(original)>, JamlSource>(ustr{text.view()});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "无名类型反序列化应成功");
    JUNGLE_SYNC_ASSERT(serialize_to_jaml(*restored).view() == text.view(), "无名类型应能通过 _unnamed 往返");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trip_field_customized) {
    serde_jaml_test_field_customized original{};
    auto text = serialize_to_jaml(original);
    auto restored = deserialize<serde_jaml_test_field_customized, JamlSource>(ustr{text.view()});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "字段定制器反序列化应成功");
    JUNGLE_SYNC_ASSERT(restored->value == 42, "字段定制器反序列化后应还原为定制前的值");
    JUNGLE_SYNC_ASSERT(serialize_to_jaml(*restored).view() == text.view(), "字段定制器应能往返");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trip_mixed_annotations) {
    serde_jaml_test_mixed_annotations original{};
    auto text = serialize_to_jaml(original);
    auto restored = deserialize<serde_jaml_test_mixed_annotations, JamlSource>(ustr{text.view()});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "混合注解反序列化应成功");
    JUNGLE_SYNC_ASSERT(serialize_to_jaml(*restored).view() == text.view(), "混合注解应能往返");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialize_into_existing_value) {
    int value = 0;
    auto ok = deserialize<int, JamlSource>(ustr{"<jaml>42</jaml>"}, value);
    JUNGLE_SYNC_ASSERT(ok.has_value() && value == 42, "deserialize(payload, value) 应写入已有变量");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialize_with_direct_source) {
    JamlSource source;
    auto doc = current_jaml_header();
    doc.append("<jaml><list><item>1</item><item>2</item><item>3</item></list></jaml>");
    source.provide_source(ustr{doc.view()});
    std::vector<int> value;
    auto ok = jungle::serde::deserialize(source, value);
    JUNGLE_SYNC_ASSERT(ok.has_value(), "直接操作 source 的反序列化应成功");
    std::vector<int> expected{1, 2, 3};
    JUNGLE_SYNC_ASSERT(value == expected, "deserialize(source, value) 应填入预构造对象");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialize_optional_clears_existing) {
    std::optional<int> value{999};
    auto ok = deserialize<std::optional<int>, JamlSource>(ustr{"<jaml><null/></jaml>"}, value);
    JUNGLE_SYNC_ASSERT(ok.has_value() && !value.has_value(), "写入 null 应清空已有 optional");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(accepts_whitespace_and_comments) {
    // 有效头由测试助手提供；正文含空白与注释，应被忽略。
    auto doc = current_jaml_header();
    doc.append(
        "<jaml>\n"
        "  <!-- numbers -->\n"
        "  <list>\n"
        "    <item>1</item>\n"
        "    <item>2</item>\n"
        "  </list>\n"
        "</jaml>\n");
    auto vec = deserialize<std::vector<int>, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(vec.has_value(), "带头文档内的空白与注释应被忽略并正确解析");
    std::vector<int> expected{1, 2};
    JUNGLE_SYNC_ASSERT(*vec == expected, "忽略空白与注释后元素应匹配");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(rejects_invalid_payloads) {
    using kind = JamlSource::error_type::kind;

    // 空输入缺少 XML 声明头：无论 Debug/Release 都应反序列化失败。
    auto empty = jungle::serde::deserialize<int, JamlSource>(ustr{""});
    JUNGLE_SYNC_ASSERT(!empty.has_value(), "空输入应失败");
    JUNGLE_SYNC_ASSERT(
        empty.error().m_kind == kind::missing_xml_header, "空输入缺少 XML 头应为 missing_xml_header");
    JUNGLE_SYNC_ASSERT(empty.error().m_position == 0, "空输入错误位置应在开头");

    auto bool_as_int = deserialize<int, JamlSource>(ustr{"<jaml>true</jaml>"});
    JUNGLE_SYNC_ASSERT(!bool_as_int.has_value(), "用 bool 文本反序列化 int 应失败");
    JUNGLE_SYNC_ASSERT(
        bool_as_int.error().m_kind == kind::expected_number, "bool 文本反序列化 int 应为 expected_number");

    auto bad_bool = deserialize<bool, JamlSource>(ustr{"<jaml>yes</jaml>"});
    JUNGLE_SYNC_ASSERT(!bad_bool.has_value(), "非法 bool 文本应失败");
    JUNGLE_SYNC_ASSERT(bad_bool.error().m_kind == kind::expected_bool, "非法 bool 文本应为 expected_bool");
    JUNGLE_SYNC_ASSERT(bad_bool.error().m_extent == 3, "非法 bool 的 extent 应为 token 长度");

    auto missing_close = deserialize<std::vector<int>, JamlSource>(ustr{"<jaml><list><item>1</item></jaml>"});
    JUNGLE_SYNC_ASSERT(!missing_close.has_value(), "缺少闭合标签应失败");
    JUNGLE_SYNC_ASSERT(
        missing_close.error().m_kind == kind::tag_mismatch, "缺少 list 闭合标签应为 tag_mismatch");
    JUNGLE_SYNC_ASSERT(missing_close.error().m_extent == 4, "错误 span 应对准实际读到的 jaml 标签名");

    auto overflow = deserialize<int, JamlSource>(ustr{"<jaml>9999999999999999999</jaml>"});
    JUNGLE_SYNC_ASSERT(!overflow.has_value(), "超出 int 范围的整数应失败");
    JUNGLE_SYNC_ASSERT(
        overflow.error().m_kind == kind::number_out_of_range, "过大整数应为 number_out_of_range");

    auto unknown_enum = deserialize<serde_jaml_test_color, JamlSource>(ustr{"<jaml>yellow</jaml>"});
    JUNGLE_SYNC_ASSERT(!unknown_enum.has_value(), "未知枚举项应失败");
    JUNGLE_SYNC_ASSERT(
        unknown_enum.error().m_kind == kind::unknown_enumerator, "未知枚举项应为 unknown_enumerator");
    JUNGLE_SYNC_ASSERT(unknown_enum.error().m_extent == 6, "未知枚举项的 extent 应为 token 长度");

    auto not_list = deserialize<std::vector<int>, JamlSource>(ustr{"<jaml>42</jaml>"});
    JUNGLE_SYNC_ASSERT(!not_list.has_value(), "用标量反序列化 range 应失败");
    JUNGLE_SYNC_ASSERT(not_list.error().m_kind == kind::expected_list, "标量处期望 list 标签");

    auto malformed = deserialize<serde_empty_struct, JamlSource>(ustr{"<jaml><</jaml>"});
    JUNGLE_SYNC_ASSERT(!malformed.has_value(), "残缺起始标签应失败");
    JUNGLE_SYNC_ASSERT(malformed.error().m_kind == kind::malformed_tag, "残缺起始标签应为 malformed_tag");

    JUNGLE_SYNC_SUCCESS();
}

// ---- XML 声明头与 build_id 版本校验 ----

JUNGLE_SYNC_TEST(serialized_document_begins_with_versioned_xml_declaration) {
    // 完整文档（含头）应精确形如：
    // <?xml version="1.0" encoding="UTF-8" jaml_version="{}<{:032x}>"?>
    const auto doc = serialize<JamlTarget>(42);

    auto expected = current_jaml_header();
    expected.append("<jaml>\n  42\n</jaml>");

    JUNGLE_SYNC_ASSERT(
        doc.view() == expected.view(),
        "序列化文档应以带 jaml_version=\"build_id_str<build_id:032x>\" 的 XML 声明头开头");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(round_trips_document_with_xml_header) {
    const serde_jaml_test_outer original{
        .number = 7,
        .ratio = 2.5,
        .flag = true,
        .color = serde_jaml_test_color::blue,
        .inner = serde_jaml_test_inner{.value = 11, .enabled = false},
        .items = {1, 2, 3}};
    const auto doc = serialize<JamlTarget>(original);
    auto restored = deserialize<serde_jaml_test_outer, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(restored.has_value(), "带头 XML 文档反序列化应成功");
    const auto redoc = serialize<JamlTarget>(*restored);
    JUNGLE_SYNC_ASSERT(redoc.view() == doc.view(), "带头文档应能完整往返");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialization_verifies_only_build_id_hex) {
    // build_id_str 仅供人类阅读：字符串部分被改掉，但只要 build_id() 十六进制一致即应成功。
    const auto hex = current_build_id_hex();
    auto doc = jaml_header_with("fake-readable-string", hex.view());
    doc.append("<jaml>42</jaml>");

    auto r = deserialize<int, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(
        r.has_value() && *r == 42, "反序列化仅校验 build_id() 对应的十六进制段，字符串部分不影响结果");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialization_build_id_mismatch_behavior) {
    // 用全零十六进制伪造一个必然与当前 build_id() 不同的版本。
    const auto zeros = ustr::format("{:032x}", jungle::u128{0});
    auto doc = jaml_header_with("stale-build", zeros.view());
    doc.append("<jaml>42</jaml>");

#ifdef JUNGLE_DEBUG_ENABLED
    // Debug / RelWithDebInfo：build_id 不匹配时静默放行。
    // TODO：未来日志系统完成后，此处应改为 warning 日志提示载荷来自不同构建。
    auto r = deserialize<int, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(r.has_value() && *r == 42, "Debug 下 build_id 不匹配应静默忽略并继续解析");
#else
    // Release：build_id 不匹配 → 反序列化失败。
    auto r = deserialize<int, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(!r.has_value(), "Release 下 build_id 不匹配应反序列化失败");
    JUNGLE_SYNC_ASSERT(
        r.error().m_kind == JamlSource::error_type::kind::build_id_mismatch,
        "Release 下失败错误类型应为 build_id_mismatch");
#endif
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialization_rejects_document_without_header) {
    // 无 XML 声明头的文档：Debug 与 Release 都视为反序列化失败。
    auto r = jungle::serde::deserialize<int, JamlSource>(ustr{"<jaml>42</jaml>"});
    JUNGLE_SYNC_ASSERT(!r.has_value(), "缺少 XML 声明头的文档应反序列化失败");
    JUNGLE_SYNC_ASSERT(
        r.error().m_kind == JamlSource::error_type::kind::missing_xml_header,
        "缺头失败错误类型应为 missing_xml_header");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialization_rejects_header_without_jaml_version) {
    // 声明头存在但缺少 jaml_version：视为无效头，Debug 与 Release 都应失败。
    jungle::ustr doc{"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<jaml>42</jaml>"};
    auto r = jungle::serde::deserialize<int, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(!r.has_value(), "缺少 jaml_version 的声明头应反序列化失败");
    JUNGLE_SYNC_ASSERT(
        r.error().m_kind == JamlSource::error_type::kind::invalid_xml_header,
        "无效头失败错误类型应为 invalid_xml_header");
    JUNGLE_SYNC_SUCCESS();
}

JUNGLE_SYNC_TEST(deserialization_rejects_malformed_jaml_version) {
    // jaml_version 缺少 <hex> 段：视为无效头，Debug 与 Release 都应失败。
    jungle::ustr doc{"<?xml version=\"1.0\" encoding=\"UTF-8\" jaml_version=\"no-hex\"?>\n<jaml>42</jaml>"};
    auto r = jungle::serde::deserialize<int, JamlSource>(ustr{doc.view()});
    JUNGLE_SYNC_ASSERT(!r.has_value(), "缺少 <hex> 段的 jaml_version 应反序列化失败");
    JUNGLE_SYNC_ASSERT(
        r.error().m_kind == JamlSource::error_type::kind::invalid_xml_header,
        "无效头失败错误类型应为 invalid_xml_header");
    JUNGLE_SYNC_SUCCESS();
}

}  // namespace
