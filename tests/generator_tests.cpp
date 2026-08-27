#include <amka/testing.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <random>
#include <unordered_set>
#include <vector>

using amka::testing::make_sin;
using amka::testing::make_sins;

TEST_CASE("determinism: same seed, same stdlib => same fixtures") {
    std::mt19937 g1{42};
    std::mt19937 g2{42};
    for (int i = 0; i < 8; ++i) {
        REQUIRE(make_sin(g1) == make_sin(g2));
    }
}

TEST_CASE("everything generated passes the public gate") {
    std::mt19937 g{7};
    bool all_valid = true;
    for (int i = 0; i < 2000; ++i) {
        const amka::sin s = make_sin(g);
        all_valid = all_valid && amka::valid(s.str());
    }
    REQUIRE(all_valid);
}

TEST_CASE("date constraint is honoured, including lenient Feb 29") {
    std::mt19937 g{1};

    const amka::date d{15, 3, 90};
    bool all_match = true;
    for (int i = 0; i < 200; ++i) {
        const amka::sin s = make_sin(g, d);
        const amka::date b = s.birth_date();
        all_match = all_match && b.day == 15 && b.month == 3 && b.year2 == 90;
    }
    REQUIRE(all_match);

    // 29/02/'00 is lenient-valid (2000 was a leap year) — must be generable.
    const amka::sin leap = make_sin(g, amka::date{29, 2, 0});
    REQUIRE(amka::valid(leap.str()));
    REQUIRE(leap.birth_date().day == 29);
    REQUIRE(leap.birth_date().month == 2);
}

TEST_CASE("sex constraint fixes serial parity exactly") {
    std::mt19937 g{3};
    const amka::date d{1, 6, 85};

    bool males_odd = true;
    bool females_even = true;
    for (int i = 0; i < 300; ++i) {
        males_odd = males_odd && (make_sin(g, d, amka::sex::male).serial() % 2 == 1);
        females_even = females_even && (make_sin(g, d, amka::sex::female).serial() % 2 == 0);
    }
    REQUIRE(males_odd);
    REQUIRE(females_even);

    // ...and encoded_sex() round-trips the request.
    REQUIRE(make_sin(g, d, amka::sex::male).encoded_sex() == amka::sex::male);
    REQUIRE(make_sin(g, d, amka::sex::female).encoded_sex() == amka::sex::female);
}

TEST_CASE("make_sins: n results, all distinct (the birthday-paradox guarantee)") {
    std::mt19937 g{99};
    const std::vector<amka::sin> v = make_sins(g, 10'000);
    REQUIRE(v.size() == 10'000);
    const std::unordered_set<amka::sin> uniq(v.begin(), v.end());
    REQUIRE(uniq.size() == 10'000);
}

TEST_CASE("make_sins at full per-date capacity is EXACTLY the serial space") {
    std::mt19937 g{5};
    const amka::date d{1, 1, 70};
    const std::vector<amka::sin> v = make_sins(g, 10'000, d);
    REQUIRE(v.size() == 10'000);

    std::vector<unsigned> serials;
    serials.reserve(v.size());
    bool dates_ok = true;
    for (const amka::sin& s : v) {
        serials.push_back(s.serial());
        const amka::date b = s.birth_date();
        dates_ok = dates_ok && b.day == 1 && b.month == 1 && b.year2 == 70;
    }
    REQUIRE(dates_ok);

    std::sort(serials.begin(), serials.end());
    bool exact_cover = true;
    for (unsigned k = 0; k < 10'000; ++k) exact_cover = exact_cover && serials[k] == k;
    REQUIRE(exact_cover);  // every serial 0..9999, each exactly once
}

TEST_CASE("make_sins at full (date, sex) capacity covers the parity class once") {
    std::mt19937 g{6};
    const amka::date d{31, 12, 99};

    const std::vector<amka::sin> males = make_sins(g, 5'000, d, amka::sex::male);
    std::vector<unsigned> serials;
    serials.reserve(males.size());
    for (const amka::sin& s : males) serials.push_back(s.serial());
    std::sort(serials.begin(), serials.end());

    bool exact_cover = serials.size() == 5'000;
    for (unsigned k = 0; k < 5'000 && exact_cover; ++k)
        exact_cover = serials[k] == 2 * k + 1;   // 1, 3, 5, ..., 9999
    REQUIRE(exact_cover);
}

TEST_CASE("batches are seed-reproducible too") {
    std::mt19937 g1{2026};
    std::mt19937 g2{2026};
    const auto a = make_sins(g1, 500);
    const auto b = make_sins(g2, 500);
    REQUIRE(a == b);   // std::vector::operator== over sin's operator==
}
