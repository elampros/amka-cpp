// amka-check: validate an AMKA from the command line.
//
//   $ amka-check 01013099997
//   OK  born 1/1/'30  serial 9999  (M)
//
//   $ amka-check 09095986680
//   FAIL  invalid_checksum

#include <amka/sin.hpp>

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: amka-check <AMKA>\n";
        return 2;
    }

    const amka::error e = amka::check(argv[1]);
    if (e != amka::error::none) {
        // to_string returns a string_view: print size-aware (iostreams do),
        // never through .data() with "%s" — the view is not null-terminated.
        std::cout << "FAIL  " << amka::to_string(e) << '\n';
        return 1;
    }

    const amka::sin id = *amka::parse(argv[1]);  // safe: check() said none
    const amka::date d = id.birth_date();

    std::cout << "OK  born " << d.day << '/' << d.month << "/'"
              << (d.year2 < 10 ? "0" : "") << d.year2   // two-digit year, no century guess
              << "  serial " << id.serial()
              << (id.encoded_sex() == amka::sex::male ? "  (M)" : "  (F)")
              << '\n';
    return 0;
}
