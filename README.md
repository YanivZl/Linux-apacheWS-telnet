Unix / Linux Project

Guide for users:

1.	Work space
  1.1	   Module – libcli
  1.2	   Module – apache
  1.3	   Module – telnetd
  1.4	   User – UNIX
  1.5	   Password – 123

2.	Compilation
2.1     gcc myFileSystemMonitor.c -pthread -lcli -finstrument-functions -rdynamic -o myFileSystemMonitor

3.	Command  - execution 
3.1	    ./myFileSystemMonitor -d /home/user/Desktop/test -i 127.0.0.1 

4.	Command – Netcat – Listening to udp client
4.1    netcat -l -u -p 8000

5.	Command – Telnet
5.1    telnet localhost 9090

Done by:
Name	            ID
Yaniv Zlotnik	    314880493
Anar Djafarov	    205982804
Daniel Modilevsky	312493000

