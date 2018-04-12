#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include "string-dict.h"

#define STRING_DICT_DELIMIT '|'
#define STRING_DICT_EQUAL '='

typedef struct string_dict_internal_t {
    unsigned data_buffer_size; // size of buffer pointed by data
    char *data;                // null-terminated string
} string_dict_internal;

// return value:
//   NOT FOUND: return = 1
//   FOUND: return = 0
static int _dictSearch(char *data, const char *key, char **ppKey, char **ppValue, char **ppValueEnd)
{
    char keyExt[STRING_DICT_MAX_KEY_VALUE_LEN + 8] = {0};
    char * ptrKey = NULL;
    char * ptrValue = NULL;
    char * ptrValueEnd = NULL;

    sprintf_s(keyExt, sizeof(keyExt), "%s%c", key, STRING_DICT_EQUAL);
    ptrKey = strstr(data, keyExt);
    if (!ptrKey)
        return 1;

    ptrValue = strchr(ptrKey, STRING_DICT_EQUAL);
    assert(ptrValue);
    ptrValue++;

    ptrValueEnd = strchr(ptrValue, STRING_DICT_DELIMIT);
    assert(ptrValueEnd);

    *ppKey = ptrKey;
    *ppValue = ptrValue;
    *ppValueEnd = ptrValueEnd;

    return 0;
}

// return value:
//   NOT FOUND: return = 1
static int _dictSearchConst(const char *data, const char *key, const char **ppKey, const char **ppValue, const char **ppValueEnd)
{
    char keyExt[STRING_DICT_MAX_KEY_VALUE_LEN + 8] = {0};
    const char * ptrKey = NULL;
    const char * ptrValue = NULL;
    const char * ptrValueEnd = NULL;

    sprintf_s(keyExt, sizeof(keyExt), "%s%c", key, STRING_DICT_EQUAL);
    ptrKey = strstr(data, keyExt);
    if (!ptrKey)
        return 1;

    ptrValue = strchr(ptrKey, STRING_DICT_EQUAL);
    assert(ptrValue);
    ptrValue++;

    ptrValueEnd = strchr(ptrValue, STRING_DICT_DELIMIT);
    assert(ptrValueEnd);

    *ppKey = ptrKey;
    *ppValue = ptrValue;
    *ppValueEnd = ptrValueEnd;

    return 0;
}

static void _dictDeleteValue(char *data, char *ptrKey, char *ptrValueEnd)
{
    int copy = strlen(data) - (ptrValueEnd - data + 1);
    assert(copy >= 0);

    if (copy > 0) {
        memmove(ptrKey, ptrValueEnd+1, copy);
    }
    *(ptrKey + copy) = '\0';
}

// Implement a simple string-string dict.
int dictInitialize(string_dict *dict)
{
    string_dict_internal * pidict = (string_dict_internal *)malloc(sizeof(string_dict_internal));
    if (!pidict) {
        return -1;
    }
    memset(pidict, 0, sizeof(string_dict_internal));
    pidict->data_buffer_size = 32 * 1024;
    pidict->data = (char*)malloc(pidict->data_buffer_size);
    if (!pidict->data) {
        free(pidict);
        return -1;
    }
    memset(pidict->data, 0, pidict->data_buffer_size);
    *dict = (void*)pidict;
    return 0;
}

void dictDestory(string_dict dict)
{
    if (dict) {
        string_dict_internal *pidict = (string_dict_internal*)dict;
        if (pidict->data)
            free(pidict->data);
        free(dict);
    }
}

//
// return value:
//   ERROR: return < 0
//   NOT FOUND: return = 1
//
int dictDeleteValue(string_dict dict, const char *key)
{
    string_dict_internal * pidict = (string_dict_internal*)dict;
    char * ptrKey = NULL;
    char * ptrValue = NULL;
    char * ptrValueEnd = NULL;
    if (!pidict || !pidict->data)
        return -1;
    if (!key || strlen(key) == 0 || strlen(key) > STRING_DICT_MAX_KEY_VALUE_LEN)
        return -1;

    if (_dictSearch(pidict->data, key, &ptrKey, &ptrValue, &ptrValueEnd) == 1)
        return 1; // not found

    _dictDeleteValue(pidict->data, ptrKey, ptrValueEnd);

    return 0;
}

//
// return value:
//   ERROR: return < 0
//   NOT FOUND: return = 1
//   BUFF TOO SMALL: return = 2
//   FOUND: return = 0
//
int dictGetValue(const string_dict dict, const char *key, char *value, unsigned size)
{
    string_dict_internal * pidict = (string_dict_internal*)dict;
    char keyExt[STRING_DICT_MAX_KEY_VALUE_LEN + 8] = {0};
    const char * ptrKey = NULL;
    const char * ptrValue = NULL;
    const char * ptrValueEnd = NULL;
    unsigned nCopied = 0;

    if (!pidict || !pidict->data)
        return -1;
    if (!key || strlen(key) == 0 || strlen(key) > STRING_DICT_MAX_KEY_VALUE_LEN)
        return -1;
    if (strchr(key, STRING_DICT_DELIMIT) || strchr(key, STRING_DICT_EQUAL))
        return -1;
    if (!value || size <= 0)
        return -1;

    sprintf_s(keyExt, sizeof(keyExt), "%s%c", key, STRING_DICT_EQUAL);
    ptrKey = strstr(pidict->data, keyExt);
    if (!ptrKey)
        return 1; // not found

    ptrValue = strchr(ptrKey, STRING_DICT_EQUAL);
    assert(ptrValue);
    ptrValue++;

    ptrValueEnd = strchr(ptrValue, STRING_DICT_DELIMIT);
    assert(ptrValueEnd);

    while (ptrValue < ptrValueEnd && nCopied < size - 1)
    {
        *value = *ptrValue;
        value++;
        ptrValue++;
        nCopied++;
    }

    *value = '\0';

    if (ptrValue != ptrValueEnd)
        return 2; // Incompleted value string returned

    return 0;
}

int dictSetValue(string_dict dict, const char *key, const char *value)
{
    string_dict_internal * pidict = (string_dict_internal*)dict;

    if (!pidict || !pidict->data)
        return -1;
    if (!key || strlen(key) == 0 || strlen(key) > STRING_DICT_MAX_KEY_VALUE_LEN)
        return -1;
    if (!value || strlen(value) > STRING_DICT_MAX_KEY_VALUE_LEN)
        return -1;

    if (1)
    {
        char *ptrKey = NULL;
        char *ptrValue = NULL;
        char *ptrValueEnd = NULL;
        if (_dictSearch(pidict->data, key, &ptrKey, &ptrValue, &ptrValueEnd) == 0)
        {
            _dictDeleteValue(pidict->data, ptrKey, ptrValueEnd);
        }
    }

    // enlarge the data buffer automatically.
    if (strlen(pidict->data) + strlen(key) + strlen(value) + 8 /*conservative*/ > pidict->data_buffer_size) {
        void *ptrLargeBuffer = malloc(pidict->data_buffer_size * 2);
        if (!ptrLargeBuffer) {
            return -1; // out of memory
        }
        ZeroMemory(ptrLargeBuffer, pidict->data_buffer_size * 2);
        memcpy(ptrLargeBuffer, pidict->data, pidict->data_buffer_size);
        free(pidict->data);
        pidict->data = (char *)ptrLargeBuffer;
    }

    // append new node at end of data buffer.
    if (1)
    {
        char node[STRING_DICT_MAX_KEY_VALUE_LEN * 2 + 8 /*conservative*/] = {0};
        sprintf_s(node, sizeof(node), "%s%c%s%c", key, STRING_DICT_EQUAL, value, STRING_DICT_DELIMIT);
        strcat_s(pidict->data, pidict->data_buffer_size, node);
    }
    return 0;
}
