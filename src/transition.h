#pragma once

void transitionStart();   // snapshot the screen and begin the wipe
bool transitionFrame();   // draw one frame; false once finished
bool transitionActive();
void transitionCancel();
