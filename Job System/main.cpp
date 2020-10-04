#include "Jobs.h"
#include <array>
#include <iostream>
#include <thread>
#include <vector>

std::chrono::system_clock::time_point m_start;
std::atomic<int> m_jobsDone = 0;
// This doesn't work if the job list for a given thread fills up - it never completes the last job
const int NUM_JOBS = 20000;

void TestJob1(void* data)
{
	int total = 0;
	for (int i = 0; i < 123456; i++)
	{
		total += i;
	}
	++m_jobsDone;
	if (m_jobsDone == NUM_JOBS)
	{
		auto end = std::chrono::system_clock::now();
		auto elapsed = end - m_start;
		std::cout << "Completed " << m_jobsDone << " jobs in " << elapsed.count() << "ns" << std::endl;
	}
}

void MainJob(void* data)
{
	m_start = std::chrono::system_clock::now();
	JobCounterPtr counter = Jobs::GetNewJobCounter();
	for (int i = 0; i < NUM_JOBS; i++)
	{
		Jobs::CreateJobAndCount(TestJob1, nullptr, counter);
	}
	std::cout << "Created all jobs" << std::endl;
}

int main()
{
	// Set up job system
	Jobs jobs(MainJob);

	while (jobs.IsRunning())
	{

		std::this_thread::sleep_for(std::chrono::seconds(3));
	}

	return 0;
}