uuid requirements
=================

## Overview
The uuid module generates unique IDs.

## References

[RFC 4122](https://datatracker.ietf.org/doc/html/rfc4122)

## Exposed API
```C

typedef unsigned char UUID_T[16]; /*introduces UUID_T as "array of 16 bytes"*/

    MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);

#ifdef WIN32 /*some functions only exists in Windows realm*/
    MOCKABLE_FUNCTION(, int, uuid_from_GUID, UUID_T, destination, const GUID*, source);
#endif

/* These 2 strings can be conveniently used directly in printf-like statements
  Notice that PRI_UUID has to be used like any other print format specifier, meaning it
  has to be preceded with % */

#define PRI_UUID_T        "02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

#define UUID_T_VALUES(uuid) \
    (uuid)[0], (uuid)[1], (uuid)[2], (uuid)[3], (uuid)[4], (uuid)[5], (uuid)[6], (uuid)[7], \
    (uuid)[8], (uuid)[9], (uuid)[10], (uuid)[11], (uuid)[12], (uuid)[13], (uuid)[14], (uuid)[15]

#define UUID_T_VALUES_OR_NULL(uuid) \
    ((uuid) == NULL) ? 0 : (uuid)[0], ((uuid) == NULL) ? 0 : (uuid)[1], ((uuid) == NULL) ? 0 : (uuid)[2], ((uuid) == NULL) ? 0 : (uuid)[3], \
    ((uuid) == NULL) ? 0 : (uuid)[4], ((uuid) == NULL) ? 0 : (uuid)[5], ((uuid) == NULL) ? 0 : (uuid)[6], ((uuid) == NULL) ? 0 : (uuid)[7], \
    ((uuid) == NULL) ? 0 : (uuid)[8], ((uuid) == NULL) ? 0 : (uuid)[9], ((uuid) == NULL) ? 0 : (uuid)[10], ((uuid) == NULL) ? 0 : (uuid)[11], \
    ((uuid) == NULL) ? 0 : (uuid)[12], ((uuid) == NULL) ? 0 : (uuid)[13], ((uuid) == NULL) ? 0 : (uuid)[14], ((uuid) == NULL) ? 0 : (uuid)[15] \


```
###  uuid_produce
```C
MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);
```

`uuid_produce` fills destination's bytes with a unique ID.

**SRS_UUID_02_001: [** If `destination` is `NULL` then `uuid_produce` shall fail and return a non-NULL value. **]**

**SRS_UUID_02_002: [** `uuid_produce` shall generate in `destination` the representation of a UUID (as per RFC 4122). **]**

**SRS_UUID_02_004: [** `uuid_produce` shall succeed and return 0. **]**

### uuid_from_GUID
```c
MOCKABLE_FUNCTION(, int, uuid_from_GUID, UUID_T, destination, const GUID*, source);
```

`uuid_from_GUID` converts a `GUID` to a `UUID_T`. This function only exists on Windows platforms. 

See [uuid requirements](../../win32/devdoc/uuid_win32_requirements.md) for further requirements.

