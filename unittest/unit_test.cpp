//
// Created by 46769 on 2021-01-22.
//

#include "unit_test.hpp"
#include <utility>
#include <stdexcept>
#include <fmt/core.h>
#include <fmt/format.h>

UnitTest::~UnitTest() noexcept(false) {
    for(const auto& st : sub_tests) {
        if(!st.success) {
            throw std::runtime_error{fmt::format(FMT_STRING("Sub test failed: {} in unit test {}: '{}'"), st.test_id, m_name, st.failMessage)};
        }
    }
    auto left_aligned = fmt::format(FMT_STRING("Test:{}"), m_name);
    auto right_aligned = fmt::format(FMT_STRING("{} -- passed ({} assertions)\n"), m_fileLocStr, sub_tests.size());
    fmt::print(FMT_STRING("{:<75} - {:>}"), left_aligned, right_aligned);
    // fmt::print(FMT_STRING("Test:{} - {} -- passed ({} assertions)\n"), m_name,m_fileLocStr, sub_tests.size());
    fflush(stdout);
}
UnitTest::UnitTest(std::string name, std::string fileLocStr, bool failImmediate) : m_name(std::move(name)), m_fileLocStr(std::move(fileLocStr)), failImmediate(failImmediate) {

}

UnitTest UnitTest::Test(std::string name, std::string fileLocStr, bool failImmediate) {
    return UnitTest{std::move(name), std::move(fileLocStr), failImmediate};
}
void UnitTest::push_result(std::string msg, bool result) {
    sub_tests.emplace_back((int)sub_tests.size(), result, msg);
    if(failImmediate && !result) {
        throw std::runtime_error{fmt::format(FMT_STRING("FAILURE: Sub test failed: {} in unit test {} - {} = '{}'"), sub_tests.size()-1, m_name, m_fileLocStr, msg)};
    }
}
