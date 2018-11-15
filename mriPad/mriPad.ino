#include <Bounce2.h>
#include <Keyboard.h>

//#define leftHand

#define numkeys 5

// Bounce declaration
Bounce * bounce = new Bounce[numkeys];
byte button[] = { 2, 3, 4, 5, 6 };
char left[] = { "12345" };
char right[] = { "67890" };

void setup() {
	for (int x = 0; x < numkeys; x++) {
		pinMode(button[x], INPUT_PULLUP);
		bounce[x].attach(button[x]);
		bounce[x].interval(8);
	}
}

void loop() {
		for (int x = 0; x < numkeys; x++) {
		bounce[x].update();
		#ifdef leftHand
			if(!bounce[x].read()) Keyboard.press(left[x]);
			if(bounce[x].read()) Keyboard.release(left[x]);
		#else
			if(!bounce[x].read()) Keyboard.press(right[x]);
			if(bounce[x].read()) Keyboard.release(right[x]);
		#endif
	}
}