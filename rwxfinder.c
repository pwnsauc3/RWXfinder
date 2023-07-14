#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

void SetConsoleColor(int colorCode)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, colorCode);
}

void CheckDLLForRWX(const char* dllPath)
{
    HANDLE fileHandle = CreateFile(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open DLL: %s\n", dllPath);
        return;
    }

    DWORD fileSize = GetFileSize(fileHandle, NULL);
    if (fileSize == INVALID_FILE_SIZE)
    {
        fprintf(stderr, "Failed to get file size: %s\n", dllPath);
        CloseHandle(fileHandle);
        return;
    }

    HANDLE fileMapping = CreateFileMapping(fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (fileMapping == NULL)
    {
        fprintf(stderr, "Failed to create file mapping: %s\n", dllPath);
        CloseHandle(fileHandle);
        return;
    }

    LPVOID fileView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, fileSize);
    if (fileView == NULL)
    {
        fprintf(stderr, "Failed to map view of file: %s\n", dllPath);
        CloseHandle(fileMapping);
        CloseHandle(fileHandle);
        return;
    }

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)fileView;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)fileView + dosHeader->e_lfanew);
    PIMAGE_OPTIONAL_HEADER optionalHeader = &(ntHeaders->OptionalHeader);
    PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);

    int hasDefaultRWXSection = 0;
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++)
    {
        if (sectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
            sectionHeader->Characteristics & IMAGE_SCN_MEM_READ &&
            sectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE)
        {
            hasDefaultRWXSection = 1;
            break;
        }
    }

    if (hasDefaultRWXSection)
    {
        SetConsoleColor(10); // Set text color to green
        printf("[RWX] %s\n", dllPath);
        SetConsoleColor(7); // Set text color to default

    printf("Section Name: %.8s\n", sectionHeader->Name);
    printf("Virtual Size: 0x%X\n", sectionHeader->Misc.VirtualSize);
    printf("Virtual Address: 0x%X\n", sectionHeader->VirtualAddress);
    printf("Size of Raw Data: 0x%X\n", sectionHeader->SizeOfRawData);
    printf("Characteristics: 0x%X\n", sectionHeader->Characteristics);
    printf("---------------------------\n");

        // Perform additional testing or analysis on the DLL here
        // ...

    }
    else
    {
        printf("%s\n", dllPath);
    }

    UnmapViewOfFile(fileView);
    CloseHandle(fileMapping);
    CloseHandle(fileHandle);
}

void TraverseDirectory(const char* directory)
{
    char searchPath[MAX_PATH];
    WIN32_FIND_DATA findData;
    HANDLE findHandle;

    sprintf(searchPath, "%s\\*", directory);
    findHandle = FindFirstFile(searchPath, &findData);

    if (findHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
            {
                char filePath[MAX_PATH];
                sprintf(filePath, "%s\\%s", directory, findData.cFileName);

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // Recursive call for subdirectories
                    TraverseDirectory(filePath);
                }
                else if (findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                {
                    // Check if the file is a DLL
                    const char* extension = strrchr(filePath, '.');
                    if (extension != NULL && _stricmp(extension, ".dll") == 0)
                    {
                        CheckDLLForRWX(filePath);
                    }
                }
            }
        } while (FindNextFile(findHandle, &findData));

        FindClose(findHandle);
    }
}

int main()
{
    // Setting PATH to check
    SetDllDirectory("C:\\Program Files (x86)");

    TraverseDirectory("C:\\");

    return 0;
}
