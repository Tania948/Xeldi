#include "core/Palette.hpp"

namespace xeldi {

Palette Palette::xeldiDefault() {
    Palette p;
    p.name = "Xeldi Pastel";
    const char* hex[] = {
        "#00000000",                                                   // transparente
        "#3B2A3A", "#5A3D52", "#775E54", "#A88A7D", "#C8AD9C",         // ciruela / cafes
        "#E2D3C7", "#F3EEEC", "#FFFFFF",                               // cremas
        "#B15B64", "#E86A92", "#F29BB8", "#E6B3B8", "#F7D6E0",         // rosas
        "#FFD8BE", "#FFEEDD",                                          // durazno
        "#7C5FB8", "#A775C9", "#C9A8E8", "#E3D4F7",                    // morados
        "#5B6FC2", "#8B9BE0", "#B8C4FF", "#DDE3FF",                    // azules
        "#6FB8A0", "#A8E0C8", "#EAF7EC",                               // verdes menta
        "#F2C94C", "#FFF1B8",                                          // amarillos
    };
    for (const char* h : hex) p.colors.push_back(*fromHex(h));
    return p;
}

}  // namespace xeldi
