#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(i2cselect, 0.1, https://github.com/tobiasvogel/i2cselect)
AC_CONFIG_SRCDIR([src/i2cselect.c])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strchr strrchr strtol])

# Definition Macros used in the Utility Code itself
AC_DEFINE([CASE_SENSITIVE], [1], [Decides when calling the utility using predefined Shortcut-Names, whether the Shortcut-Names should be treated Case Sensitive or not. 1 = True (Case Sensitive), 0 = False (Case Insensitive)])

AC_DEFINE([USE_TEXT_FORMATING], [0], [If true, Help- and Usage-Message are being rendered with advanced Text-Formatting. Not all Terminals support this. 1 = True (Advanced Text Formatting), 0 = False (Standard Text Output)])

AC_DEFINE([SKIP_IOCTL_FUNC_CHECK], [1], [If not skipped, prior to opening the I2C-Bus a very simple Check of the I2C-Lines Capabilities is performed. Not relevant for the most basic usage (Single Byte Read). 1 = True (Skip Checks - default), 0 = False (Do some basic checks, not really helpful at the moment)])

AC_DEFINE([IGNORE_COMPLEX_SHORTCUTS], [0], [If false, more complex options can be passed along with Shortcut definitions like Pre- and Post-Execution of Commands. If very concerned about security, set to 1. 1 = True (Allow only Aliasing Lines, no additional Options), 0 = False (Allow complex Shortcuts)])

AC_DEFINE([IGNORE_EXTERNAL_SHORTCUTS_FILE], [0], [If false, Shortcuts can not only be hardcoded into the executable, but also be specified in an external file at runtime. If very concerned about security, set to 1. 1 = True (No external Shortcut-definitions allowed), 0 == False (Do read/process external Shortcut-definitions if specified at runtime)])

AC_DEFINE([IGNORE_EXTERNAL_COMPLEX_SHORTCUTS], [0], [If false, Shortcuts from external Files will be processed at runtime. However, only basic Shortcuts (a.k.a Aliasing) will be allowed. Complex options like Pre- and Post-Execution of Commands will be ignored. If the above "IGNORE_EXTERNAL_SHORTCUTS_FILE" is set as 1 (True), this setting has no effect whatsoever, as external Shortcuts are ignored anyway. If very concerned about security, set to 1. 1 = True (External Shortcuts may not contain complex options), 0 = False (Include complex shortcuts from external file)])

AC_DEFINE([VERBOSE_OUT_STREAM], [stderr], [If the utility is run in verbose mode, Messages will be directed to either the standard output or the standard error. "stderr" (Without quotes; --> standard output), "stdout" (Without quotes; --> standard error)])

AC_DEFINE([MAX_SHORTCUT_LENGTH], [100], [Maximum length of Shortcut-Names (characters). Usually no need to change this!])

AC_DEFINE([MAX_SHELL_COMMAND_LENGHT], [999], [Maximum length of Commands to be executed as Pre- or Post-Execution Option as specified in complex Shortcuts. Usually no need to change this!])

AC_DEFINE([REDIRECT_EXTERNAL_COMMANDS_STDERR], [1], [Whether the output to Standard Error of Commands executed as Pre- or Post-Execution Command is being redirected to the output of the utility. (Append '2>&1' to external commands, outputted only if run verbosely!). 1 = True (Redirect output), 0 = False (Discard output)])

AC_DEFINE([WAITTIME_ON_PIDLOCK], [3], [To prevent multiple Operations to different Lines at once, especially when Pre- or Post-Execution Commands are running, the utility restricts to only one running instance. In such cases, it will go to sleep and retry periodocally every second for X of seconds, before exiting with an error or completing the task. Set the number of seconds below. To immediately exit with an error and avoid having multiple instances queue up, set to 0.])

AC_DEFINE([PIDFILE], ["/var/run/i2cselect.pid"], [Filesystem-Location to put the locking PID-File. Using the default location will require the utility to be either run as root or otherwise to be granted write-access to the specified location. Alternatively, set to a location where a unprivileged user can write to. On a Raspberry Pi (*only* if not more than one user - including any system user accounts! - are invoking the utility -- e.g. YOU!) /var/run/user/<UID>/i2cselect.pid may be a good alternative, UID being the primary numerical user id as reported by `id -u`])

AC_CHECK_FILE(src/user_shortcuts.h, AC_DEFINE([HAVE_USER_SHORTCUTS_FILE], [], [Whether file src/user_shortcuts.h exists.]))

AC_CONFIG_HEADERS(config.h)

AC_OUTPUT(Makefile src/Makefile doc/Makefile man/Makefile examples/Makefile)
