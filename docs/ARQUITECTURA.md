# Arquitectura de Xeldi

## Capas del sistema

```
            ┌──────────────────────────── app/ (Qt 6) ─────────────────────────────┐
            │ MainWindow · CanvasView2D · Viewport3D · Capas · Línea de tiempo ·    │
            │ Paleta · Preview idle · Escena 3D · Tema pastel                       │
            └───────────────▲───────────────────────────────▲──────────────────────┘
                            │ llamadas + PointerInput        │ notificaciones (ChangeFlags)
            ┌───────────────┴──────────── editor/ ───────────┴──────────────────────┐
            │ Editor: documento, historial, estado (frame, capa, color, herramienta), │
            │ trazos 2D/3D, operaciones de alto nivel, ActionRegistry                 │
            └──────▲──────────────────▲───────────────────▲───────────────▲─────────┘
                   │                  │                   │               │
            ┌──────┴─────┐   ┌────────┴────────┐  ┌───────┴──────┐  ┌─────┴─────┐
            │  engine2d  │   │    engine3d     │  │      io      │  │   core    │
            │ composición│   │ cámara, picking,│  │ .xeldi, PNG, │  │  modelo,  │
            │ pinceles,  │   │ render software,│  │ OBJ, sprite- │  │ comandos, │
            │ onion,     │   │ extrusión, vistas│ │ sheet        │  │ acciones  │
            │ animación  │   │                 │  │              │  │           │
            └────────────┘   └─────────────────┘  └──────────────┘  └───────────┘
```

Regla: **solo `app/` conoce Qt.** Todo lo demás es C++20 estándar + zlib + nlohmann-json, y se compila como
la librería estática `xeldi_engine`. Una futura interfaz para tablets reutiliza `xeldi_engine` completo.

## Decisiones principales

- **Escala única.** 1 unidad del mundo = 1 vóxel = 1 pixel del lienzo. Eje Y hacia arriba. El lienzo se
  mapea al mundo con la columna central en x = 0 y la fila de abajo en y = 0 (ver `engine3d/ViewProjection.hpp`).
- **Vóxeles dispersos.** `VoxelGrid` guarda chunks de 16³ en un `unordered_map`; un chunk que queda vacío se libera.
- **Planos de trabajo.** Cada `Workplane` tiene origen, normal, ejes derecha/arriba y su propio bitmap RGBA.
  Pintar en 3D = intersectar el rayo de la cámara con el plano y redondear al pixel.
- **Deshacer barato.** Los trazos 2D guardan solo el rectángulo que cambió (`PixelPatchCommand`); los cambios 3D
  guardan solo los vóxeles tocados (`VoxelEditCommand`); los cambios estructurales copian punteros compartidos
  a las cels, no los pixeles (`StructureCommand`). El historial tiene límite de pasos y de memoria.
- **Acciones con nombre.** `ActionRegistry` (core) es la única fuente de acciones: id, texto, atajo portable,
  icono, si está activa/marcada. La UI de escritorio genera sus `QAction` desde ahí.
- **Entrada independiente del dispositivo.** `PointerInput {x, y, pressure, primary}`; la UI traduce ratón,
  lápiz o dedos a esto. Nada en el motor depende de hover, botón central o teclado.
- **Render 3D por software.** Rasterizador con z-buffer en `engine3d`. Evita problemas de contextos OpenGL entre
  X11/Wayland/controladores, funciona en pruebas automáticas y sirve para miniaturas. La vista usa pixeles de 2×
  por defecto (look retro y 4× menos trabajo). Un backend GPU se puede añadir detrás de la misma interfaz.

## Flujo de un trazo 2D

1. `CanvasView2D` convierte el evento a coordenadas del lienzo → `Editor::beginStroke2D(PointerInput)`.
2. El editor crea un `PixelStroke` (copia temporal de la cel) y aplica la herramienta.
3. Cada movimiento dibuja con Bresenham (y filtro *pixel perfect*); las figuras se previsualizan restaurando el rectángulo sucio.
4. Al soltar, `PixelStroke::finish` recorta a los pixeles que cambiaron y produce un `PixelPatchCommand` para el historial.
5. El editor notifica `ChangePixels`; los paneles interesados se refrescan.
