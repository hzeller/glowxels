#!/usr/bin/env -S awk -f

function diode_pos(str) {
    kStartX = 50;
    kStartY = 45;
    kDeltaX = 1.6;
    kDeltaY = 3.2;
    num = int(substr(str, 3));   # starts with quote: "D1
    block = int((num - 1) / 16);
    block_led = (num - 1) % 16;

    if (block_led < 8) {
	xpos = kStartX + (block * 8 + block_led) * kDeltaX;
	ypos = kStartY;
	return sprintf("\t\t(at %.1f %.1f -90)", xpos, ypos);
    } else {
	xpos = kStartX + (block * 8 + (15 - block_led) + 0.5) * kDeltaX;
	ypos = kStartY + kDeltaY;
	return sprintf("\t\t(at %.1f %.1f 90)", xpos, ypos);
    }
}

function label_pos(str) {
    num = int(substr(str, 3));   # starts with quote: "D1
    block_led = (num - 1) % 16;

    if (block_led < 8) {
	return sprintf("\t\t\t(at -4.5 0.1 -90)");
    } else {
	return sprintf("\t\t\t(at -1.5 0.1  90)");
    }
}

/\(footprint "LED_SMD/ { ACTIVE=1; }

/\(at / {
    if (!ACTIVE) print $0;    # Defer printing at until we see referece
    next;
}

/"Reference"/ {
    if (ACTIVE) {
	print(diode_pos($3));  # Diode position.
	print($0);
	print(label_pos($3));  # silk screen position of reference.
	next;
    }
}

/\(justify/ {
    if (ACTIVE) {
	print("\t\t\t\t(justify left mirror)");
	ACTIVE=0;              # This was the last thing to change.
	next;
    }
}

{
    print $0;
}
