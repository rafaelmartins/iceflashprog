// SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
// SPDX-License-Identifier: CERN-OHL-S-2.0

thickness = 1.6;

devboard_pcb_height = 1.6;
devboard_width = 23.114;
devboard_length = 51.943;
devboard_screw_padding_front_x = 7.874;
devboard_screw_padding_back_x = 7.366;
devboard_screw_padding_y = 2.921;
devboard_screw_distance_y = 2 * 8.636;
devboard_screw_distance_x = 36.703;
devboard_screw_base_d = 5;
devboard_screw_base_h = 7.5;
devboard_distance_usb = devboard_screw_padding_y + 8.636;
devboard_distance_led = devboard_screw_padding_y + 1.524;
devboard_pcb_padding_x = thickness + 1;
devboard_pcb_padding_y = thickness + 1;

length = devboard_length + 8.5 + 2 * devboard_pcb_padding_x;
width = devboard_width + 2 * devboard_pcb_padding_y;
height = thickness + devboard_screw_base_h + devboard_pcb_height + 5;

led_d = 3.2;
usb_height = 4.4;
usb_width = 8.2;

flatcable_distance = 21;
flatcable_width = 13;
flatcable_height = 0.95;
