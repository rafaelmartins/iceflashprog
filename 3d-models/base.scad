// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: CERN-OHL-S-2.0

include <lib/simple-case.scad>
include <settings.scad>

difference() {
    union() {
        simple_case_base(length, width, height, thickness=thickness, gap=0);

        for (i=[0:1])
            for (j=[0:1])
                translate([devboard_pcb_padding_x + devboard_screw_padding_front_x + i * devboard_screw_distance_x,
                           devboard_pcb_padding_y + devboard_screw_padding_y + j * devboard_screw_distance_y,
                           thickness])
                    cylinder(h=devboard_screw_base_h, d=devboard_screw_base_d, $fn=20);
    }

    for (i=[0:1])
        for (j=[0:1])
            translate([devboard_pcb_padding_x + devboard_screw_padding_front_x + i * devboard_screw_distance_x,
                       devboard_pcb_padding_y + devboard_screw_padding_y + j * devboard_screw_distance_y,
                       thickness])
                cylinder(h=devboard_screw_base_h, d=1.8, $fn=20);

    translate([devboard_pcb_padding_x + flatcable_distance,
               width - thickness, thickness + devboard_screw_base_h + devboard_pcb_height])
        cube([flatcable_width, thickness, height - thickness - devboard_screw_base_h - devboard_pcb_height]);
}
