#pragma once
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>

// Helper-e minimale pentru serializare binara.
//
// Nota: formatul e host-endian si host-size pentru tipurile POD. E corect cat
// timp citesti indexul pe aceeasi arhitectura pe care l-ai scris (cazul normal:
// il construiesti si il folosesti pe acelasi calculator). Un format portabil ar
// codifica numere pe latime fixa, little-endian explicit -- lasat ca imbunatatire.

template <typename T>
void write_pod(std::ostream& out, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "doar tipuri POD");
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
T read_pod(std::istream& in) {
    static_assert(std::is_trivially_copyable<T>::value, "doar tipuri POD");
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

// String = lungime (uint32) urmata de bytes.
inline void write_string(std::ostream& out, const std::string& s) {
    write_pod<uint32_t>(out, static_cast<uint32_t>(s.size()));
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
}

inline std::string read_string(std::istream& in) {
    uint32_t n = read_pod<uint32_t>(in);
    std::string s(n, '\0');
    if (n > 0) in.read(&s[0], static_cast<std::streamsize>(n));
    return s;
}
