# Tasks

[  ] Loading textures

[OK] Chunks

[  ] Sort render commands back-to-front (for non-transparent objects) to avoid redraw

[  ] Collapse contiguous cubes into less geometry


/////////////////

Note: have found that some visual bugs (empty chunks, etc) are caused by our cache mechanism.
When disabling cache eviction and chunk recycling, all chunks look good.