#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include "alma/alma.h"
#include "io/csv_utils.h"

void ref_mul(const double* A, const double* B, double* C, int n);

bool test_matrix(int n, int block, const std::vector<double>& A, const std::vector<double>& B);

struct TestCase {
    const char* name;
    bool (*func)();
};

class TestSuite {
public:
    static TestSuite& instance() {
        static TestSuite s;
        return s;
    }
    
    void add(const char* name, bool (*func)()) {
        tests.push_back({name, func});
    }
    
    int run() {
        int failed = 0;
        
        std::cout << "1.." << tests.size() << "\n";
        
        for (size_t i = 0; i < tests.size(); ++i) {
            const auto& t = tests[i];
            bool result = t.func();
            if (result) {
                std::cout << "ok " << (i + 1) << " - " << t.name << "\n";
            } else {
                std::cout << "not ok " << (i + 1) << " - " << t.name << "\n";
                failed++;
            }
        }
        
        return failed;
    }
    
    size_t count() const { return tests.size(); }

private:
    TestSuite() = default;
    std::vector<TestCase> tests;
};

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define REGISTER_TEST(name, func) \
    namespace { struct CONCAT(Registrator_, __LINE__) { CONCAT(Registrator_, __LINE__)() { TestSuite::instance().add(name, func); } } CONCAT(registrator_, __LINE__); }
