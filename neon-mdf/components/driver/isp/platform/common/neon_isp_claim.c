#include "../../neon_isp_claim.h"
#include <stdatomic.h>
#include <stddef.h>
static _Atomic(const void *) device_owner;
bool neon_isp_device_claim(const void *owner)
{
    const void *expected = NULL;
    return owner != NULL && atomic_compare_exchange_strong(&device_owner, &expected, owner);
}
bool neon_isp_device_release(const void *owner)
{
    const void *expected = owner;
    return owner != NULL && atomic_compare_exchange_strong(&device_owner, &expected, NULL);
}
bool neon_isp_device_available_to(const void *owner)
{
    const void *current = atomic_load(&device_owner);
    return current == NULL || current == owner;
}
