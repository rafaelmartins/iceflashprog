// SPDX-FileCopyrightText: 2024 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: CERN-OHL-S-2.0

module simple_case_base(width, length, height, thickness=1.6, screw_d=2, screw_p=2, gap=0) {
    difference() {
        union() {
            cube([width, length, thickness]);
            cube([width, thickness, height]);
            translate([0, length - thickness, 0])
                cube([width, thickness, height]);
        }

        translate([(thickness - gap) / 2, thickness, (thickness - gap) / 2])
            cube([gap + thickness / 2, length - 2 * thickness, gap + thickness / 2]);
        translate([width - thickness - gap / 2, thickness, (thickness - gap) / 2])
            cube([gap + thickness / 2, length - 2 * thickness, gap + thickness / 2]);

        for (i=[0:1])
            for (j=[0:1])
                translate([i * (width - screw_d - screw_p) + (screw_d + screw_p) / 2,
                           j * (length - thickness),
                           height - (screw_d + screw_p) / 2])
                    rotate([-90, 0, 0])
                        cylinder(h=thickness, d=screw_d * 1.1, $fn=20);
    }
}

module simple_case_cover(width, length, height, thickness=1.6, screw_d=2, screw_p=2, screw_h=6, gap=0) {
    difference() {
        union() {
            cube([width, length - 2 * (thickness + gap), thickness]);
            cube([thickness, length - 2 * (thickness + gap), height - thickness - gap]);
            translate([width - thickness, 0, 0])
                cube([thickness, length - 2 * (thickness + gap), height - thickness - gap]);

            for (i=[0:1])
                for (j=[0:1])
                    translate([i * (width - screw_d - screw_p),
                               j * (length - 2 * (thickness + gap) - screw_h - screw_p), 0])
                        cube([screw_d + screw_p, screw_h + screw_p, screw_d + screw_p]);

            translate([thickness / 2, thickness, height - thickness - gap])
                cube([thickness / 2, length - 4 * thickness - 2 * gap, thickness / 2]);
            translate([width - thickness, thickness, height - thickness - gap])
                cube([thickness / 2, length - 4 * thickness - 2 * gap, thickness / 2]);
        }

        for (i=[0:1])
            for (j=[0:1])
                translate([i * (width - screw_d - screw_p) + (screw_d + screw_p) / 2,
                           j * (length - 2 * (thickness + gap) - screw_h),
                           (screw_d + screw_p) / 2])
                    rotate([-90, 0, 0])
                        cylinder(h=screw_h, d=screw_d * 0.9, $fn=20);
    }
}
