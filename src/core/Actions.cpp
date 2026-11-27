#include "core/Actions.hpp"

namespace xeldi {

Action& ActionRegistry::add(ActionInfo info, std::function<void()> trigger, std::function<bool()> enabled,
                            std::function<bool()> checked) {
    for (auto& a : actions_)
        if (a.info.id == info.id) {
            a = Action{std::move(info), std::move(trigger), std::move(enabled), std::move(checked)};
            return a;
        }
    actions_.push_back(Action{std::move(info), std::move(trigger), std::move(enabled), std::move(checked)});
    return actions_.back();
}

const Action* ActionRegistry::find(const std::string& id) const {
    for (const auto& a : actions_)
        if (a.info.id == id) return &a;
    return nullptr;
}

bool ActionRegistry::isEnabled(const std::string& id) const {
    const Action* a = find(id);
    return a && (!a->enabled || a->enabled());
}

bool ActionRegistry::isChecked(const std::string& id) const {
    const Action* a = find(id);
    return a && a->checked && a->checked();
}

bool ActionRegistry::trigger(const std::string& id) {
    const Action* a = find(id);
    if (!a || (a->enabled && !a->enabled()) || !a->trigger) return false;
    auto fn = a->trigger;  // copia: la accion podria re-registrarse durante la llamada
    fn();
    return true;
}

}  // namespace xeldi
