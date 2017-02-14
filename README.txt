Client Usage: ./awget <URL> [-c chainfile]
Note: Default filename for the chain file is chaingang.txt

Stepping Stone Usage: ./ss -p <port>

Description:
A simple proxy created by using multiple machines running ss and a client requesting a webpage or file using wget. 

Chainfile Format:
<Number of SteppingStones>
<hostname> <port>
<hostname> <port>
<hostname> <port>
...
<hostname> <port>

Example:
2
192.168.0.2 5001
192.168.0.3 5001