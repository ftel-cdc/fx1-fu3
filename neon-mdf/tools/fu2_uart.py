"""Host UART transport: standard-library POSIX I/O, optional pyserial elsewhere.

Copyright (c) 2026 FPT Telecom. All rights reserved.
"""

import os
from pathlib import Path
import sys
import time

if os.name == "posix":
    import fcntl
    import select
    import termios
    import tty


class PosixUART:
    """Minimal serial interface used by the FU2 loader and monitor."""

    def __init__(self, port, baud):
        self.port = port
        self.timeout = 0.05
        self.write_timeout = 2.0
        self._fd = None
        self._saved_attrs = None
        self._received = bytearray()
        try:
            self._fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            if not os.isatty(self._fd):
                raise OSError(f"{port} is not a TTY")
            fcntl.flock(self._fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self._saved_attrs = termios.tcgetattr(self._fd)
            tty.setraw(self._fd, termios.TCSANOW)
            self.baudrate = baud
            # Do not flush RX: a ROMCODE request may already be waiting.
        except BaseException:
            self.close()
            raise

    def _descriptor(self):
        if self._fd is None:
            raise OSError("UART is closed")
        return self._fd

    @property
    def baudrate(self):
        return self._baudrate

    @baudrate.setter
    def baudrate(self, value):
        speed = getattr(termios, f"B{value}", None)
        if speed is None:
            raise ValueError(f"Unsupported baud rate: {value}")
        fd = self._descriptor()
        attrs = termios.tcgetattr(fd)
        attrs[0] &= ~(termios.IXON | termios.IXOFF | getattr(termios, "IXANY", 0))
        attrs[2] &= ~(termios.CSIZE | termios.PARENB | termios.CSTOPB |
                      getattr(termios, "CRTSCTS", 0))
        attrs[2] |= termios.CS8 | termios.CLOCAL | termios.CREAD
        attrs[4] = attrs[5] = speed
        attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        self._baudrate = value

    def _receive(self, size):
        try:
            data = os.read(self._descriptor(), size)
        except BlockingIOError:
            return b""
        if not data:
            raise OSError("UART disconnected")
        return data

    @property
    def in_waiting(self):
        # Buffer available data instead of relying on platform-specific ioctls.
        if not self._received and select.select([self._descriptor()], [], [], 0)[0]:
            self._received.extend(self._receive(4096))
        return len(self._received)

    def read(self, size):
        if size <= 0:
            return b""
        if self._received:
            data = bytes(self._received[:size])
            del self._received[:size]
            return data
        if not select.select([self._descriptor()], [], [], self.timeout)[0]:
            return b""
        return self._receive(size)

    def read_available(self, timeout):
        """Wait once, then read the available burst like the original loader."""
        if self._received:
            data = bytes(self._received)
            self._received.clear()
            return data
        if not select.select([self._descriptor()], [], [], timeout)[0]:
            return b""
        return self._receive(4096)

    def write(self, data):
        fd = self._descriptor()
        deadline = time.monotonic() + self.write_timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([], [fd], [], remaining)[1]:
                raise TimeoutError("UART write timed out")
            try:
                return os.write(fd, data)
            except BlockingIOError:
                continue

    def flush(self):
        termios.tcdrain(self._descriptor())

    def close(self):
        fd, self._fd = self._fd, None
        if fd is None:
            return
        try:
            if self._saved_attrs is not None:
                try:
                    termios.tcsetattr(fd, termios.TCSANOW, self._saved_attrs)
                except (OSError, termios.error):
                    pass  # A disconnected device must still release its fd/lock.
        finally:
            os.close(fd)


def open_uart(port, baud):
    """Open UART in raw 8N1 mode without a software-triggered board reset."""
    if os.name == "posix":
        try:
            return PosixUART(port, baud)
        except (OSError, ValueError, termios.error) as error:
            raise OSError(f"Cannot open/configure UART {port}: {error}. "
                          "Close other serial monitors and check permissions/baud.") from error

    try:
        import serial
    except ImportError as error:
        requirements = Path(__file__).with_name("requirements.txt")
        raise OSError(
            "Windows UART support requires pyserial. Install with: "
            f'"{sys.executable}" -m pip install -r "{requirements}"') from error
    connection = serial.Serial(port=None, baudrate=baud, bytesize=serial.EIGHTBITS,
                               parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
                               timeout=0.05, write_timeout=2.0, xonxoff=False,
                               rtscts=False, dsrdtr=False)
    connection.dtr = False
    connection.rts = False
    connection.port = port
    try:
        connection.open()
    except (OSError, serial.SerialException) as error:
        connection.close()
        raise OSError(f"Cannot open UART {port}: {error}. "
                      "Close other serial monitors and check permissions.") from error
    return connection
