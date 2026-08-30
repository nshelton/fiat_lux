#pragma once

void transitionStart();   // snapshot the screen and begin the crossfade
void transitionPost();    // dissolve the old frame over the new render, healing to a no-op
void transitionCancel();
