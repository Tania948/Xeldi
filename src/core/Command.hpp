#pragma once
// Sistema de comandos: cada accion que modifica el documento es un Command
// reversible. El historial guarda solo diferencias (rectangulos sucios en 2D,
// voxeles cambiados en 3D) para no copiar el proyecto entero en cada trazo.
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace xeldi {

class Document;

/// Que partes del documento cambiaron (para que la UI sepa que refrescar).
enum ChangeFlags : unsigned {
    ChangeNone = 0,
    ChangePixels = 1u << 0,
    ChangeLayers = 1u << 1,
    ChangeFrames = 1u << 2,
    ChangeScene3D = 1u << 3,
    ChangePalette = 1u << 4,
    ChangeSettings = 1u << 5,   ///< onion skin, fps, tags...
    ChangeSelection = 1u << 6,  ///< frame/capa/color/herramienta actual (no undoable)
    ChangeDocument = 1u << 7,   ///< documento nuevo/abierto
    ChangeHistory = 1u << 8,
    ChangeAll = 0xFFFFu,
};

class Command {
public:
    virtual ~Command() = default;
    virtual std::string label() const = 0;
    virtual void undo(Document& doc) = 0;
    virtual void redo(Document& doc) = 0;
    virtual unsigned changes() const { return ChangeAll; }
    virtual std::size_t memoryBytes() const { return 64; }
};

class History {
public:
    /// Registra un comando que YA fue aplicado al documento.
    void push(std::unique_ptr<Command> cmd);
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    /// Devuelve las banderas de cambio (0 si no habia nada que deshacer).
    unsigned undo(Document& doc);
    unsigned redo(Document& doc);
    std::string undoLabel() const { return canUndo() ? undo_.back()->label() : std::string(); }
    std::string redoLabel() const { return canRedo() ? redo_.back()->label() : std::string(); }
    void clear();
    std::size_t undoCount() const { return undo_.size(); }
    std::size_t redoCount() const { return redo_.size(); }
    std::size_t memoryBytes() const;

    void markClean() { cleanIndex_ = long(undo_.size()); }
    bool isClean() const { return cleanIndex_ == long(undo_.size()); }

    std::size_t maxSteps = 512;
    std::size_t maxBytes = std::size_t(512) * 1024 * 1024;

private:
    void trim();
    std::vector<std::unique_ptr<Command>> undo_;
    std::vector<std::unique_ptr<Command>> redo_;
    long cleanIndex_ = 0;
};

}  // namespace xeldi
