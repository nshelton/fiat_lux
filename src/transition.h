#pragma once

void transitionStart();   // snapshot the screen and begin the wipe
bool transitionFrame();   // draw one out-phase frame; false once the old scene is gone
void transitionPost();    // corrupt the incoming scene's render, healing to a no-op
bool transitionActive();
void transitionCancel();
