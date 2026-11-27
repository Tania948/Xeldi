#pragma once
#include <string>
#include <vector>

#include "core/Color.hpp"

namespace xeldi {

struct Palette {
    std::string name;
    std::vector<Rgba> colors;

    /// Paleta por defecto de Xeldi: rosas, cafes suaves, azules y morados pastel
    /// y algunos tonos de contraste para contornos.
    static Palette xeldiDefault();
};

}  // namespace xeldi
