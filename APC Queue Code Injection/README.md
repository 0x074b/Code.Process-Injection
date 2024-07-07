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
