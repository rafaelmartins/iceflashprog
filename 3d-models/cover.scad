// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: CERN-OHL-S-2.0

include <lib/simple-case.scad>
include <settings.scad>

difference() {
    union() {
        simple_case_cover (length, width, height, thickness=thickness, gap=0);

        translate([devboard_pcb_padding_x + flatcable_distance, -thickness, 0])
            cube([flatcable_width, thickness,
                  height - thickness - devboard_screw_base_h - devboard_pcb_height - flatcable_height]);
    }

    translate([0, devboard_pcb_padding_y - thickness + devboard_distance_usb - usb_width / 2,
               height - thickness - devboard_screw_base_h - 0.2])
        cube([thickness, usb_width, usb_height]);
    translate([0, devboard_pcb_padding_y - thickness + devboard_distance_led,
               height - thickness - devboard_screw_base_h + led_d / 2])
        rotate([-90, 0, -90])
            cylinder(h=thickness, d=led_d, $fn=20);
}
