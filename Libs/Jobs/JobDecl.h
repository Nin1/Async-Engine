#pragma once

/**
 * Macro to declare a job in a class in the header file. Use DEFINE_CLASS_JOB to define that job.
 * The job cannot have parameters.
 */
#define DECLARE_CLASS_JOB(className, jobName) \
void _##jobName(); \
static void jobName(void* data) { static_cast<className*>(data)->_##jobName(); } \

/** Macro to define a class job in a .cpp file with no parameters. */
#define DEFINE_CLASS_JOB(className, jobName) void className::_##jobName()


/** Macro to declare a job function. */
#define DECLARE_JOB_FUNCTION(funcName) static void funcName(void* data)

/**
 * Macro to define a job function.
 * @param funcName - The name of the function
 * @param dataType - The data type that the function expects
 * @param dataName - The name of the data that the function receives
 */
#define JOB_FUNCTION_START(funcName, dataType, dataName) void funcName(void* data) \
{ \
	dataType* dataName = static_cast<dataType*>(data);

 /** Macro to declare the end of a job function. */
#define JOB_FUNCTION_END() }