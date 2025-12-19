# uuid requirements

## Overview

The uuid module generates unique IDs.

## References

[RFC 4122](https://datatracker.ietf.org/doc/html/rfc4122)

## Exposed API

```C

typedef unsigned char UUID_T[16]; /*introduces UUID_T as "array of 16 bytes"*/
MOCKABLE_FUNCTION_WITH_RETURNS(, int, uuid_produce, UUID_T*, destination)(0, MU_FAILURE);
MOCKABLE_FUNCTION(, bool, is_uuid_nil, const UUID_T, uuid_value);

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

### uuid_produce

```C
MOCKABLE_FUNCTION_WITH_RETURNS(, int, uuid_produce, UUID_T*, destination)(0, MU_FAILURE);
```

`uuid_produce` fills destination's bytes with a unique ID.

**SRS_UUID_LINUX_02_001: [** If `destination` is `NULL` then `uuid_produce` shall fail and return a non-zero value. **]**

**SRS_UUID_LINUX_02_002: [** `uuid_produce` shall call `uuid_generate` to generate a `UUID`. **]**

**SRS_UUID_LINUX_02_004: [** `uuid_produce` shall succeed and return 0. **]**

### is_uuid_nil

```C
MOCKABLE_FUNCTION(, bool, is_uuid_nil, const UUID_T, uuid_value);
```

`is_uuid_nil` determined if the specified uuid `uuid_value` is `NULL`.

**SRS_UUID_LINUX_11_002: [** If all the values of `is_uuid_nil` are `0` then `is_uuid_nil` shall return `true`. **]**

**SRS_UUID_LINUX_11_003: [** If any the values of `is_uuid_nil` are not `0` then `is_uuid_nil` shall return `false`. **]**
