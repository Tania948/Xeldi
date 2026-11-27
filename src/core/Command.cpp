#include "core/Command.hpp"

namespace xeldi {

void History::push(std::unique_ptr<Command> cmd) {
    if (!cmd) return;
    if (cleanIndex_ > long(undo_.size())) cleanIndex_ = -1;  // el estado limpio estaba en redo
    redo_.clear();
    undo_.push_back(std::move(cmd));
    trim();
}

unsigned History::undo(Document& doc) {
    if (undo_.empty()) return ChangeNone;
    auto cmd = std::move(undo_.back());
    undo_.pop_back();
    cmd->undo(doc);
    unsigned f = cmd->changes();
    redo_.push_back(std::move(cmd));
    return f | ChangeHistory;
}

unsigned History::redo(Document& doc) {
    if (redo_.empty()) return ChangeNone;
    auto cmd = std::move(redo_.back());
    redo_.pop_back();
    cmd->redo(doc);
    unsigned f = cmd->changes();
    undo_.push_back(std::move(cmd));
    return f | ChangeHistory;
}

void History::clear() {
    undo_.clear();
    redo_.clear();
    cleanIndex_ = 0;
}

std::size_t History::memoryBytes() const {
    std::size_t n = 0;
    for (const auto& c : undo_) n += c->memoryBytes();
    for (const auto& c : redo_) n += c->memoryBytes();
    return n;
}

void History::trim() {
    while (undo_.size() > 1 && (undo_.size() > maxSteps || memoryBytes() > maxBytes)) {
        undo_.erase(undo_.begin());
        --cleanIndex_;
        if (cleanIndex_ < 0) cleanIndex_ = -1000000;  // ya no es alcanzable
    }
}

}  // namespace xeldi
