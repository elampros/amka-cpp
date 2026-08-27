# amka-cpp

[![CI](https://github.com/elampros/amka-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/elampros/amka-cpp/actions/workflows/ci.yml)

Validation and parsing of the **Greek Social Insurance Number** (ΑΜΚΑ — Αριθμός Μητρώου Κοινωνικής Ασφάλισης), for C++.

Header-only. C++17. Zero dependencies. Everything `constexpr`.

```cpp
#include <amka/sin.hpp>

// Validation is pure integer arithmetic, so the compiler can be your test runner:
static_assert(amka::valid("01013099997"));
static_assert(amka::check("0101309999X") == amka::error::invalid_character);

// At runtime, parse once — then the type carries the proof:
std::optional<amka::sin> id = amka::parse(user_input);
if (id) {
    amka::date d  = id->birth_date();    // {day, month, year2}
    amka::sex  s  = id->encoded_sex();   // parity of the serial
    unsigned   n  = id->serial();        // 0..9999
    std::string_view digits = id->str(); // the 11 digits, not null-terminated
}
```

## What an AMKA is

An 11-digit identifier: `DDMMYY SSSS C`

| Part | Meaning |
|------|---------|
| `DDMMYY` | date of birth — **two-digit year, the century is not encoded** |
| `SSSS` | registry serial within that birth date; odd = male, even = female |
| `C` | [Luhn](https://en.wikipedia.org/wiki/Luhn_algorithm) check digit over the full number |

## API

Everything lives in `namespace amka`, in a single header `<amka/sin.hpp>`.

* `check(std::string_view) -> error` — diagnose, with documented precedence: `invalid_length` → `invalid_character` → `invalid_date` → `invalid_checksum`.
* `check(std::string_view, unsigned full_birth_year) -> error` — strict variant when the caller knows the actual birth year (see footgun #1).
* `valid(std::string_view) -> bool` — sugar for `check(s) == error::none` (same overload pair).
* `parse(std::string_view) -> std::optional<sin>` — the only way to obtain a `sin`.
* `class sin` — an immutable value type whose existence proves structural validity. Accessors: `birth_date()`, `encoded_sex()`, `serial()`, `str()`. Fully comparable, ordered lexicographically over its digits, and hashable (`std::hash<amka::sin>` is provided), so it is a first-class key for `std::set` and `std::unordered_map` alike.
* `to_string(error) -> std::string_view` — stable technical names for logs and CLIs. Human-facing, localized wording is deliberately your job.

All of the above is `constexpr` and `noexcept`. The library allocates nothing, throws nothing, and includes only `<array>`, `<cstdint>`, `<functional>`, `<optional>`, `<string_view>`.

## Footguns — read before shipping

**1. The century is not encoded, and February 29 knows it.**
`290200...` may be 29/02/1900 (not a leap year) or 29/02/2000 (a leap year). The default policy is *lenient*: a date is accepted if it exists in **at least one** candidate century, which mathematically collapses to `YY % 4 == 0`. If you know the real birth year, use the strict overload:

```cpp
static_assert(amka::valid("29020000013"));                                    // lenient
static_assert(amka::check("29020000013", 1900) == amka::error::invalid_date); // strict
```

**2. Validity is structural — never existential.**
`amka::valid` proves the string is *well-formed*: correct length, digits, plausible date, matching check digit. It does **not** prove the number has been issued, or that it belongs to any person. No offline library can prove that.

**3. `encoded_sex()` reports the encoding, not the person.**
The serial's parity is an administrative convention of the registry. Treat it as metadata of the *number*, and use it as a fact about a *person* only where that is actually warranted.

**4. `sin` vs `<cmath>`.**
The type is named after the domain term (Social Insurance Number). `<math.h>` puts a `sin` in the global namespace and implementations may do so via `<cmath>` too. Qualified use — `amka::sin` — is always unambiguous; in math-heavy translation units simply avoid `using namespace amka;`, advice that holds for any namespace.

Also by design: **no input normalization.** Exactly 11 digits — no spaces, no dashes. Trimming user input is presentation-layer work.

## Synthetic AMKA for tests — `<amka/testing.hpp>`

A second, **runtime-only** header (it pulls `<random>`, which the constexpr core deliberately never does):

```cpp
#include <amka/testing.hpp>

std::mt19937 g{42};                       // the caller owns the URBG => determinism by default

amka::sin a = amka::testing::make_sin(g);                                  // fully random
amka::sin b = amka::testing::make_sin(g, amka::date{15, 3, 90});           // fixed birth date
amka::sin c = amka::testing::make_sin(g, amka::date{15, 3, 90},
                                         amka::sex::female);               // + fixed parity

std::vector<amka::sin> fixtures = amka::testing::make_sins(g, 10'000);     // mutually distinct
auto same_day  = amka::testing::make_sins(g, 10'000, amka::date{1, 1, 70});          // = ALL serials
auto same_both = amka::testing::make_sins(g,  5'000, amka::date{1, 1, 70},
                                              amka::sex::male);                      // = every odd serial
```

Guarantees and caveats:

* **Every result goes through `amka::parse()`** — the generator holds no keys to the `sin` type; assembly bugs fail loudly at the same gate user input does.
* **Batches are distinct by construction.** Capacity is a hard precondition: 10 000 AMKA per birth date, 5 000 per (date, sex), 365 250 000 in total. Exceeding it is a pigeonhole violation, checked with `assert` (UB under `NDEBUG`, like standard-library preconditions).
* **Determinism is per standard library.** `std::uniform_int_distribution`'s algorithm is implementation-defined, so *same seed + same stdlib ⇒ same fixtures*; sequences differ across libstdc++/libc++/MSVC. A portable distribution is a candidate for a later version.
* **The output is fiction.** Structurally valid, never checked against any registry, and possibly coinciding with issued numbers — see Non-affiliation below.

## Installation

It is one header. Copy `include/amka/sin.hpp` into your tree, or:

```cmake
# as a subdirectory
add_subdirectory(amka-cpp)
target_link_libraries(your_target PRIVATE amka::amka)

# or via FetchContent
include(FetchContent)
FetchContent_Declare(amka GIT_REPOSITORY https://github.com/elampros/amka-cpp.git GIT_TAG v0.2.0)
FetchContent_MakeAvailable(amka)
target_link_libraries(your_target PRIVATE amka::amka)
```

## Testing

Three suites, one philosophy:

* `tests/compile_time_tests.cpp` — `static_assert`s only. **Compiling it is running it.** The corpus of valid numbers is borrowed from the test suites of the neighbouring ecosystems (greecejs, zoispag's Python/Rust/Nim trilogy, spapas' Elixir localflavor) so all implementations stay mutually honest.
* `tests/runtime_tests.cpp` — Catch2 (dev-only dependency, fetched by CMake, never shipped). Covers what `static_assert` cannot: `std::hash` behaviour, container usage, and the Luhn uniqueness property (for any 10-digit prefix, exactly one check digit validates).
* `tests/generator_tests.cpp` — Catch2 as well. Exercises `<amka/testing.hpp>`: determinism per seed, batch distinctness, and constraint honouring (fixed date / fixed sex).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

Tests default to `-std=c++17` with extensions off, so any accidental C++20-ism (say, a `constexpr` `std::array` comparison) fails loudly instead of silently raising the floor. CI additionally builds them under C++20 and C++23 (`-DAMKA_TEST_CXX_STANDARD=20|23`) across GCC, Clang, and MSVC.

## Roadmap

* **v0.2** — ✅ shipped: `<amka/testing.hpp>`, the deterministic synthetic-AMKA generator described above.
* **v0.3** — ✅ CI matrix (GCC/Clang/MSVC × C++17/20/23); still open: install rules + `find_package` config, possibly a portable integer distribution for cross-stdlib reproducibility.

## License

[MIT](LICENSE).

## Non-affiliation

This is an independent open-source project. It is not affiliated with, or endorsed by, ΗΔΙΚΑ, e-ΕΦΚΑ, or any Greek government body. Synthetic numbers produced by the testing header are structurally valid by construction and may coincidentally match issued numbers — there is no reserved "test range" for AMKA.
