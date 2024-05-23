# Backdooring PE Files with Shellcode
*The purpose of this lab is to learn the Portable Executable (PE) backdooring technique by adding a new readable/writable/executable code section with our malicious shellcode to any portable executable file.*

*High level process of this technique*:

*-Add a new RWX PE section, big enough to hold our shellcode, to any .exe file*  
*-Generate shellcode*  
*-Add shellcode to the newly created PE section*  
*-Redirect execution flow of the .exe file being backdoored to the shellcode*  
*-Redirect execution flow back to the legitimate .exe instructions*  

