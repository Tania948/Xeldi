#pragma once
// Acciones con nombre ("edit.undo", "onion.toggle", "anim.play_preview"...).
// La UI de escritorio las convierte en QAction con atajos; una futura UI tactil
// las puede conectar a gestos o botones grandes sin tocar el motor.
#include <functional>
#include <string>
#include <vector>

namespace xeldi {

struct ActionInfo {
    std::string id;        ///< "categoria.nombre"
    std::string label;     ///< texto visible (espanol)
    std::string shortcut;  ///< formato portable: "Ctrl+Z", "Shift+O", "1"
    std::string icon;      ///< nombre de icono del tema (opcional)
    bool checkable = false;
};

struct Action {
    ActionInfo info;
    std::function<void()> trigger;
    std::function<bool()> enabled;  ///< vacio = siempre habilitada
    std::function<bool()> checked;  ///< solo si checkable
};

class ActionRegistry {
public:
    Action& add(ActionInfo info, std::function<void()> trigger, std::function<bool()> enabled = {},
                std::function<bool()> checked = {});
    /// Ejecuta una accion por id. false si no existe o esta deshabilitada.
    bool trigger(const std::string& id);
    const Action* find(const std::string& id) const;
    bool isEnabled(const std::string& id) const;
    bool isChecked(const std::string& id) const;
    const std::vector<Action>& all() const { return actions_; }

private:
    std::vector<Action> actions_;
};

}  // namespace xeldi
