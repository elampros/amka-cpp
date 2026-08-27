// amka-gen: print n synthetic, mutually-distinct AMKA — one per line.
//
//   $ amka-gen 3 42        # seeded => reproducible on the same stdlib
//   $ amka-gen 100         # unseeded => std::random_device
//
// The output is FICTION: structurally valid by construction, never checked
// against any registry, and possibly coinciding with really issued numbers.

#include <amka/testing.hpp>

#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: amka-gen <n> [seed]\n";
        return 2;
    }

    const unsigned long n = std::strtoul(argv[1], nullptr, 10);
    const unsigned seed = argc == 3
        ? static_cast<unsigned>(std::strtoul(argv[2], nullptr, 10))
        : std::random_device{}();

    std::mt19937 g{seed};
    for (const amka::sin& s : amka::testing::make_sins(g, n)) {
        std::cout << s.str() << '\n';
    }
    return 0;
}
