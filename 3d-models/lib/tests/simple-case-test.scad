// SPDX-FileCopyrightText: 2024 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: CERN-OHL-S-2.0

include <../simple-case.scad>

width = 40;
length = 60;
height = 20;
thickness = 1.6;
gap = 0.2;


simple_case_base(width, length, thickness=thickness, height, gap=gap);

translate([width, thickness + gap, height])
    rotate([0, 180, 0])
        simple_case_cover(width, length, height, thickness=thickness, gap=gap);
