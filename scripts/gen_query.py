#!/usr/bin/env python3
from scapy.all import sendp, Ether, IP
from scapy.contrib.igmp import IGMP

iface = "eth0"
dst = "224.0.0.1"

print(f"[SCAPY] Sending IGMPv2 Query to {dst} via {iface}")

# pkt creating
pkt = Ether()/IP(dst=dst)/IGMP(type=0x11)

# L2 through interface
sendp(pkt, iface=iface, verbose=True)

print("[SCAPY] Done.")
