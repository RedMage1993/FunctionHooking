I demonstrate how to use Microsoft Detours. I originally used this to sniff packets passing through the recv() network function, used by win32 sockets. I found that the start of the function has a random MOV EDI, EDI opcode that's literally so that it can be replaced with a JMP lol.

Requires installing and compiling detours and including it into your Visual Studio project:
https://www.microsoft.com/en-us/download/details.aspx?id=52586

<img src="https://d26dzxoao6i3hh.cloudfront.net/items/0j0n3E2m0i0p123e1e07/10462188_859164637444932_2600564250799219960_o.jpg">
