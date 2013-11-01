#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "mqtts.h"
#include "net.h"


static void should_clear_connection_return_0(void **state)
{
    mqtts_t* mqtts;
    mqtts = malloc(sizeof (mqtts_t));
    memset (mqtts->conns, 0, sizeof (mqtts->conns));
    mqtts->available_conns_index = NULL;
    assert_int_equal(clear_connection (mqtts, 0), 0);
}

static void should_clear_connection_memset_connection_to_0(void **state)
{
    int i;
    mqtts_t* mqtts;
    mqtts = malloc(sizeof (mqtts_t));
    memset (mqtts->conns, 0, sizeof (mqtts->conns));
    mqtts->available_conns_index = NULL;
    clear_connection (mqtts, 0);
    for (i = 0; i < MQTTS_MAXEVENTS; ++i)
    {
        assert_int_equal(mqtts->conns[i].fd, 0);
    }

}

int main(void) {
    const UnitTest test_clear_connection[] = {
        unit_test(should_clear_connection_return_0),
        unit_test(should_clear_connection_memset_connection_to_0)
    };
    return run_tests(test_clear_connection);
}
