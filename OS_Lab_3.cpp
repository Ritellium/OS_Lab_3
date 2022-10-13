#include <iostream>
#include <Windows.h>
#include <process.h>

bool all_zero(int* threads, int emount)
{
    for (size_t i = 0; i < emount; i++)
    {
        if (threads[i] == 1)
            return false;
    }
    return true;
} // Function checks if all threads are stopped. If yes -- returns true. else -- false

struct Array_Size_Num {
    int size;
    int* Arr;
};

Array_Size_Num inf;

CRITICAL_SECTION struct_work;

HANDLE* MarkerN_wait;
HANDLE* MarkerN_console_stop;
HANDLE* MarkerN_console_resume;
// I know that globals are bad, but let it be (a so-so solution exists)

DWORD WINAPI Marker(LPVOID number) 
{

    EnterCriticalSection(&struct_work); // Work is started -- protect Inf{Arr, size} from other threads 

    int num = *(int*)(number); // Thread number
    int index; // oblivious
    int marked_count; // oblivious
    int* marked_array = new int[inf.size]; // array to mark indexes of marked elements... cringe

    for (size_t i = 0; i < inf.size; i++)
    {
        marked_array[i] = 0; 
    } // filling array of marks with "false" for each element

    srand(num); // Random generation seed
    marked_count = 0;
    while (true)
    {
        index = rand() % inf.size; // random index in {Inf.Arr}

        if (inf.Arr[index] == 0)
        {
            Sleep(5);
            inf.Arr[index] = num; // the operation
            marked_count++; // how much is marked
            marked_array[index] = 1; // marking indexes to clear them later
            Sleep(5);           
        }
        else
        {
            printf("Thread number: %d, marked_count: %d, unmarked index: %d \n", num, marked_count, index);
            SetEvent(MarkerN_wait[num - 1]); // Work is ended temporarily 
            LeaveCriticalSection(&struct_work); // Work is ended temporarily -- allow other threads to work

            WaitForSingleObject(MarkerN_console_stop[num - 1], INFINITE);

            if (WAIT_OBJECT_0 == WaitForSingleObject(MarkerN_console_resume[num - 1], 200))
            {
                EnterCriticalSection(&struct_work); // Work is started -- protect Inf{Arr, size} from other threads
            }     
            else
            {
                EnterCriticalSection(&struct_work); // this one is needed, believe me
                break;
            }   
        }
    }

    for (size_t i = 0; i < inf.size; i++)
    {
        if (marked_array[i] == 1)
        {
            inf.Arr[i] = 0;
        }
    } // filling all marked elements with 0 before leaving

    LeaveCriticalSection(&struct_work); // Work is ended permanently -- allow other threads to work

    ExitThread(0);
}

int main() {

    int synchronization_time = 0;
    int thread_emount = 0;
    printf("Enter array size: ");
    scanf_s("%d", &inf.size);
    printf("Enter marker thread emount: ");
    scanf_s("%d", &thread_emount);
    // Input

    if (thread_emount != 0)
        synchronization_time = 500 / thread_emount;
    //  for threads to work in {thread_number}-growing sequence [used in Sleep()]
    
    inf.Arr = new int[inf.size];
    for (int i = 0; i < inf.size; ++i) 
        inf.Arr[i] = 0;
    // Array filling

    HANDLE* hThread = new void*[thread_emount];

    MarkerN_wait = new void* [thread_emount];
    MarkerN_console_resume = new void* [thread_emount];
    MarkerN_console_stop = new void* [thread_emount];
    // Memory allocation for Events and Threads

    for (int i = 0; i < thread_emount; i++)
    {
        MarkerN_wait[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        MarkerN_console_resume[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        MarkerN_console_stop[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    } // Events initialization 

    InitializeCriticalSection(&struct_work); // Inf{Arr, size} is the one Critical Section in application

    int* end_of_threads = new int[thread_emount];
    for (int i = 0; i < thread_emount; ++i)
    {
        end_of_threads[i] = 1; // proof of thread not closed (needed further)
    }

    int* numbers = new int[thread_emount]; // Unique memory places for Marker() arguments

    for (int i = 0; i < thread_emount; i++)
    {
        numbers[i] = i + 1;
        hThread[i] = CreateThread(nullptr, 0, Marker, &numbers[i], CREATE_SUSPENDED, nullptr);
        ResumeThread(hThread[i]);

        Sleep(synchronization_time); // threads to work in {thread_number}-growing sequence [and it's just beautiful]
    } // Threads creation

    while (!all_zero(end_of_threads, thread_emount)) // While not all threads are finished
    {
        for (int i = 0; i < thread_emount; i++)
        {
            if (end_of_threads[i] != 0)
                WaitForSingleObject(MarkerN_wait[i], INFINITE);
        } // Works? -- Works!! (Wait for all !working! threads' events {MarkerN_wait[i]})

        int break_num;
        printf("Enter num of Marker() to break: ");
        scanf_s("%d", &break_num);
        break_num--; // input of index of Thread to stop 
        
        SetEvent(MarkerN_console_stop[break_num]); // Thread stopping
        WaitForSingleObject(hThread[break_num], INFINITE); // Wait

        printf("Array: ");
        for (int i = 0; i < inf.size; ++i)
            printf("%d ", inf.Arr[i]); // Console output of {inf.Arr}
        printf("\n");

        end_of_threads[break_num] = 0; // Proof, that thread (number {break_num}) is stopped

        for (size_t i = 0; i < thread_emount; i++)
        {
            if (end_of_threads[i] != 0)
            {
                SetEvent(MarkerN_console_stop[i]);
                SetEvent(MarkerN_console_resume[i]);

                Sleep(synchronization_time); // threads to work in {thread_number}-growing sequence [and it's just beautiful]
            }
        } // Signal for others to resume work

        CloseHandle(hThread[break_num]);
        CloseHandle(MarkerN_wait[break_num]);
        CloseHandle(MarkerN_console_stop[break_num]);
        CloseHandle(MarkerN_console_resume[break_num]); // Closing all objects needed for stopped thread
    }

    DeleteCriticalSection(&struct_work); // Critical Section Inf{Arr, size} is no more needed

    delete[] numbers;
    delete[] end_of_threads;
    delete[] hThread;
    delete[] MarkerN_wait;
    delete[] MarkerN_console_resume;
    delete[] MarkerN_console_stop;
    delete[] inf.Arr; // Memory cleaning

    return 0;
}