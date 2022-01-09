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

