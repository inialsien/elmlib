#!/bin/bash

# if /etc/modules exists AND /etc/modules does not contain "vcan"
#       add vcan0 to /etc/modules
if grep -Fxq "vcan" /etc/modules
then
    echo "vcan is installed in /etc/modules"
else
    echo "adding vcan to /etc/modules"
    echo "vcan" >> /etc/modules
fi


# if /etc/network/interfaces does not contain "vcan"
#       add rule to /etc/network/interfaces


if grep -Fxq "vcan0" /etc/network/interfaces
then
    echo "vcan0 already exists in /etc/network/interfaces"
else
    echo "adding vcan0 to /etc/network/interfaces"
    echo "vcan" >> /etc/modules
    echo '
auto vcan0
   iface vcan0 inet manual
   pre-up /sbin/ip link add dev $IFACE type vcan
   up /sbin/ifconfig $IFACE up
'  >> /etc/network/interfaces
fi

echo "Installation complete"
