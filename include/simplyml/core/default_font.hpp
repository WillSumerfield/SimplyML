#pragma once
#include <cstddef>

namespace sml
{

struct ByteView
{
    unsigned char const* data;
    std::size_t          size;
};

/// Bytes of the embedded default font (Share Tech Mono, SIL OFL 1.1), for `sf::Font::openFromMemory`.
ByteView defaultFontData();

} // namespace sml
