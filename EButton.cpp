#include "EButton.h"

EButton::EButton(byte pin, bool pressedLow) {
    this->pin = pin;
    pinMode(pin, pressedLow ? INPUT_PULLUP : INPUT);
    pressedState = pressedLow ? 0 : 1;
    reset();
}

void EButton::setDebounceTime(byte time) {
    debounceTime = time;
}

#if defined(EBUTTON_SUPPORT_DONE_CLICKING) || defined(EBUTTON_SUPPORT_DOUBLE_CLICK)
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
void EButton::attachTransition(EButtonEventHandler cb) {
    transitionCB = cb;
}
#endif

#ifdef EBUTTON_SUPPORT_EACH_CLICK
void EButton::attachEachClick(EButtonEventHandler cb) {
    eachClickCB = cb;
}
#endif

#ifdef EBUTTON_SUPPORT_DONE_CLICKING
void EButton::attachDoneClicking(EButtonEventHandler cb) {
    doneClickingCB = cb;
}
#endif

#ifdef EBUTTON_SUPPORT_DOUBLE_CLICK
void EButton::attachSingleClick(EButtonEventHandler cb) {
    singleClickCB = cb;
}

void EButton::attachDoubleClick(EButtonEventHandler cb) {
    doubleClickCB = cb;
}
#endif

#ifdef EBUTTON_SUPPORT_LONG_PRESS_START
void EButton::attachLongPressStart(EButtonEventHandler cb) {
    longPressStartCB = cb;
}
#endif
#ifdef EBUTTON_SUPPORT_LONG_PRESS_HELD
void EButton::attachLongPressHeld(EButtonEventHandler cb) {
    longPressHeldCB = cb;
}
#endif
#ifdef EBUTTON_SUPPORT_LONG_PRESS_END
void EButton::attachLongPressEnd(EButtonEventHandler cb) {
    longPressEndCB = cb;
}
#endif

void EButton::reset() {
    state = EBUTTON_STATE_IDLE;
    startTime = 0;
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

unsigned long EButton::getPrevTransitionTime() {
    return prevTransitionTime;
}

bool EButton::operator==(EButton &other) {
    return (this == &other);
}

void EButton::tick() {
    unsigned long now = millis();

#ifdef EBUTTON_SUPPORT_LONG_PRESS_HELD
    if (state == EBUTTON_STATE_LONG_PRESSED) {
        // Call long press held callback function if in LONG_PRESSED state - ON EACH TICK!
        if (longPressHeldCB)
            longPressHeldCB(*this);
    }
#endif

    unsigned long sinceLastTransition = now - prevTransitionTime;
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
            startTime = now;        // remember when the first click was detected
            transition(now, true);  // call transition callback function
        }
    } else if (state == EBUTTON_STATE_COUNTING_CLICKS_DOWN) {
        if (buttonPressed) {
#ifdef EBUTTON_SUPPORT_LONG_PRESS
            // if the button is still pressed
            if (sinceLastTransition >= longPressTime) {
                // and it's been pressed for long enough since last transition...
                state = EBUTTON_STATE_LONG_PRESSED;  // change to LONG_PRESSED state

#ifdef EBUTTON_SUPPORT_LONG_PRESS_START
                // Call the press start callback function
                if (longPressStartCB)
                    longPressStartCB(*this);
#endif
            }
#endif
        } else {
            // if the button is released
            transition(now, false);        // call transition callback function
        }
    } else if (state == EBUTTON_STATE_COUNTING_CLICKS_UP) {
        if (buttonPressed) {
            // If the button is pressed
            transition(now, true);
        } else {
#if defined(EBUTTON_SUPPORT_DONE_CLICKING) || defined(EBUTTON_SUPPORT_DOUBLE_CLICK)
            if (sinceLastTransition >= clickTime) {
#ifdef EBUTTON_SUPPORT_DONE_CLICKING
                // Handling any-click
                if (doneClickingCB)
                    doneClickingCB(*this);
#endif
#ifdef EBUTTON_SUPPORT_DOUBLE_CLICK
                // Handling single-click
                if (clicks == 1 && singleClickCB)
                    singleClickCB(*this);

                // Handling double-click
                if (clicks == 2 && doubleClickCB)
                    doubleClickCB(*this);
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
            transition(now, false);
#ifdef EBUTTON_SUPPORT_LONG_PRESS_END
            if (longPressEndCB)
                longPressEndCB(*this);
#endif
            // Reset the FSM
            reset();
        }
    }
#endif
}

void EButton::transition(unsigned long now, bool pressed) {
    // Performed when a click transition is detected
    if (pressed) {
        state = EBUTTON_STATE_COUNTING_CLICKS_DOWN; // change to COUNTING_CLICKS_DOWN state
    } else {
#ifdef EBUTTON_SUPPORT_LONG_PRESS
        if (state != EBUTTON_STATE_LONG_PRESSED) {
            // Count a click only if we were not in LONG_PRESSED state
#endif
            state = EBUTTON_STATE_COUNTING_CLICKS_UP;   // change to COUNTING_CLICKS_DOWN state
            clicks++;                                   // increase clicks on transition to UP
#ifdef EBUTTON_SUPPORT_LONG_PRESS
        }
#endif
    }

#ifdef EBUTTON_SUPPORT_TRANSITION
    // Call the transition callback function
    if (transitionCB)
        transitionCB(*this);
#endif

#ifdef EBUTTON_SUPPORT_EACH_CLICK
    if (!pressed) {
#ifdef EBUTTON_SUPPORT_LONG_PRESS
        if (state != EBUTTON_STATE_LONG_PRESSED) {
#endif
            // if released and it was not in LONG_PRESSED state, then we have a CLICK event
            if (eachClickCB)
                eachClickCB(*this);
#ifdef EBUTTON_SUPPORT_LONG_PRESS
        }
#endif
    }
#endif
    prevTransitionTime = now;                       // remember last transition time
}
