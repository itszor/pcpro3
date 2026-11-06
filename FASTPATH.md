# The NE2000's fast path

!PC's NE2000 is software on the ARM: every 8390 register access, and every
word of a frame through its data port, is a trip through the Gemini mailbox.
The fast path is a second way into the same card, for a PC driver that knows
of it: frames go both ways through two rings in memory that the PC and the
ARM share, and a batch of frames costs the PC one or two mailbox accesses.

It is the same card as the NE2000: the same PCNE2 connection (to PCNAT, or a
real network), Ethernet address and IRQ (`NE2000Int`, 5). A driver that never
touches its ports sees an NE2000 as before. The code is in
`PC/DEV/c/NE2000`, under "Fast path"; the PC side is `drivers/apcnet.c` in
pccard-linux (branch `apcnet`), built into its Linux 4.19 kernel.

## Using it

- Build !PC from this branch (`BUILDING.md`) and use `build/bin/!RunImageG`
  as `!PC.!RunImage`. PCNE2 must be loaded, as for the NE2000 (the
  `Netlinks` extension).
- Boot a kernel with apcnet. It looks for the fast path early in boot and,
  if an attach works, takes the NE2000's ports too, so that ne.c (probing
  later, from `netdev=5,0x340,eth0`) leaves the card alone. If anything
  fails it says why and ne.c drives the NE2000 as before;
  `apcnet.disable=1` on the kernel command line does the same. Its messages
  begin `apcnet N:`, N being the driver's revision.
- In !PCConfig, Advanced, shared memory: **Video off, disc on** gives the
  network 32K (see below). The default, **All on**, gives it 16K.

## Shared memory on the Gemini card

Findings that decided the design, from a real Risc PC with a Gemini card and
from !PC's source:

- **The card's L2 cache holds the PC's ordinary RAM and does not see the
  ARM write to it.** !PC configures it with `CFGR_RAM_CACHED`
  (`CPU/c/cpu_g`), and the PC's page tables don't affect it: with the rings
  in kernel pages marked uncached (`set_memory_uc`), the ARM read what the
  PC wrote, but the PC never saw the ARM's answer. Version 1 of the fast
  path failed this way on real hardware (it works in RPCEmu, which has no
  caches).
- **E0000-EFFFF is the shared window.** !PC maps it to PC RAM, leaves it out
  of the L2 ("Leave CFGR_UMB_CACHED off, for shared memory"), and hands it
  out in pieces with `CPU_AllocSharedMem`, from the top down. The PC must
  still keep it out of its own cache (Linux: `set_memory_uc` over the
  kernel's mapping of 640K-1M, as apcdisk does for its buffer).
- **Who has the window:** HPC's buffers A and B, 16K each (disc and CD-ROM
  calls; apcdisk uses A), and Windrv, 16K (Aleph One's Windows 3.1/95
  display driver only; Linux doesn't use it). The BIOS is at F4000-FFFFF.
  That left 16K, E0000-E3FFF, unused: the fast path takes it, or Windrv's
  16K as well when Windrv is to have none.
- **The write FIFO.** The PC's memory writes reach memory through a write
  FIFO in the Gemini, but its port accesses reach the ARM at once, so a port
  write can arrive before the memory writes made just before it (HPC 1.1's
  write interlock is for this). The FIFO does not empty while the PC is held
  in the mailbox on that port write, so **any wait for PC-written memory
  must be done on a callback, never in the I/O handler**: a wait in the
  handler can only time out. The fast path's attach did exactly that at
  first and failed on real hardware with "status 0".
- **A mailbox access costs the PC about a millisecond** (measured in
  RPCEmu): the x86 is stalled until !PC's mailbox loop gets to it. So the
  fast path avoids them: no kick while the ARM is still sending (tx_busy),
  one interrupt and one port read per batch received.

### !PCConfig's shared memory settings

The Advanced window's radio buttons set `SharedMemFlags` (bit 0
`SM_DONT_USE`, bit 1 `SM_DONT_ALLOC`, bit 2 `SM_NOT_WINDRV`) to the
button's position. From !PCConfig's `Res` and !PC's source:

| Label | Value | HPC (disc) buffers | Windrv buffer | Fast path block |
|---|---|---|---|---|
| All on | 0 | shared, PC told | shared | 16K |
| Video on, ARM disc on | 1 | shared, but PC not told: it uses the data ports | shared | none |
| Video on, disc off | 2 | in !PC's own memory: data ports only | shared | none |
| Video off, disc on | 4 | shared, PC told | none | 32K |
| Video off, ARM disc on | 5 | shared, PC not told | none | none |
| All off | 6 | in !PC's own memory | none | none |

The fast path follows the disc flags (no block with `SM_DONT_USE` or
`SM_DONT_ALLOC`); with no block it isn't offered and the PC uses the
NE2000.

## The protocol (version 3)

Ports 0x310-0x317, 16-bit. (0x300-0x307 are HPC's, 0x308-0x30B Windrv's,
0x318-0x31B the mouse's pointer ports.)

| Port | Write | Read |
|---|---|---|
| 0x310 | command | 0x4E50 ("PN"), or after WHERE the block's address >> 4, then its length >> 4 |
| 0x312 | parameter bits 0-15 | version (3) |
| 0x314 | parameter bits 16-31 | how the last attach went: 0 none asked, 2 pending, 1 attached, -1 no magic word, -2 bad layout, -3 not the shared block |
| 0x316 | kick: the new tx_head (frames to send) | what interrupted (1 frames received, 2 room to send), which also clears the IRQ |

Commands: 1 ATTACH (parameter: the block's address), 2 DETACH, 3 RXOPTS
(parameter: 8390 RCR bits; multicast filter in the block), 4 RXKICK (room
made in a full receive ring), 5 WHERE.

ATTACH and RXOPTS read the block, so !PC carries them out on a callback once
the PC has gone on; the PC writes the block with status 0, the magic word
last, and waits for status to become 1 or negative (0x314 says the same
through the mailbox). A kick is also handled on a callback, which waits for
tx_head to reach the kicked value.

The block: a 32-word header (magic "APCN", version, sizes and offsets,
tx_head, tx_tail, rx_head, rx_tail, flags, rx_stalled, status, MAC address,
multicast filter, counters, tx_busy), then the rings. Ring records are a
length word and the frame, padded to a word; offsets wrap at the ring's size
(any multiple of 4) and a ring is never filled to its last word. apcnet
puts the send ring after the header and the receive ring in the second half.

On the ARM, received frames arrive from PCNE2 into its RAM as for the
NE2000, used as a staging ring (pages 46h-7Fh), and are moved to the PC's
ring from PCNE2's callback and !PC's poll. Frames to send are copied to
page 40h and given to `PCNE2_Transmit`.

## Results

RPCEmu, Debian 7 on Linux 4.19 (pccard-linux), 4MB over PCNAT, elapsed
seconds:

| | NE2000 (ne.c) | v1, rings in PC RAM | v2, 16K, 4K send ring | v3, 16K |
|---|---|---|---|---|
| Download | 3.7 (2.1 PC kernel) | 1.9-2.2 (0.55) | 2.4 | 1.9-2.4 (0.7) |
| Upload | 5.7 (4.5) | 2.9-3.0 (1.6) | 7.2 (5.1) | 3.9-4.1 (2.4) |
| Ping | 0.71ms | 0.57ms | | 0.57ms |

v1 doesn't work on a real card (above). With 32K the rings are as large as
v1's send ring was. Not yet measured on a real card.

## Debugging

- The PC: `dmesg | grep -i apcnet`. A failed attach is explained from port
  0x314: never got it, not yet carried out, attached but the answer didn't
  reach the PC (caching), or refused.
- !PC's trace (`Trace On`, to `<Wimp$ScrapDir>.PC.TraceFile`) has the
  shared block's address and size, and each attach, refusal and timeout.
