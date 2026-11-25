# NET_Loopback

Validates `neon_net` (lwIP 2.2.1 on FreeRTOS over `neon_eth`) on the FX1 VP.

`CONFIG_NEON_MDF_NET_LOOPBACK=n`, so traffic to the interface's own address
(192.168.10.2/24, static) is not short-circuited inside lwIP: it leaves through
the MAC, descriptor DMA and PHY, and `--eth-peer loopback` returns every frame.
The checks therefore exercise the real driver path:

| Step | What it proves |
|---|---|
| init, set_config, start, wait_ready | tcpip thread, ETH0 ownership, PHY scan + autonegotiation, link poll, IPv4 events |
| 3 × ICMP echo (raw socket) | ARP request/reply for the own address through the line, ICMP reply |
| ARP table | own IP resolved to own MAC from frames that came back |
| UDP 1, 512, 1472, 4000, 8000 bytes | datagrams, IPv4 fragmentation and reassembly |
| TCP 16 KiB echo | listen/accept/connect, segmentation, windows, two tasks |
| stats | every frame sent came back, no descriptor/pbuf/mailbox/ring exhaustion, PLIC IRQ used |
| stop, restart, UDP + ICMP, stop, deinit | lifecycle, ARP relearned |

```sh
. scripts/env/env.sh                     # from the fx1-driver workspace
cd neon_mdf/examples/NET_Loopback
cmake --preset FX1 -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
cmake --build --preset FX1 --target run-vp
```

`run_vp.py` requires the VP verdict and every numbered check; `--writeback-delay-us N`
adds the VP fault injection where DONE precedes the DES0 write-back. Result on
VP_FX1_V3.0 `8aafae0`: 33 checks PASS, about 5 s of host time (also with
`--writeback-delay-us 500`).

Not covered (VP has no host network): DHCP, a second station, LAN/Internet
traffic. Those run on the FPGA (Phase 1C).
