#!/bin/bash

IFACE="eth0"
MAX_RESP_TIME=3000

sudo ./bin/igmp_client -i "$IFACE" -g 239.1.2.3 239.1.2.4 239.1.2.5 239.1.3.1 239.2.0.1 -t "$MAX_RESP_TIME"