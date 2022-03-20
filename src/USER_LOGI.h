#ifndef _USER_LOGI_H
#define _USER_LOGI_H
#define TRACE_LOG(LEVEL, INFO) Serial.printf("[%c][%s:%d] %s:%s\n", ((LEVEL == 0) ? ('I') : ((LEVEL == 1) ? ('W') : ('E'))), __FILE__, __LINE__, __func__, INFO)
#endif