//
// Created by 46769 on 2021-01-22.
//

#pragma once
#include <string>
#include <vector>

struct SubTest {
    int test_id;
    bool success;
    std::string failMessage;
};

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
    UnitTest(std::string name, std::string fileLocStr, bool failImmediate);
    static UnitTest Test(std::string name, std::string fileLocStr, bool failImmediate = false);
    void push_result(std::string msg, bool result);
    ~UnitTest() noexcept(false);

private:
    std::string m_name;
    std::string m_fileLocStr;
    bool failImmediate;
    std::vector<SubTest> sub_tests;
};
