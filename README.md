# ReverseShell

N.B. This is a work in progress and not yet functional.

Creates a reverse shell over a secure websocket connection, i.e. it connects securely to a server and allows the server
to run commands on the client. Offers security such as certificate pinning.

This program is intended for support use cases, where a system you own needs upgrading or whatever. The system can be set
up to connect to your support server where an admin can reconfigure the device as required. This is **not intended as a
security exploit**, if you have the power to start this program you have the power to start a reverse shell in any number
of ways. This program adds layers of security that a hacker would not require.

Only works on UNIX.
