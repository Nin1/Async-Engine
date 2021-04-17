#include <Jobs/Jobs.h>

std::atomic<int> m_jobsDone = 0;
int NUM_JOBS_SINGLETHREADTEST = 2000;
int NUM_JOBS_NEST_A = 200;
int NUM_JOBS_NEST_B = 200;

void Test1b(void* data)
{
	// Do some task
	int total = 0;
	for (int i = 0; i < 123456; i++)
	{
		total += i;
	}
	// Stop once all jobs are done
	++m_jobsDone;
	if (m_jobsDone == NUM_JOBS_SINGLETHREADTEST)
	{
		std::cout << "Completed " << m_jobsDone << " jobs" << std::endl;
		Jobs::Stop();
	}
}

void Test1a(void* data)
{
	m_jobsDone = 0;
	JobCounterPtr counter = Jobs::GetNewJobCounter();
	for (int i = 0; i < NUM_JOBS_SINGLETHREADTEST; i++)
	{
		Jobs::CreateJobAndCount(Test1b, nullptr, counter);
	}
}

void Test2c(void* data)
{
	// Do some task
	int total = 0;
	for (int i = 0; i < 123456; i++)
	{
		total += i;
	}
	// Stop once all jobs are done
	++m_jobsDone;
	if (m_jobsDone == NUM_JOBS_NEST_A * NUM_JOBS_NEST_B)
	{
		std::cout << "Completed " << m_jobsDone << " jobs" << std::endl;
		Jobs::Stop();
	}
}

// Creates several small jobs
void Test2b(void* data)
{
	for (int i = 0; i < NUM_JOBS_NEST_B; i++)
	{
		Jobs::CreateJob(Test2c, nullptr);
	}
}

// Creates several jobs that will create more jobs
void Test2a(void* data)
{
	m_jobsDone = 0;
	JobCounterPtr counter = Jobs::GetNewJobCounter();
	for (int i = 0; i < NUM_JOBS_NEST_A; i++)
	{
		Jobs::CreateJobAndCount(Test2b, nullptr, counter);
	}
}

int main()
{
	// Single-thread test
	std::cout << "Starting single-thread test" << std::endl;
	auto start = std::chrono::system_clock::now();
	Jobs singleThreadTest(1, Test1a, nullptr);
	auto end = std::chrono::system_clock::now();
	auto elapsed = end - start;
	std::cout << "Single-thread test completed in " << elapsed.count() << "ns" << std::endl;

	// Multi-thread test
	std::cout << "Starting multi-thread test" << std::endl;
	start = std::chrono::system_clock::now();
	Jobs multiThreadTest(Test1a, nullptr);
	end = std::chrono::system_clock::now();
	elapsed = end - start;
	std::cout << "Multi-thread test completed in " << elapsed.count() << "ns" << std::endl;

	// Nested jobs test
	std::cout << "Starting nested job test" << std::endl;
	start = std::chrono::system_clock::now();
	Jobs nestedJobTest(Test2a, nullptr);
	end = std::chrono::system_clock::now();
	elapsed = end - start;
	std::cout << "nested job test completed in " << elapsed.count() << "ns" << std::endl;

	return 0;
}