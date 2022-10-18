#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>

// STL version

std::mutex Vector_Lock;
std::mutex Main_Mutex;
std::mutex* Marker_Mutex;
std::condition_variable* cond_val_continue;
std::condition_variable* cond_val_end;
std::condition_variable* cond_val_temporal_workend;

bool all_zero(std::vector<int>& threads)
{
	for (size_t i = 0; i < threads.size(); i++)
	{
		if (threads[i] == 1)
			return false;
	}
	return true;
} // Function checks if all threads are stopped. If yes -- returns true. else -- false

void Marker(std::vector<int>& Arr, int num)
{
	std::unique_lock<std::mutex> Marker_Wait(Marker_Mutex[num - 1]);
	Vector_Lock.lock();

	int index; // oblivious
	int marked_count = 0; // oblivious
	std::vector<int> marked_array(Arr.size()); // array to mark indexes of marked elements... cringe
	std::chrono::duration < double, std::milli> wait(5);

	for (size_t i = 0; i < Arr.size(); i++)
	{
		marked_array[i] = 0;
	} // filling array of marks with "false" for each element

	srand(num); // Random generation seed

	while (true)
	{
		index = rand() % Arr.size(); // random index in {Inf.Arr}

		if (Arr[index] == 0)
		{
			std::this_thread::sleep_for(wait);
			Arr[index] = num; // the operation
			marked_count++; // how much is marked
			marked_array[index] = 1; // marking indexes to clear them later
			std::this_thread::sleep_for(wait);
		}
		else
		{
			std::cout << "Thread number: " << num 
				<< ", marked_count: " << marked_count 
				<< ", unmarked index: " << index << std::endl;

			cond_val_temporal_workend[num - 1].notify_one();
			Vector_Lock.unlock(); // Work is ended temporarily -- allow other threads to work

			cond_val_continue[num - 1].wait_for(Marker_Wait, std::chrono::seconds(10));

			if (cond_val_end[num - 1].wait_for(Marker_Wait, std::chrono::milliseconds(200)) == std::cv_status::timeout)
			{
				Vector_Lock.lock();
				break; // cycle ending
			}
			else
			{
				Vector_Lock.lock();
			}
		}
	}

	for (size_t i = 0; i < Arr.size(); i++)
	{
		if (marked_array[i] == 1)
		{
			Arr[i] = 0;
		}
	} // filling all marked elements with 0 before leaving

	// <code> std::cout << "Thread number: " << num << " job done" << std::endl;

	Vector_Lock.unlock(); // Work is ended permanently -- allow other threads to work

	return;
}

int main() {

	std::unique_lock<std::mutex> Main_Lock(Main_Mutex); // Enter Main
	Vector_Lock.lock();

	int size = 0;
	int thread_emount = 0;
	std::cout << "Enter array size: ";
	std::cin >> size;
	std::cout << "Enter marker thread emount: ";
	std::cin >> thread_emount;
	// Input
	std::vector<int> Arr(size);
	for (int i = 0; i < Arr.size(); ++i)
		Arr[i] = 0;
	// Array filling

	int synchro_time = 0;
	if (thread_emount != 0)
		synchro_time = 500 / thread_emount;
	std::chrono::duration < double, std::milli> synchronization_time(synchro_time);
	//  for threads to work in {thread_number} - growing sequence [used in this_thread::sleep_for]

	std::vector<std::thread> Threads(thread_emount);
	cond_val_continue = new std::condition_variable[thread_emount];
	cond_val_end = new std::condition_variable[thread_emount];
	cond_val_temporal_workend = new std::condition_variable[thread_emount];
	Marker_Mutex = new std::mutex[thread_emount];

	for (size_t i = 0; i < Threads.size(); i++)
	{
		Vector_Lock.unlock();
		Threads[i] = std::thread(Marker, std::ref(Arr), i + 1);
		cond_val_temporal_workend[i].wait_for(Main_Lock, std::chrono::seconds(1));
		Vector_Lock.lock();
	}

	std::vector<int> end_of_threads(thread_emount, 1);

	while (!all_zero(end_of_threads)) // While not all threads are finished
	{
		int break_num;
		std::cout << "Enter num of Marker() to break: ";
		std::cin >> break_num;
		break_num--; // input of index of Thread to stop 

		Vector_Lock.unlock();
		cond_val_continue[break_num].notify_all();

		Threads[break_num].join(); // Wait

		Vector_Lock.lock();

		std::cout << "Array: ";
		for (int i = 0; i < Arr.size(); ++i)
			std::cout << Arr[i] << " "; // Console output of {inf.Arr}
		std::cout << std::endl;

		end_of_threads[break_num] = 0; // Proof, that thread (number {break_num}) is stopped

		for (size_t i = 0; i < thread_emount; i++)
		{
			if (end_of_threads[i] != 0)
			{
				// <code> std::cout << "Thread: " << i + 1 << std::endl;
				Vector_Lock.unlock();
				cond_val_continue[i].notify_all();
				std::this_thread::sleep_for(std::chrono::duration < double, std::milli>(50));
				cond_val_end[i].notify_all();
				cond_val_temporal_workend[i].wait_for(Main_Lock, std::chrono::seconds(1));
				Vector_Lock.lock();
			}
		}
	}

	Vector_Lock.unlock();

	delete[] cond_val_continue;
	delete[] cond_val_end;
	delete[] cond_val_temporal_workend;
	delete[] Marker_Mutex;

	return 0;
}