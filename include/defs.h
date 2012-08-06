#pragma once

#define TP_CONCAT_INNER(a,b) a##b
#define TP_CONCAT(a,b) TP_CONCAT_INNER(a,b)
#define TP_LINE_NAME(prefix) TP_CONCAT(prefix, __LINE__)
#define TP_UNIQUE_NAME(prefix) TP_CONCAT(prefix, __COUNTER__)

