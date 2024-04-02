#include <iostream> // Include necessary libraries
#include <Windows.h>
#include <TlHelp32.h>
#include <string>

// Injects the DLL into the process
bool InjectDLL(DWORD processId, const std::wstring& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId); // Open the target process
    if (hProcess == NULL) {
        std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl; // Print error message if opening process fails
        return false;
    }

    // Allocate memory in the target process for DLL path
    LPVOID pRemoteBuffer = VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pRemoteBuffer == NULL) { // Check if memory allocation was successful
        std::cerr << "Failed to allocate memory in the target process. Error code: " << GetLastError() << std::endl; // Print error message if memory allocation fails
        CloseHandle(hProcess); // Close process handle
        return false;
    }

    // Write DLL path into target process memory
    if (!WriteProcessMemory(hProcess, pRemoteBuffer, dllPath.c_str(), (dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
        std::cerr << "Failed to write DLL path into target process memory. Error code: " << GetLastError() << std::endl; // Print error message if writing to process memory fails
        VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE); // Free allocated memory
        CloseHandle(hProcess); // Close process handle
        return false;
    }

    // Get the address of LoadLibraryW function
    LPVOID pLoadLibrary = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    if (pLoadLibrary == NULL) { // Check if getting address was successful
        std::cerr << "Failed to get the address of LoadLibraryW function. Error code: " << GetLastError() << std::endl; // Print error message if getting address fails
        VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE); // Free allocated memory
        CloseHandle(hProcess); // Close process handle
        return false;
    }

    // Create a remote thread in the target process
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteBuffer, 0, NULL);
    if (hThread == NULL) { // Check if creating thread was successful
        std::cerr << "Failed to create remote thread in the target process. Error code: " << GetLastError() << std::endl; // Print error message if creating thread fails
        VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE); // Free allocated memory
        CloseHandle(hProcess); // Close process handle
        return false;
    }

    WaitForSingleObject(hThread, INFINITE); // Wait for the thread to finish execution

    CloseHandle(hThread); // Close thread handle
    VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE); // Free allocated memory
    CloseHandle(hProcess); // Close process handle

    std::cout << "DLL injected successfully!" << std::endl; // Print success message
    return true; // Return true to indicate success
}

// Lists running processes
void ListProcesses() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Take a snapshot of running processes
    if (hSnap != INVALID_HANDLE_VALUE) { // Check if snapshot creation was successful
        PROCESSENTRY32 pe32; // Create structure to hold process information
        pe32.dwSize = sizeof(PROCESSENTRY32); // Set the size of the structure

        if (Process32First(hSnap, &pe32)) { // Get the first process in the snapshot
            do {
                std::wcout << L"Process ID: " << pe32.th32ProcessID << L"\tName: " << pe32.szExeFile << std::endl; // Print process ID and name
            } while (Process32Next(hSnap, &pe32)); // Get the next process in the snapshot
        }

        CloseHandle(hSnap); // Close snapshot handle
    }
}

// Main function
int main() {
    std::wcout << L"Select a process to inject into:" << std::endl; // Prompt user to select a process
    ListProcesses(); // List running processes

    DWORD processId; // Variable to hold selected process ID
    std::wcout << L"Enter the Process ID: "; // Prompt user to enter process ID
    std::wcin >> processId; // Read process ID from user input

    std::wstring dllPath; // Variable to hold path to DLL
    std::wcout << L"Enter the path to the DLL: "; // Prompt user to enter path to DLL
    std::getline(std::wcin >> std::ws, dllPath); // Read path to DLL from user input

    if (InjectDLL(processId, dllPath)) { // Call function to inject DLL into selected process
        std::wcout << L"DLL injected successfully!" << std::endl; // Print success message
    }
    else {
        std::cerr << "Failed to inject DLL." << std::endl; // Print error message
    }

    return 0; // Return 0 to indicate successful program execution
}
