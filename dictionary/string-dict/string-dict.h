
// Max string length of the keys and values, not include null-terminal.
#define STRING_DICT_MAX_KEY_VALUE_LEN 63

typedef void* string_dict;

// Allocate memory and initialize the memory.
// Caller is responsible for release the memory by calling dictDestory.
// return value:
//    MALLOC FAIL: return = -1
//    OK: return = 0
int dictInitialize(string_dict *dict);

// Release the resource.
void dictDestory(string_dict dict);

// Remove a key-value pair from the dict.
// return value:
//   ERROR: return < 0
//   NOT FOUND: return = 1
//   OK: return = 0
int dictDeleteValue(string_dict dict, const char *key);

// Search the key in dict and return its value.
// return value:
//   ERROR: return < 0
//   NOT FOUND: return = 1
//   OK: return = 0
int dictGetValue(const string_dict dict, const char *key, char *value, unsigned size);

// Add a new key-value pare or change the value if the key is already existing.
// return value:
//   ERROR: return < 0
//   OK: return = 0
int dictSetValue(string_dict dict, const char *key, const char *value);

//
// Sample and testing code
//
// ==============================================
//
// string_dict dict;
// char value[STRING_DICT_MAX_KEY_VALUE_LEN + 1];
//
// dictInitialize(&dict);
//
// dictSetValue(dict, "key1", "value1");
// dictSetValue(dict, "key2", "value2");
// dictSetValue(dict, "key3", "value3");
// dictSetValue(dict, "key3", "3");
// dictSetValue(dict, "key2", "2");
// dictSetValue(dict, "key1", "1");
//
// dictGetValue(dict, "key1", value, sizeof(value));
// dictGetValue(dict, "key2", value, sizeof(value));
// dictGetValue(dict, "key3", value, sizeof(value));
//
// dictDestory(dict);
//