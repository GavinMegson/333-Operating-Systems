// Simple debug log macro
//
// example usage:
// LOG("Added to ready list")
#define LOG(msg) cprintf("%s line %d: %s\n", __func__, __LINE__, msg);

// Log with a format string for custom logging
//
// example usage:
// LOG_S("setting UID pid %d uid %d", uid, gid)
#define LOG_S(fmt, ...)                                                   \
  ({do { if (DEBUG) cprintf("%s %s:%d " fmt, __FILE__, __func__, __LINE__, __VA_ARGS__); } while (0);})
