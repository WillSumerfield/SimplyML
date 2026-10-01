// Scratch consumer of an installed SimplyML: links the umbrella target and loads the embedded font.
#include <simplyml/core/default_font.hpp>
#include <simplyml/util/format.hpp>
#include <simplyml/util/ring_buffer.hpp>

#include <SFML/Graphics/Font.hpp>

#include <cstdio>

int main()
{
    auto const font_data = sml::defaultFontData();
    sf::Font   font;
    if (!font.openFromMemory(font_data.data, font_data.size)) {
        std::puts("failed to load default font");
        return 1;
    }

    sml::RingBuffer<float> rb{4};
    for (int i = 0; i < 10; ++i) {
        rb.push(static_cast<float>(i));
    }
    std::printf("font '%s', mean %s\n", font.getInfo().family.c_str(), sml::toString(rb.mean()).c_str());
    return rb.mean() == 7.5f ? 0 : 1;
}
