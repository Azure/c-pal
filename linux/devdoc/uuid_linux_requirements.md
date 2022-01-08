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

```
###  uuid_produce
```C
MOCKABLE_FUNCTION(, int, uuid_produce, UUID_T, destination);
```

`uuid_produce` fills destination's bytes with a unique ID.

**SRS_UUID_LINUX_02_001: [** If `destination` is `NULL` then `uuid_produce` shall fail and return a non-NULL value. **]**

**SRS_UUID_LINUX_02_002: [** `uuid_produce` shall call `uuid_generate` to generate a `UUID`. **]**

**SRS_UUID_LINUX_02_004: [** `uuid_produce` shall succeed and return 0. **]**