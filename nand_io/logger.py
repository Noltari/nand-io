# SPDX-License-Identifier: MIT
"""Logger."""

import sys

NOTSET = 0
DEBUG = 10
INFO = 20
WARNING = 30
ERROR = 40
CRITICAL = 50


class Logger:
    """Logger."""

    def __init__(self, level=NOTSET, stream=sys.stdout):
        """Init Logger."""
        self.stream = stream
        self.level = level

    def critical(self, fmt, *args):
        """Logger critical."""
        return self.print(CRITICAL, fmt % args)

    def debug(self, fmt, *args):
        """Logger debug."""
        return self.print(DEBUG, fmt % args)

    def error(self, fmt, *args):
        """Logger error."""
        return self.print(ERROR, fmt % args)

    def info(self, fmt, *args):
        """Logger info."""
        return self.print(INFO, fmt % args)

    def print(self, level, string):
        """Logger print."""
        if self.stream and self.level <= level:
            self.stream.write(string)
            return True
        return False

    def warning(self, fmt, *args):
        """Logger warning."""
        return self.print(WARNING, fmt % args)
