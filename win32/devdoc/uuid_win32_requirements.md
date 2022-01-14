uuid requirements
=================

## Overview
The uuid module generates unique IDs.

## References

[RFC 4122](https://datatracker.ietf.org/doc/html/rfc4122)

## Exposed API

```c
typedef unsigned char UUID_T[16]; /*introduces UUID_T as "array of 16 bytes"*/
    MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);

#ifdef WIN32 /*some functions only exists in Windows realm*/
    MOCKABLE_FUNCTION(, int, uuid_from_GUID, UUID_T, destination, const GUID*, source);
    MOCKABLE_FUNCTION(, int, GUID_from_uuid, GUID*, destination, const UUID_T, source);
#endif

/* These 2 strings can be conveniently used directly in printf-like statements
  Notice that PRI_UUID_T has to be used like any other print format specifier, meaning it
  has to be preceded with % */

#define PRI_UUID_T        "02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

#define UUID_VALUES(uuid) \
    (uuid)[0], (uuid)[1], (uuid)[2], (uuid)[3], (uuid)[4], (uuid)[5], (uuid)[6], (uuid)[7], \
    (uuid)[8], (uuid)[9], (uuid)[10], (uuid)[11], (uuid)[12], (uuid)[13], (uuid)[14], (uuid)[15]

#define UUID_VALUES_OR_NULL(uuid) \
    ((uuid) == NULL) ? 0 : (uuid)[0], ((uuid) == NULL) ? 0 : (uuid)[1], ((uuid) == NULL) ? 0 : (uuid)[2], ((uuid) == NULL) ? 0 : (uuid)[3], \
    ((uuid) == NULL) ? 0 : (uuid)[4], ((uuid) == NULL) ? 0 : (uuid)[5], ((uuid) == NULL) ? 0 : (uuid)[6], ((uuid) == NULL) ? 0 : (uuid)[7], \
    ((uuid) == NULL) ? 0 : (uuid)[8], ((uuid) == NULL) ? 0 : (uuid)[9], ((uuid) == NULL) ? 0 : (uuid)[10], ((uuid) == NULL) ? 0 : (uuid)[11], \
    ((uuid) == NULL) ? 0 : (uuid)[12], ((uuid) == NULL) ? 0 : (uuid)[13], ((uuid) == NULL) ? 0 : (uuid)[14], ((uuid) == NULL) ? 0 : (uuid)[15] \

```

###  uuid_produce
```c
MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);
```

`uuid_produce` fills destination's bytes with a unique ID.

**SRS_UUID_WIN32_02_001: [** If `destination` is `NULL` then `uuid_produce` shall fail and return a non-NULL value. **]**

**SRS_UUID_WIN32_02_002: [** `uuid_produce` shall call `UuidCreate` to generate a `UUID`. **]**

**SRS_UUID_WIN32_02_003: [** `uuid_produce` shall copy the generated `UUID`'s bytes in `destination`. **]**

**SRS_UUID_WIN32_02_004: [** `uuid_produce` shall succeed and return 0. **]**

**SRS_UUID_WIN32_02_005: [** If there are any failures then `uuid_produce` shall fail and return a non-zero value. **]**

### uuid_from_GUID
```c
MOCKABLE_FUNCTION(, int, uuid_from_GUID, UUID_T, destination, const GUID*, source);
```

`uuid_from_GUID` convers a Windows `GUID` to a `UUID_T`.

**SRS_UUID_WIN32_02_006: [** If `destination` is `NULL` then `uuid_from_GUID` shall fail and return a non-zero value. **]**

**SRS_UUID_WIN32_02_007: [** If `source` is `NULL` then `uuid_from_GUID` shall fail and return a non-zero value. **]**

**SRS_UUID_WIN32_02_008: [** `uuid_from_GUID` shall convert `GUID` to `UUID_T`, succeed and return 0. **]**

### GUID_from_uuid
```c
    MOCKABLE_FUNCTION(, int, GUID_from_uuid, GUID*, destination, const UUID_T, source);
```

**SRS_UUID_WIN32_02_009: [** If `destination` is `NULL` then `GUID_from_uuid` shall fail and return a non-zero value. **]**

**SRS_UUID_WIN32_02_010: [** If `source` is `NULL` then `GUID_from_uuid` shall fail and return a non-zero value. **]**

**SRS_UUID_WIN32_02_011: [** `GUID_from_uuid` shall convert `UUID_T` to `GUID`, succeed and return 0. **]**