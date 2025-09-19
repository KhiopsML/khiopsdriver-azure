#pragma once

#define STRINGIFY(s) #s

#define HANDLE_ERRORS(errval, code)						                                                    \
	try													                                                    \
	{													                                                    \
		code											                                                    \
	}													                                                    \
	catch (const exception& exc)						                                                    \
	{													                                                    \
		errorLogger.LogException(exc);					                                                    \
		return (errval);									                                                \
	}													                                                    \
	catch (...)											                                                    \
	{													                                                    \
		errorLogger.LogError("unknown error (caught a value that is not a descendant of std::exception)");  \
		return (errval);									                                                \
	}
