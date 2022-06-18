#ifndef _ERROR_H_
#define _ERROR_H_

typedef enum ErrorType
{
	ErrorNoError = 0,
	ErrorInvalidArguments,
	ErrorBuddyOutOfMemmory,
	ErrorNoMetadata,
	ErrorNoSlabForObject,
	ErrorInvalidObject,
	ErrorDeleteSystemCache,
	ErrorCacheNotExsist,
	ErrorBufferOrderOutOfBounds,
	ErrorCount
} EErrorType;

#define MAX_ERROR_MESSAGE_LENGTH 128

#ifdef _DEBUG
void SetShouldBreakOnError(char enable);
#endif // _DEBUG

void SetShouldPrintOnError(char enable);

void HandleError(EErrorType errorType);

const char* GetLatestError();

#endif // !_ERROR_H_