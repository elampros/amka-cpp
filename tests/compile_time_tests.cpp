// Compilation of this translation unit IS the test suite.
// If it builds, every assertion below was proven by the compiler.

#include <amka/sin.hpp>

namespace {

using amka::check;
using amka::error;
using amka::valid;

// ---- Corpus: valid AMKAs published in other ecosystems' test suites --------
static_assert(valid("01013099997"));   // greecejs/greece-amka
static_assert(valid("09095986684"));   // zoispag/amka-{py,rs,nim}
static_assert(valid("21068302674"));   // spapas/localflavor-gr
static_assert(valid("20108201821"));   // spapas/localflavor-gr
static_assert(valid("24058202672"));   // spapas/localflavor-gr

// ---- Structural errors, in documented precedence ----------------------------
static_assert(check("")             == error::invalid_length);
static_assert(check("0101309999")   == error::invalid_length);     // 10 digits
static_assert(check("010130999971") == error::invalid_length);     // 12 digits
static_assert(check("0101309999X")  == error::invalid_character);
static_assert(check("01013O99997")  == error::invalid_character);  // capital O, not zero
static_assert(check(" 1013099997")  == error::invalid_character);  // no normalization, by design

// ---- Date errors -------------------------------------------------------------
static_assert(check("32120012345") == error::invalid_date);   // day 32
static_assert(check("00120012345") == error::invalid_date);   // day 00
static_assert(check("01130012345") == error::invalid_date);   // month 13
static_assert(check("01000012345") == error::invalid_date);   // month 00
static_assert(check("31040012345") == error::invalid_date);   // April has 30 days
static_assert(check("29020112345") == error::invalid_date);   // '01: no candidate century is leap

// ---- Checksum ------------------------------------------------------------------
static_assert(check("09095986680") == error::invalid_checksum);  // zoispag's canonical bad digit
static_assert(check("01013099998") == error::invalid_checksum);

// ---- Century policy: the README footgun, as an executable spec -----------------
static_assert(valid("29020000013"));                              // lenient: 29/02/'00 exists in 2000
static_assert(check("29020000013", 2000) == error::none);         // strict agrees for 2000...
static_assert(check("29020000013", 1900) == error::invalid_date); // ...and rejects for 1900
static_assert(check("01013099997", 1931) == error::invalid_date); // strict: YY mismatch (30 vs 31)
static_assert(check("01013099997", 2030) == error::none);

// ---- The wristband, worn at compile time ---------------------------------------
static_assert(amka::parse("29020400007").has_value());
static_assert(!amka::parse("29020400008").has_value());

constexpr amka::sin leap04 = *amka::parse("29020400007");  // 29/02/'04, serial 0000
static_assert(leap04.birth_date().day == 29);
static_assert(leap04.birth_date().month == 2);
static_assert(leap04.birth_date().year2 == 4);
static_assert(leap04.serial() == 0);
static_assert(leap04.encoded_sex() == amka::sex::female);
static_assert(leap04.str() == "29020400007");

constexpr amka::sin male00 = *amka::parse("29020000013");  // 29/02/'00, serial 0001
static_assert(male00.encoded_sex() == amka::sex::male);
static_assert(male00.serial() == 1);

static_assert(male00 != leap04);
static_assert(leap04 == *amka::parse("29020400007"));
static_assert((leap04 < male00) == (leap04.str() < male00.str()));  // ordering == lexicographic

// ---- to_string -------------------------------------------------------------------
static_assert(amka::to_string(error::none) == "none");
static_assert(amka::to_string(error::invalid_checksum) == "invalid_checksum");

} // namespace

int main() { return 0; }  // if it compiled, it already passed
