# Tasks

[  ] Loading textures

[OK] Chunks

[OK] Sort render commands back-to-front (for non-transparent objects) to avoid redraw

[OK] Collapse contiguous cubes into less geometry

[OK] Fix weird visual chunk bugs (empty chunks, etc)

[  ] Make asset manager thread-safe

[  ] Write `RenderModel` component (like an `IndexedRenderMesh` but having an array of meshes).
     This will remove the pain of needing multiple entities per chunk (one for each material mesh).
     Also, we will then pack the three meshes in a single VBO and appease the driver demons.

[  ] Load config from json/lua/ini files

[  ] Make render commands also hold underlying buffer size