#include "internal.h"

#ifdef NDEBUG
void	yoyo_assert(const char* strexp, bool exp, const char* file, unsigned int line, const char* func) {
	(void)strexp;
	(void)exp;
	(void)file;
	(void)line;
	(void)func;
}
#else
void	yoyo_assert(const char* strexp, bool exp, const char* file, unsigned int line, const char* func) {
	if (exp) { return; }
	yoyo_dprintf(STDERR_FILENO, "assertion failed: `%s' at %s:%u %s\n", strexp, file, line, func);
	abort();
}
#endif
