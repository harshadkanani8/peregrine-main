#include "test_framework.hpp"

int main() {
    return ::peregrine_test::TestRegistry::instance().run_all();
}
