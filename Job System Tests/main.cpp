#include <Jobs/Jobs.h>


std::atomic<int> m_jobsDone = 0;
int NUM_JOBS_SINGLETHREADTEST = 2000;
int NUM_JOBS_NEST_A = 200;
int NUM_JOBS_NEST_B = 200;
static std::atomic<uint64_t> count = 0;

void Test1b(void* data)
{
	// Do some task
	uint64_t total = 0;
	for (int i = 0; i < 123456; i++)
	{
		total += i;
	}
	count.fetch_add(total);
}

void Test1a(void* data)
{
	m_jobsDone = 0;
	JobCounterPtr counter = Jobs::GetNewJobCounter();
	for (int i = 0; i < NUM_JOBS_SINGLETHREADTEST; i++)
	{
		Jobs::CreateJobAndCount(Test1b, nullptr, JOBFLAG_NONE, counter);
	}
	Jobs::JoinUntilCompleted(counter);
	Jobs::Stop();
}

void Test2c(void* data)
{
	// Do some task
	uint64_t total = 0;
	for (int i = 0; i < 123456; i++)
	{
		total += i;
	}
	count.fetch_add(total);
}

// Creates several small jobs that will each increment the counter
void Test2b(void* data)
{
	JobCounterPtr* counter = (JobCounterPtr*)data;
	for (int i = 0; i < NUM_JOBS_NEST_B; i++)
	{
		Jobs::CreateJobAndCount(Test2c, nullptr, JOBFLAG_NONE, *counter);
	}
}

// Creates several jobs that will create more jobs
void Test2a(void* data)
{
	m_jobsDone = 0;
	JobCounterPtr counter = Jobs::GetNewJobCounter();
	for (int i = 0; i < NUM_JOBS_NEST_A; i++)
	{
		Jobs::CreateJobAndCount(Test2b, (void*)&counter, JOBFLAG_NONE, counter);
	}
	Jobs::JoinUntilCompleted(counter);
	// Stop once all jobs are done
	Jobs::Stop();
}

int main()
{
	// Single-thread test
	std::cout << "Starting single-thread test" << std::endl;
	count = 0;
	auto start = std::chrono::system_clock::now();
	Jobs singleThreadTest(1, Test1a, nullptr);
	auto end = std::chrono::system_clock::now();
	auto elapsed = end - start;
	std::cout << "Single-thread test completed in " << elapsed.count() << "ns" << "(Result: " << count << ")" << std::endl;

	// Multi-thread test
	std::cout << "Starting multi-thread test" << std::endl;
	count = 0;
	start = std::chrono::system_clock::now();
	Jobs multiThreadTest(12, Test1a, nullptr);
	end = std::chrono::system_clock::now();
	elapsed = end - start;
	std::cout << "Multi-thread test completed in " << elapsed.count() << "ns" << "(Result: " << count << ")" << std::endl;

	// Nested jobs test
	std::cout << "Starting nested job test" << std::endl;
	count = 0;
	start = std::chrono::system_clock::now();
	Jobs nestedJobTest(12, Test2a, nullptr);
	end = std::chrono::system_clock::now();
	elapsed = end - start;
	std::cout << "nested job test completed in " << elapsed.count() << "ns" << "(Result: " << count << ")" << std::endl;

	return 0;
}