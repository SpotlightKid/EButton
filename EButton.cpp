#include "EButton.h"

EButton::EButton(byte pin, bool pressedLow) {
	this->pin = pin;
	pinMode(pin, pressedLow ? INPUT_PULLUP : INPUT);
	pressedState = !pressedLow;
	reset();
}

void EButton::setDebounceTime(byte time) {
	debounceTime = time;
}

#if defined(EBUTTON_SUPPORT_DONE_CLICKING) || defined(EBUTTON_SUPPORT_SINGLE_AND_DOUBLE_CLICKS)
void EButton::setClickTime(unsigned int time) {
	clickTime = time;
}
#endif

#ifdef EBUTTON_SUPPORT_LONG_PRESS
void EButton::setLongPressTime(unsigned int time) {
	longPressTime = time;
}
#endif

#ifdef EBUTTON_SUPPORT_TRANSITION
void EButton::attachTransition(EButtonEventHandler method) {
	transitionMethod = method;
}
#endif

#ifdef EBUTTON_SUPPORT_EACH_CLICK
void EButton::attachEachClick(EButtonEventHandler method) {
	eachClickMethod = method;
}
#endif

#ifdef EBUTTON_SUPPORT_DONE_CLICKING
void EButton::attachDoneClicking(EButtonEventHandler method) {
	doneClickingMethod = method;
}
#endif

#ifdef EBUTTON_SUPPORT_SINGLE_AND_DOUBLE_CLICKS
void EButton::attachSingleClick(EButtonEventHandler method) {
	singleClickMethod = method;
}

void EButton::attachDoubleClick(EButtonEventHandler method) {
	doubleClickMethod = method;
}
#endif

#ifdef EBUTTON_SUPPORT_LONG_PRESS
void EButton::attachLongPressStart(EButtonEventHandler method) {
	longPressStartMethod = method;
}

void EButton::attachDuringLongPress(EButtonEventHandler method) {
	duringLongPresstMethod = method;
}

void EButton::attachLongPressEnd(EButtonEventHandler method) {
	longPressEndMethod = method;
}
#endif

void EButton::reset() {
	state = EBUTTON_STATE_IDLE;
	startTime = 0;
	lastTransitionTime = 0;
	clicks = 0;
}

byte EButton::getPin() {
	return pin;
}

byte EButton::getClicks() {
	return clicks;
}

bool EButton::isButtonPressed() {
	return buttonPressed;
}

#ifdef EBUTTON_SUPPORT_LONG_PRESS
bool EButton::isLongPressed() {
	return state == EBUTTON_STATE_LONG_PRESSED;
}
#endif

unsigned long EButton::getStartTime() {
	return startTime;
}

unsigned long EButton::getLastTransitionTime() {
	return lastTransitionTime;
}

bool EButton::operator==(EButton &other) {
	return (this == &other);
}

void EButton::tick() {
	unsigned long now = millis();

#ifdef EBUTTON_SUPPORT_LONG_PRESS
	if (state == EBUTTON_STATE_LONG_PRESSED) {
		// Call during press method if in PRESSED state - ON EACH TICK!
		if (duringLongPresstMethod != NULL)
			duringLongPresstMethod(*this);
	}
#endif

	unsigned long sinceLastTransition = now - lastTransitionTime;
	if (sinceLastTransition < debounceTime) {
		// Skip the rest if there is a sample delay applied
		return;
	}

	// Sample button state
	buttonPressed = digitalRead(pin) == pressedState;

	if (state == EBUTTON_STATE_IDLE) {
		// If the state was idle
		if (buttonPressed) {
			//... and the button is pressed now
			startTime = lastTransitionTime = now;		// remember when the first click was detected
			transition(now);		// call transition method
		}
	} else if (state == EBUTTON_STATE_COUNTING_CLICKS_DOWN) {
		if (buttonPressed) {
#ifdef EBUTTON_SUPPORT_LONG_PRESS
			// if the button is still pressed
			if (sinceLastTransition >= longPressTime) {
				// and it's been pressed for long enough since last transition...
				state = EBUTTON_STATE_LONG_PRESSED;			// change to LONG_PRESSED state

				// Call the press start method
				if (longPressStartMethod != NULL)
					longPressStartMethod(*this);
			}
#endif
		} else {
			// if the button is released
			transition(now);		// call transition method
		}
	} else if (state == EBUTTON_STATE_COUNTING_CLICKS_UP) {
		if (buttonPressed) {
			// If the button is pressed
			transition(now);
		} else {
#if defined(EBUTTON_SUPPORT_DONE_CLICKING) || defined(EBUTTON_SUPPORT_SINGLE_AND_DOUBLE_CLICKS)
			if (sinceLastTransition >= clickTime) {
#ifdef EBUTTON_SUPPORT_DONE_CLICKING
				// Handling any-click
				if (doneClickingMethod != NULL)
					doneClickingMethod(*this);
#endif
#ifdef EBUTTON_SUPPORT_SINGLE_AND_DOUBLE_CLICKS
				// Handling single-click
				if (clicks == 1 && singleClickMethod != NULL)
					singleClickMethod(*this);

				// Handling double-click
				if (clicks == 2 && doubleClickMethod != NULL)
					doubleClickMethod(*this);
#endif
				// if the button is not pressed for long enough, then reset the FSM
				reset();
			}
#endif
		}
	}
#ifdef EBUTTON_SUPPORT_LONG_PRESS
	else if (state == EBUTTON_STATE_LONG_PRESSED) {
		if (!buttonPressed) {
			// Button was released from pressed state
			transition(now);
			if (longPressEndMethod != NULL)
				longPressEndMethod(*this);

			// Reset the FSM
			reset();
		}
	}
#endif
}

void EButton::transition(unsigned long now) {
	// Performed when a click transition is detected
	if (buttonPressed) {
		state = EBUTTON_STATE_COUNTING_CLICKS_DOWN;	// change to COUNTING_CLICKS_DOWN state
	} else {
#ifdef EBUTTON_SUPPORT_LONG_PRESS
		if (state != EBUTTON_STATE_LONG_PRESSED) {
			// Count a click only if we were not in PRESSED state
#endif
			state = EBUTTON_STATE_COUNTING_CLICKS_UP;	// change to COUNTING_CLICKS_DOWN state
			clicks++;									// increase clicks on transition to UP
#ifdef EBUTTON_SUPPORT_LONG_PRESS
		}
#endif
	}

#ifdef EBUTTON_SUPPORT_TRANSITION
	// Call the transition method
	if (transitionMethod != NULL)
		transitionMethod(*this);
#endif

#ifdef EBUTTON_SUPPORT_EACH_CLICK
	if (!buttonPressed) {
#ifdef EBUTTON_SUPPORT_LONG_PRESS
		if (state != EBUTTON_STATE_LONG_PRESSED) {
#endif
			// if released and it was not in PRESSED mode, then we have a CLICK event
			if (eachClickMethod != NULL)
				eachClickMethod(*this);
#ifdef EBUTTON_SUPPORT_LONG_PRESS
		}
#endif
	}
#endif
	lastTransitionTime = now;						// remember last transition time
}
