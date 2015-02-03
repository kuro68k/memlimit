# memlimit
Limit memory use of running processes.

Memlimit monitors a process and if it exceeds the limit on memory use kills it. Memory use is calculated similarly but not exactly the same way as the Windows Task Manager. Essentially it is a watchdog designed to kill tasks that start allocating lots of RAM when they fail.

Memlimit was originally written to kill off Directory Opus when it started using gigabytes of RAM on my home server. It has not been extensively tested on other systems.

Usage: `memlimit <task name> <memory limit in megabytes>`

Note the use of JEDEC standard megabytes (2^20 bytes). A tray iconw will appear; hovering over it will give some stats on the task being monitored. Memlimit must be run at administrator priviledge level in order to kill other tasks.

Licence: GPL v3.
