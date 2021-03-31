# arm9linuxfw

Firmware that runs on the ARM9 core.

The 3DS has multiple ARM11 cores that are the main application processor which
for linux-3ds run the kernel and userspace. The ARM9 core gets loaded with this
firmware. It communicates with the ARM11 cores via
[the PXI protocol](https://www.3dbrew.org/wiki/PXI_Services), and facilitates
SDMMC access
([though controller select can be transferred to arm11](https://www.3dbrew.org/wiki/CONFIG9_Registers#CFG9_SDMMCCTL)).

## LICENSE

GPLv2
