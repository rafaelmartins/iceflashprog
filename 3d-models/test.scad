// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: CERN-OHL-S-2.0

include <settings.scad>

color("orange") {
    import("base.stl");

    translate([0, width - thickness, height])
        rotate([180, 0, 0])
            import("cover.stl");
}
