# Formato de proyecto `.xeldi` (versión 1.0)

Contenedor binario little-endian: cabecera fija de 32 bytes, bloques (*chunks*) con CRC y un índice al
final. Los metadatos van en JSON legible; los pixeles y vóxeles, comprimidos con zlib.

> Se llama `.xeldi` (y no `.pf3d`) para que el sistema reconozca el archivo por su nombre y por la firma `XELD`
> (ver `packaging/linux/xeldi-mime.xml`). La estructura sigue el borrador original con dos ajustes: la cabecera
> apunta al **índice** en lugar de solo a `META` (así se puede saltar a cualquier bloque, incluida la miniatura),
> y cada bloque lleva su propio códec y CRC.

## Cabecera (32 bytes)

| Offset | Tamaño | Tipo | Campo | Descripción |
|---|---|---|---|---|
| 0x00 | 4 | char[4] | magic | `"XELD"` |
| 0x04 | 2 | u16 | version_major | `1` |
| 0x06 | 2 | u16 | version_minor | `0` |
| 0x08 | 4 | u32 | flags | 1 = tiene 3D, 2 = bloques comprimidos, 4 = tiene miniatura |
| 0x0C | 8 | u64 | index_offset | posición del bloque `IDX_` |
| 0x14 | 8 | u64 | index_size | tamaño del bloque `IDX_` |
| 0x1C | 4 | u32 | crc32 | CRC-32 de los bytes 0x00–0x1B |

## Bloque genérico

```
tag[4] | u32 codec (0 = crudo, 1 = zlib, 2 = reservado zstd) | u64 tamaño_original | u64 tamaño_guardado | u32 crc32(datos guardados) | datos
```

Orden en el archivo: `META`, `C2D_`, `WPLN`, `VX3D`, `THMB` (opcional), `IDX_`. Un lector debe usar el índice
y **ignorar bloques desconocidos**.

### `META` (JSON UTF-8)

```json
{
  "project":   { "name": "Conejito", "unit_scale": 1.0, "palette_name": "Xeldi Pastel", "palette": ["#00000000", "#5A3D52FF"] },
  "canvas_2d": { "width": 32, "height": 32, "pixel_format": "RGBA8" },
  "timeline":  { "fps": 12, "frame_count": 4,
                 "frames": [ { "index": 0, "duration_ms": 220 } ],
                 "tags": [ { "name": "idle", "from": 0, "to": 3, "loop_mode": "forward",
                             "fps_override": 0, "auto_preview": true, "color": "#F29BB8FF" } ] },
  "onion_skin": { "enabled": true, "prev_frames": 2, "next_frames": 1, "prev_tint": "#F2709BFF",
                  "next_tint": "#7FA2F5FF", "base_opacity": 0.35, "falloff": 0.65,
                  "only_current_layer": false, "loop_within_tag": true },
  "layers": [ { "id": "layer_1", "name": "Sombra", "type": "raster2d", "visible": true, "locked": false,
                "opacity": 0.7, "blend_mode": "normal", "parent_id": null } ],
  "scene_3d": { "chunk_size": [16, 16, 16],
                "camera": { "position": [..], "target": [0, 14, 2], "yaw_deg": 30, "pitch_deg": 20,
                            "distance": 70, "fov_deg": 45, "ortho": false },
                "workplanes": [ { "id": "plane_floor", "name": "Piso", "origin": [-16, 0, 16], "normal": [0, 1, 0],
                                  "right": [1, 0, 0], "up": [0, 0, -1], "width_px": 32, "height_px": 32,
                                  "snap_to_grid": true, "visible": true } ],
                "active_workplane": 0 }
}
```

Las capas se referencian por `id` estable; el índice de capa en los bloques binarios es su posición en `layers`.

### `C2D_` — cels 2D

```
u32 n
n × ( u32 capa | u32 frame | u32 flags (0) | width*height*4 bytes RGBA8 )
```

Las cels vacías o totalmente transparentes no se guardan.

### `WPLN` — bitmaps de los planos de trabajo

```
u32 n
n × ( u32 ancho | u32 alto | ancho*alto*4 bytes RGBA8 )     // mismo orden que META.scene_3d.workplanes
```

### `VX3D` — vóxeles

```
u32 n_chunks
n × ( i32 cx | i32 cy | i32 cz | i32 frame (-1 = estático) | u8 modo | datos )
  modo 0 (disperso): u16 k; k × (u8 lx | u8 ly | u8 lz | u32 rgba 0xRRGGBBAA | u8 flags)
  modo 1 (denso):    4096 × (u32 rgba | u8 flags), índice = (lz*16 + ly)*16 + lx
```

Se usa el modo denso cuando más de 1/6 del chunk está ocupado.

### `THMB` — miniatura

PNG (frame 0 compuesto, máx. 128 px, escalado al vecino más cercano). Lo usan los diálogos de abrir.

### `IDX_` — índice

```
u32 n
n × ( tag[4] | u64 offset | u64 tamaño )
```

## Reglas de evolución

- Versión menor: solo se **agregan** campos o bloques; al leer se usan valores por defecto.
- Versión mayor distinta: el lector la rechaza con un mensaje claro.
- Guardado atómico: se escribe `archivo.xeldi.tmp`, se sincroniza a disco, se verifica el tamaño y se renombra.
