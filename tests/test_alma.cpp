#include "test_util.h"

int main() {
    std::cout << "Running alma tests...\n\n";
    return TestSuite::instance().run();
}
