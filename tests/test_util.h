#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include "../src/alma.h"

void ref_mul(const double* A, const double* B, double* C, int n);

bool load_csv(const std::string& path, std::vector<double>& M, int& n);

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
        int passed = 0;
        int failed = 0;
        
        for (const auto& t : tests) {
            std::cout << "Test: " << t.name << "... ";
            if (t.func()) {
                std::cout << "PASSED\n";
                passed++;
            } else {
                std::cout << "FAILED\n";
                failed++;
            }
        }
        
        std::cout << "\n====================\n";
        std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
        
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
