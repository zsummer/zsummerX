#!/sbin/bash

echo "root soft core unlimited" >> /etc/security/limits.conf
echo "*    soft core unlimited" >> /etc/security/limits.conf
echo "root hard core unlimited" >> /etc/security/limits.conf
echo "*    hard core unlimited" >> /etc/security/limits.conf
echo "root  soft nofile 32768" >> /etc/security/limits.conf
echo "*     soft nofile 32768" >> /etc/security/limits.conf
echo "root  hard nofile 32768" >> /etc/security/limits.conf
echo "*     hard nofile 32768" >> /etc/security/limits.conf
echo "fs.file-max=655350" >> /etc/sysctl.conf 
echo "net.ipv4.tcp_syncookies = 1" >> /etc/sysctl.conf
echo "net.ipv4.tcp_tw_reuse = 1" >> /etc/sysctl.conf
echo "net.ipv4.tcp_tw_recycle = 1" >> /etc/sysctl.conf
echo "net.ipv4.tcp_fin_timeout = 1" >> /etc/sysctl.conf
echo "net.ipv4.ip_local_port_range = 1024 65535" >> /etc/sysctl.conf
sysctl -p
