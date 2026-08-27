#include <amka/sin.hpp>

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

TEST_CASE("check() diagnoses in the documented precedence") {
    using amka::error;
    REQUIRE(amka::check("01013099997") == error::none);
    REQUIRE(amka::check("123") == error::invalid_length);
    REQUIRE(amka::check("abcdefghijk") == error::invalid_character);
    REQUIRE(amka::check("99999999999") == error::invalid_date);
    REQUIRE(amka::check("09095986680") == error::invalid_checksum);
}

TEST_CASE("to_string names every error") {
    using amka::error;
    REQUIRE(amka::to_string(error::none) == "none");
    REQUIRE(amka::to_string(error::invalid_length) == "invalid_length");
    REQUIRE(amka::to_string(error::invalid_character) == "invalid_character");
    REQUIRE(amka::to_string(error::invalid_date) == "invalid_date");
    REQUIRE(amka::to_string(error::invalid_checksum) == "invalid_checksum");
}

TEST_CASE("Luhn: exactly one check digit validates any 10-digit prefix") {
    // A property test with zero randomness — and the mathematical guarantee
    // the M2 generator will rely on: c = (10 - S mod 10) mod 10 is unique.
    const std::vector<std::string> prefixes = {
        "0101309999",  // corpus prefixes...
        "0909598668",
        "2106830267",
        "2010820182",
        "2405820267",
        "2902040000",  // ...and the leap-'04 fixture
        "1503900000",
        "3112999999",
    };
    for (const auto& p : prefixes) {
        int hits = 0;
        for (char c = '0'; c <= '9'; ++c) {
            if (amka::valid(p + c)) ++hits;
        }
        CAPTURE(p);
        REQUIRE(hits == 1);
    }
}

TEST_CASE("sin is a first-class key in both map families") {
    const auto a = amka::parse("01013099997");
    const auto b = amka::parse("09095986684");
    const auto c = amka::parse("21068302674");
    REQUIRE(a);
    REQUIRE(b);
    REQUIRE(c);

    // unordered: exercises std::hash<amka::sin>
    std::unordered_set<amka::sin> uniq{*a, *b, *c};
    REQUIRE(uniq.size() == 3);
    uniq.insert(*a);  // duplicate must be absorbed
    REQUIRE(uniq.size() == 3);

    // equal values must hash equally (fresh parse, same digits)
    REQUIRE(std::hash<amka::sin>{}(*a) == std::hash<amka::sin>{}(*amka::parse("01013099997")));

    // ordered: exercises operator< (lexicographic over the digits)
    std::set<amka::sin> ordered{*c, *a, *b};
    REQUIRE(ordered.begin()->str() == "01013099997");
}

TEST_CASE("parse() round-trips the exact digits and decodes the fields") {
    const std::string raw = "20108201821";
    const auto id = amka::parse(raw);
    REQUIRE(id);
    REQUIRE(id->str() == raw);
    REQUIRE(id->birth_date().day == 20);
    REQUIRE(id->birth_date().month == 10);
    REQUIRE(id->birth_date().year2 == 82);
    REQUIRE(id->serial() == 182);
    REQUIRE(id->encoded_sex() == amka::sex::female);  // serial 0182: even
}

TEST_CASE("strict century overload") {
    using amka::error;
    REQUIRE(amka::valid("29020000013"));                            // lenient default
    REQUIRE(amka::check("29020000013", 2000) == error::none);
    REQUIRE(amka::check("29020000013", 1900) == error::invalid_date);
    REQUIRE(amka::check("20108201821", 1982) == error::none);
    REQUIRE(amka::check("20108201821", 1983) == error::invalid_date);  // YY mismatch
    REQUIRE(amka::valid("20108201821", 1982));
}
