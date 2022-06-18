#include "error.h"

#include <stdio.h>

#include "constants.h"

#ifdef _DEBUG
char breakOnError = FALSE;

void SetShouldBreakOnError(char enable)
{
	breakOnError = enable;
}
#endif // _DEBUG

char printOnError = TRUE;

void SetShouldPrintOnError(char enable)
{
	printOnError = enable;
}

const char ErrorMap[ErrorCount][MAX_ERROR_MESSAGE_LENGTH] = 
{ 
	"No Error\0", 
	"Invalid Function Arguments\0", 
	"Buddy Allocator Out Of Memmory\0", 
	"Memory managment metadata is not initialized\0",
	"Object doesnt exsist in any slab\0",
	"Invalid object pointer\0",
	"Can not delete system caches\0",
	"Cache does not exsist\0",
	"Buffer Order is out of bounds\0"
};

const char* latestError = NULL;

void HandleError(EErrorType errorType)
{
	if (errorType == ErrorNoError)
	{
		return;
	}

	latestError = ErrorMap[errorType];
	if (printOnError)
	{
		printf("LOG[ERROR]: %s\n", latestError);
	}

#ifdef _DEBUG
	if (breakOnError)
	{
		__debugbreak();
	}
#endif // _DEBUG
}

const char* GetLatestError()
{
	return latestError;
}