// Smoke test: consume the installed package via find_package(amka).
#include <amka/sin.hpp>

static_assert(amka::valid("01013099997"));
static_assert(!amka::valid("01013099996"));

int main() {
    return amka::parse("01013099997").has_value() ? 0 : 1;
}
