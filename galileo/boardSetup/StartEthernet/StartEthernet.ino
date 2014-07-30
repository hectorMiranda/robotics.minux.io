//Sketch to connect a PC host to Linux Galileo via telnet 
// Written by M. Lastri fo garretlabs.wordpress.com
//The sketch will:
//1. set the Galileo IP to 192.168.0.102
//2. start telnet server
//------>>>>>After this the user can:
//3. From the pc host (i.e. with IP =169.254.1.2) send ping 169.254.1.1 and verify the answer
//4. From the host send the famous command "telnet 192.168.0.102"...and the Linux Galileo shell should appear (user: root, no password)!
//5. Enjoy!
void setup() 
{
  system("ifconfig > /dev/ttyGS0");
  system("ifconfig eth0 192.168.0.102 netmask 255.255.255.0 up");
  system("ifconfig > /dev/ttyGS0");
  system("telnetd -l /bin/sh");
}
void loop() 
{
  //nothing here 
}
