// amka-cpp — synthetic AMKA generation for tests and fixtures.
//
// Runtime-only by nature (<random>), deliberately split from the constexpr
// core so production translation units never pay <random>'s compile time.
//
// Contract:
//   * The caller supplies the URBG (std::shuffle style). No hidden globals,
//     no silent clock seeding: same URBG type + seed + standard library
//     => same fixtures. NOTE: std::uniform_int_distribution's algorithm is
//     implementation-defined, so sequences are reproducible per standard
//     library, not across different ones (libstdc++ vs libc++ vs MSVC).
//   * Every returned amka::sin is obtained through amka::parse() — the
//     generator holds no keys to the type; it walks through the same gate
//     as user input. Assembly bugs therefore fail loudly, at the boundary.
//   * Preconditions are checked with assert(); violating them in NDEBUG
//     builds is undefined behaviour, as with standard-library preconditions.
//   * Synthetic numbers are structurally valid BY CONSTRUCTION and may
//     coincide with really issued ones — AMKA has no reserved test range.
//     Do not treat them as anonymised data; treat them as fiction.
//
// Capacity (see README): 10'000 valid AMKA per birth date, 5'000 per
// (birth date, sex), 365'250'000 in total under the lenient century policy.

#ifndef AMKA_TESTING_HPP_INCLUDED
#define AMKA_TESTING_HPP_INCLUDED

#include <amka/sin.hpp>

#include <algorithm>      // std::shuffle
#include <array>
#include <cassert>
#include <cstddef>
#include <numeric>        // std::iota
#include <random>
#include <unordered_set>
#include <vector>

namespace amka::testing {

namespace detail {

// Luhn check digit for the first 10 digits of the (future) 11-digit number.
// Positions 0..9 use the same doubling rule as the full string (double odd
// 0-based indices); position 10 — the check digit itself — is undoubled, so:
//   c = (10 - S mod 10) mod 10
// This is the uniqueness property the runtime suite proves exhaustively.
inline unsigned check_digit(const std::array<char, 11>& b) noexcept {
    unsigned sum = 0;
    for (std::size_t i = 0; i < 10; ++i) {
        unsigned v = amka::detail::digit(b[i]);
        if (i % 2 == 1) {
            v *= 2;
            if (v > 9) v -= 9;
        }
        sum += v;
    }
    return (10 - sum % 10) % 10;
}

inline void write2(std::array<char, 11>& b, std::size_t at, unsigned v) noexcept {
    b[at]     = static_cast<char>('0' + v / 10 % 10);
    b[at + 1] = static_cast<char>('0' + v % 10);
}

inline void write_serial(std::array<char, 11>& b, unsigned serial) noexcept {
    b[6] = static_cast<char>('0' + serial / 1000 % 10);
    b[7] = static_cast<char>('0' + serial / 100 % 10);
    b[8] = static_cast<char>('0' + serial / 10 % 10);
    b[9] = static_cast<char>('0' + serial % 10);
}

inline bool lenient_valid_date(date d) noexcept {
    return d.month >= 1 && d.month <= 12 && d.year2 <= 99 && d.day >= 1 &&
           d.day <= amka::detail::days_in_month(
                        d.month, amka::detail::lenient_leap(d.year2));
}

// Assemble a full AMKA and hand it to the public gate. No friendship, no
// shortcut: parse() re-validates everything, doubling as an internal check.
inline sin assemble(date d, unsigned serial) noexcept {
    std::array<char, 11> b{};
    write2(b, 0, d.day);
    write2(b, 2, d.month);
    write2(b, 4, d.year2);
    write_serial(b, serial);
    b[10] = static_cast<char>('0' + check_digit(b));

    const std::optional<sin> s = amka::parse(std::string_view{b.data(), b.size()});
    assert(s && "amka::testing internal error: assembled AMKA failed parse()");
    return *s;
}

// Draw order (part of the determinism contract): year, month, day.
template <class URBG>
date random_date(URBG& g) {
    std::uniform_int_distribution<unsigned> year_d(0, 99);
    std::uniform_int_distribution<unsigned> month_d(1, 12);
    const unsigned yy = year_d(g);
    const unsigned mm = month_d(g);
    std::uniform_int_distribution<unsigned> day_d(
        1, amka::detail::days_in_month(mm, amka::detail::lenient_leap(yy)));
    return date{day_d(g), mm, yy};
}

} // namespace detail

// ---- Single values -----------------------------------------------------------

// Fixed date and sex. Serial parity encodes sex, so we draw uniformly over
// the 5'000-strong parity class directly (k -> 2k [+1]) — exact, no rejection.
template <class URBG>
sin make_sin(URBG& g, date d, sex s) {
    assert(detail::lenient_valid_date(d) &&
           "make_sin: date must be a lenient-valid DDMMYY (see README, footgun #1)");
    std::uniform_int_distribution<unsigned> half(0, 4999);
    const unsigned serial = half(g) * 2 + (s == sex::male ? 1u : 0u);
    return detail::assemble(d, serial);
}

// Fixed date, any serial.
template <class URBG>
sin make_sin(URBG& g, date d) {
    assert(detail::lenient_valid_date(d) &&
           "make_sin: date must be a lenient-valid DDMMYY (see README, footgun #1)");
    std::uniform_int_distribution<unsigned> serial(0, 9999);
    return detail::assemble(d, serial(g));
}

// Fully random. Distribution is unspecified beyond: every structurally valid
// AMKA has non-zero probability. Draw order: date (year, month, day), serial.
template <class URBG>
sin make_sin(URBG& g) {
    const date d = detail::random_date(g);
    return make_sin(g, d);
}

// ---- Batches (all results mutually distinct) ---------------------------------
//
// Two capacity regimes, two algorithms:
//   * unconstrained space (365.25M): rejection sampling with a seen-set.
//     Collisions only matter near sqrt(space) ≈ 19k draws, and each retry
//     is cheap — expected extra draws ≈ n²/(2·space).
//   * fixed-date spaces (10k / 5k): materialise the whole serial space,
//     std::shuffle it with the caller's URBG, take the first n. Exact
//     sampling without replacement, no retry loop, no tail risk.

template <class URBG>
std::vector<sin> make_sins(URBG& g, std::size_t n) {
    assert(n <= 365'250'000ull &&
           "make_sins: n exceeds the space of structurally valid AMKA");
    std::vector<sin> out;
    out.reserve(n);
    std::unordered_set<sin> seen;
    seen.reserve(n);
    while (out.size() < n) {
        const sin s = make_sin(g);
        if (seen.insert(s).second) out.push_back(s);
    }
    return out;
}

template <class URBG>
std::vector<sin> make_sins(URBG& g, std::size_t n, date d) {
    assert(detail::lenient_valid_date(d) &&
           "make_sins: date must be a lenient-valid DDMMYY");
    assert(n <= 10'000 &&
           "make_sins: at most 10000 distinct AMKA share one birth date (pigeonhole)");
    std::vector<unsigned> serials(10'000);
    std::iota(serials.begin(), serials.end(), 0u);
    std::shuffle(serials.begin(), serials.end(), g);

    std::vector<sin> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) out.push_back(detail::assemble(d, serials[i]));
    return out;
}

template <class URBG>
std::vector<sin> make_sins(URBG& g, std::size_t n, date d, sex s) {
    assert(detail::lenient_valid_date(d) &&
           "make_sins: date must be a lenient-valid DDMMYY");
    assert(n <= 5'000 &&
           "make_sins: at most 5000 distinct AMKA share one (birth date, sex) (pigeonhole)");
    std::vector<unsigned> serials;
    serials.reserve(5'000);
    for (unsigned k = 0; k < 5'000; ++k)
        serials.push_back(k * 2 + (s == sex::male ? 1u : 0u));
    std::shuffle(serials.begin(), serials.end(), g);

    std::vector<sin> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) out.push_back(detail::assemble(d, serials[i]));
    return out;
}

} // namespace amka::testing

#endif // AMKA_TESTING_HPP_INCLUDED
