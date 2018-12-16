#ifndef USER_SHORTCUTS_H
#define USER_SHORTCUTS_H

// Convenience Shortcuts, list aliases here.
// These Shortcuts get hardcoded in the Utility itself
// Shortcuts can however, also be placed in a File
// (conflicting shortcuts from file overwrite shortcuts
// specified here).
// External files needs to be referenced as environment-
// variable called IICSELECT_CONFIG at runtime.
// Reading shortcuts at runtime from a file can
// be globally disabled by setting the macro
// IGNORE_EXTERNAL_SHORTCUTS_FILE to 1 in config.h


shortcut_struct static_shortcuts[] = {

  { .bus = 3, .name = "display" },
  { .bus = 4, .name = "camera" },
  { .bus = 5, .name = "ir-sensor" },
  { .bus = 4, .name = "camera-1", .exec_after = "/usr/local/bin/cameraselect 1" }

};


#endif // USER_SHORTCUTS_H