// amka-cpp — validation and parsing of the Greek Social Insurance Number (ΑΜΚΑ)
// Header-only, C++17, zero dependencies.
//
// AMKA structure (11 digits):   D D M M Y Y S S S S C
//   DDMMYY — date of birth (two-digit year: the century is NOT encoded)
//   SSSS   — registry serial within the birth date; its parity encodes
//            administrative sex (odd = male, even = female)
//   C      — Luhn check digit over the full 11-digit number
//
// Design notes live in README.md ("Footguns" section). Short version:
//   * validity is structural — it never implies the number exists or is assigned
//   * the century of the birth year is a caller-side decision (see check overloads)

#ifndef AMKA_SIN_HPP_INCLUDED
#define AMKA_SIN_HPP_INCLUDED

#include <array>
#include <cstdint>
#include <functional>   // std::hash primary template
#include <optional>
#include <string_view>

namespace amka {

inline constexpr std::string_view version = "0.2.0";

enum class error : std::uint8_t {
    none = 0,
    invalid_length,     // input is not exactly 11 characters
    invalid_character,  // input contains a non-digit character
    invalid_date,       // leading DDMMYY is not a plausible date (see century policy)
    invalid_checksum    // Luhn check digit mismatch
};

// Technical names for logs, CLIs and asserts.
// Human-facing (localized) wording is deliberately the caller's job.
constexpr std::string_view to_string(error e) noexcept {
    switch (e) {
        case error::none:              return "none";
        case error::invalid_length:    return "invalid_length";
        case error::invalid_character: return "invalid_character";
        case error::invalid_date:      return "invalid_date";
        case error::invalid_checksum:  return "invalid_checksum";
    }
    return "unknown";  // unreachable for in-range values; keeps -Wreturn-type honest
}

struct date {
    unsigned day;    // 1..31
    unsigned month;  // 1..12
    unsigned year2;  // 0..99 — exactly as encoded; no century guess is made
};

enum class sex : std::uint8_t { male, female };  // as *encoded* by serial parity

namespace detail {

constexpr bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

constexpr unsigned digit(char c) noexcept {
    return static_cast<unsigned>(c) - static_cast<unsigned>('0');
}

// Century policy (lenient): a two-digit year YY names both 19YY and 20YY.
//   20YY is a leap year iff YY % 4 == 0   (2000 included: divisible by 400)
//   19YY is a leap year iff YY % 4 == 0 and YY != 0   (1900: /100 but not /400)
// A date is accepted if it exists in AT LEAST ONE candidate century, so the
// union of the two rules collapses to: YY % 4 == 0.
constexpr bool lenient_leap(unsigned yy) noexcept { return yy % 4 == 0; }

constexpr bool gregorian_leap(unsigned year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

constexpr unsigned days_in_month(unsigned m, bool leap) noexcept {
    constexpr unsigned table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && leap) return 29;
    return table[m - 1];
}

// Luhn over the full 11-digit string. Because the length is fixed and odd,
// "double every second digit from the right" is exactly "double the digits
// at odd 0-based indices" — the index arithmetic is settled at design time.
constexpr bool luhn_ok(std::string_view s) noexcept {
    unsigned sum = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        unsigned v = digit(s[i]);
        if (i % 2 == 1) {
            v *= 2;
            if (v > 9) v -= 9;
        }
        sum += v;
    }
    return sum % 10 == 0;
}

constexpr error check_structure(std::string_view s) noexcept {
    if (s.size() != 11) return error::invalid_length;
    for (const char c : s)
        if (!is_digit(c)) return error::invalid_character;
    return error::none;
}

// Precondition: check_structure(s) == error::none.
constexpr error check_semantics(std::string_view s, bool leap) noexcept {
    const unsigned dd = digit(s[0]) * 10 + digit(s[1]);
    const unsigned mm = digit(s[2]) * 10 + digit(s[3]);
    if (mm < 1 || mm > 12) return error::invalid_date;
    if (dd < 1 || dd > days_in_month(mm, leap)) return error::invalid_date;
    if (!luhn_ok(s)) return error::invalid_checksum;
    return error::none;
}

} // namespace detail

// Diagnose an AMKA candidate. Errors are reported in documented precedence:
// length -> character -> date -> checksum. Uses the lenient century policy.
constexpr error check(std::string_view s) noexcept {
    if (const error e = detail::check_structure(s); e != error::none) return e;
    const unsigned yy = detail::digit(s[4]) * 10 + detail::digit(s[5]);
    return detail::check_semantics(s, detail::lenient_leap(yy));
}

// Strict variant for callers who know the full birth year (e.g. 1900 vs 2000).
// Additionally requires the encoded YY to equal full_birth_year % 100 and
// applies the proper Gregorian leap rule for that exact year.
constexpr error check(std::string_view s, unsigned full_birth_year) noexcept {
    if (const error e = detail::check_structure(s); e != error::none) return e;
    const unsigned yy = detail::digit(s[4]) * 10 + detail::digit(s[5]);
    if (yy != full_birth_year % 100) return error::invalid_date;
    return detail::check_semantics(s, detail::gregorian_leap(full_birth_year));
}

constexpr bool valid(std::string_view s) noexcept {
    return check(s) == error::none;
}

constexpr bool valid(std::string_view s, unsigned full_birth_year) noexcept {
    return check(s, full_birth_year) == error::none;
}

// The wristband: an amka::sin can only be obtained through parse(), therefore
// the existence of an instance IS the proof of structural validity.
// It proves nothing about real-world existence or assignment of the number.
class sin {
public:
    sin() = delete;  // no "empty" or "invalid" state exists, by construction

    constexpr date birth_date() const noexcept {
        return {d(0) * 10 + d(1), d(2) * 10 + d(3), d(4) * 10 + d(5)};
    }

    // Parity of the serial (equivalently: of its last digit).
    // This is the administrative encoding — see README before using it as fact.
    constexpr sex encoded_sex() const noexcept {
        return d(9) % 2 == 1 ? sex::male : sex::female;
    }

    constexpr unsigned serial() const noexcept {
        return d(6) * 1000 + d(7) * 100 + d(8) * 10 + d(9);
    }

    // View over the 11 digits. NOT null-terminated: print size-aware
    // (iostreams, std::string, "%.*s"), never through .data() with "%s".
    constexpr std::string_view str() const noexcept {
        return std::string_view{digits_.data(), digits_.size()};
    }

    // std::array's comparison operators are constexpr only since C++20,
    // so in a C++17 library the loops are written by hand.
    friend constexpr bool operator==(const sin& a, const sin& b) noexcept {
        for (std::size_t i = 0; i < 11; ++i)
            if (a.digits_[i] != b.digits_[i]) return false;
        return true;
    }
    friend constexpr bool operator!=(const sin& a, const sin& b) noexcept {
        return !(a == b);
    }
    friend constexpr bool operator<(const sin& a, const sin& b) noexcept {
        for (std::size_t i = 0; i < 11; ++i)
            if (a.digits_[i] != b.digits_[i]) return a.digits_[i] < b.digits_[i];
        return false;
    }
    friend constexpr bool operator>(const sin& a, const sin& b) noexcept { return b < a; }
    friend constexpr bool operator<=(const sin& a, const sin& b) noexcept { return !(b < a); }
    friend constexpr bool operator>=(const sin& a, const sin& b) noexcept { return !(a < b); }

    friend constexpr std::optional<sin> parse(std::string_view) noexcept;

private:
    constexpr explicit sin(std::string_view s) noexcept : digits_{} {
        for (std::size_t i = 0; i < 11; ++i) digits_[i] = s[i];
    }

    constexpr unsigned d(std::size_t i) const noexcept {
        return detail::digit(digits_[i]);
    }

    std::array<char, 11> digits_;  // 11 bytes, trivially copyable
};

// The only gate. Precondition-free: any string_view is a legal argument.
constexpr std::optional<sin> parse(std::string_view s) noexcept {
    if (check(s) != error::none) return std::nullopt;
    return sin{s};
}

} // namespace amka

// FNV-1a over the 11 digit bytes: makes amka::sin a first-class key for
// std::unordered_map / unordered_set without any user-side boilerplate.
namespace std {

template <>
struct hash<amka::sin> {
    std::size_t operator()(const amka::sin& s) const noexcept {
        std::size_t h = 14695981039346656037ull;   // FNV offset basis (64-bit)
        for (const char c : s.str()) {
            h ^= static_cast<unsigned char>(c);
            h *= 1099511628211ull;                 // FNV prime (64-bit)
        }
        return h;
    }
};

} // namespace std

#endif // AMKA_SIN_HPP_INCLUDED
