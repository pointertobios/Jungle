#[test]
fn component_macro_errors() {
    let test_cases = trybuild::TestCases::new();
    test_cases.compile_fail("tests/ui/component_macro/*.rs");
}
