#!/bin/sh -e

function setup {
    ip netns add TEST
    ip link add veth0 type veth peer name veth1
    ip link set dev veth0 up
    ip link set dev veth1 up
    ip addr add dev veth0 local 10.0.0.1
    ip route add 10.0.0.2 dev veth0
    ip link set dev veth1 netns TEST
    ip netns exec TEST ip link set dev veth1 up
}

function teardown {
    ip link set dev veth0 down
    ip link set dev veth1 down
    ip link delete veth0
    ip netns delete TEST
}

if [ $# -eq 0 ]; then
    echo "Usage: $(basename $0) COMMAND"
    exit
fi
$1
