uuid requirements
=================

## Overview
The uuid module generates unique IDs.

## References

[RFC 4122](https://datatracker.ietf.org/doc/html/rfc4122)

## Exposed API
```C

typedef unsigned char UUID_T[16]; /*introduces UUID_T as "array of 16 bytes"*/
MOCKABLE_FUNCTION(, int, uuid_generate, UUID_T, destination);

```
###  uuid_generate
```C
MOCKABLE_FUNCTION(, int, uuid_generate, UUID_T, destination);
```

`uuid_generate` fills destination's bytes with a unique ID.

**SRS_UUID_02_001: [** If `destination` is `NULL` then `uuid_generate` shall fail and return a non-NULL value. **]**

**SRS_UUID_02_002: [** `uuid_generate` shall generate in `destination` the representation of a UUID (as per RFC 4122). **]**

**SRS_UUID_02_004: [** `uuid_generate` shall succeed and return 0. **]**