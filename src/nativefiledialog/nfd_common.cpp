#include "nfd.h"
#include <string>

static std::string g_errorstr;

/* public routines */
std::string NFD::GetError() {
	return g_errorstr;
}

/* internal routines */
void NFDi_SetError(const char* msg) {
	g_errorstr = msg;
}

int NFDi_IsFilterSegmentChar(char ch) {
	return (ch == ',' || ch == ';' || ch == '\0');
}