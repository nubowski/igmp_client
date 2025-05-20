#!/usr/bin/env python3
from scapy.all import send, IP, load_contrib

load_contrib("igmp")
from scapy.contrib.igmp import IGMP

iface = "eth0"
dst = "224.0.0.1"

print(f"[SCAPY] Sending IGMPv2 Query to {dst} via {iface}")
pkt = IP(dst=dst)/IGMP(type=0x11)
send(pkt, iface=iface, verbose=True)
print("[SCAPY] Done.")