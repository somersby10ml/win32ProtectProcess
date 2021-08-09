# Windows Protect Process

UserMode and KernelMode Simple SourceCode
The source uses Visual Studio C++ 2019

## UserMode 
Written and tested on Windows 10 64-bit.

Use **ZwQuerySystemInformation** and **DuplicateHandle(DUPLICATE_CLOSE_SOURCE)**

Query all handles on the system and close the process if it is open (OpenProcess).
It's a little slow.
Administrator privileges are required.


## KernelMode
Written and tested on Windows 7 64-bit.


Driver : c  
Driver Install and Controller : C# (Administrator privileges are required.)

Use **ObRegisterCallbacks** to block handles being fetched.

