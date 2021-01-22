//
// Created by 46769 on 2021-01-22.
//

#pragma once
#include <string>
#include <vector>

/// Really a bad name, this is meant to keep track of assertions within a unit test. For instance, we might be testing a specific function or feature, and really testing it might require
/// running a whole *host* of iterations with varius edge cases (for instance search_test & search_from_test tests)
struct SubTest {
    int test_id;
    bool success;
    std::string failMessage;
};

/// Requires &lt;string_view&gt; and &lt;filesystem&gt; to be included in the CU, and of course &lt;fmt/format.h&gt;
// Requires <string_view> and <filesystem> to be included in the CU, and of course <fmt/format.h>
#define BeginUnitTest()                             \
    auto p = std::filesystem::path{__FILE__};       \
    std::string fn_tmp_sig{__FUNCSIG__};            \
    std::string_view fn_sig{fn_tmp_sig};            \
    auto pos = fn_sig.find("__cdecl");              \
    fn_sig.remove_prefix(pos + "__cdecl"sv.size()); \
    auto unitTestInfo = UnitTest::Test(std::string{fn_sig}, FORMAT("{}:{}", p.filename().string(), __LINE__), true)

#define UnitTestPush(failMessage, fnResult) unitTestInfo.push_result(failMessage, fnResult);

class UnitTest {
public:
    /// takes function name, file location in the format of "file.cpp:lineNumber", and if test should abort the test run immediately on err
    UnitTest(std::string name, std::string fileLocStr, bool failImmediate);

    /// Factory function for UnitTest type, takes function name, file location in the format of "file.cpp:lineNumber", and if test should abort the test run immediately on err
    static UnitTest Test(std::string name, std::string fileLocStr, bool failImmediate = false);

    /// Push a result, with the fail message which gets printed on error
    void push_result(std::string msg, bool result);
    ~UnitTest() noexcept(false);

private:
    std::string m_name;
    std::string m_fileLocStr;
    bool failImmediate;
    std::vector<SubTest> sub_tests;
};
