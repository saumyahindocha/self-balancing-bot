# Chassis plates

Three-tier acrylic chassis based on Section 4.1 of the IIT Delhi reference
report: three identical plates stacked on four threaded rods.

| File | Plate | Holes |
|---|---|---|
| `bottom_plate.svg` | Bottom: motors, IR sensors | 4 x Ø6.5 mm rod holes, 3 x Ø3 mm IR sensor holes |
| `middle_plate.svg` | Middle: Arduino Mega, L298N | 4 x Ø6.5 mm rod holes |
| `top_plate.svg` | Top: 3S LiPo | 4 x Ø6.5 mm rod holes |

**Dimensions**
- Plates are 230 x 100 mm, cut from 6 mm acrylic.
- Rod holes are Ø6.5 mm (for M6 rods plus 0.5 mm clearance), 15 mm in from each edge.
- The IR sensor holes are Ø3 mm at 30 mm pitch, centred, 10 mm from the front edge.
- The SVGs are drawn 1:1 in millimetres (`viewBox` units = mm). They contain cut lines only.

**Why 230 mm:** the reference report isn't consistent. Section 4.1 says
230 x 100 x 6 mm. Section 4.6.1 mentions a revised 260 x 100 x 4 mm plate,
and the drawings (Figs. 26-27) show 250 mm. These files use Section 4.1, the
only set of dimensions the report states explicitly.

**Not included, drill to fit:** mounting holes for the motor brackets, the
Arduino Mega, the L298N, and the battery strap slots. The report doesn't give
these, so they were left for measuring against the real parts.

**Using the files**
- In Fusion 360, use Insert > Insert SVG on a sketch, then check that an edge
  measures 230 mm before extruding 6 mm.
- Laser cutters need 2D vectors (SVG/DXF), not STL. Ask the shop about kerf
  compensation (about 0.1-0.2 mm).

The plates were laser-cut on 24 Jun 2026. The chat doesn't record whether the
cut used these exact files or a version with the extra mounting holes added.
