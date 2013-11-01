#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "mqtts.h"
#include "handle.h"

static void should_handle_unknown_return_0()
{
    mqtts_t* mqtts;
    mqtts = malloc(sizeof (mqtts_t));
    memset (mqtts->conns, 0, sizeof (mqtts->conns));
    mqtts->available_conns_index = NULL;
    assert_int_equal(handle_connect (mqtts, 0, 0, 0, 0), 0);
}


static void should_handle_connect_return_0()
{
    /*
        when mqtts->conns[sock].package_in.pos be equals to remaining_length + fixed_header_length,
        this func returns 0
    */
    mqtts_t* mqtts;
    mqtts = malloc(sizeof (mqtts_t));
    memset (mqtts->conns, 0, sizeof (mqtts->conns));
    mqtts->available_conns_index = NULL;
    assert_int_equal(handle_connect (mqtts, 0, 0, 0, 0), 0);
}

static void should_handle_connect_smoke()
{
    mqtts_t* mqtts;
    mqtts = malloc(sizeof (mqtts_t));
    memset (mqtts->conns, 0, sizeof (mqtts->conns));
    mqtts->available_conns_index = NULL;
    mqtts->conns[0].package_in.buf = malloc(512);
    mqtts->conns[0].package_in.buf = "\020\f\000\006MQIsdp\003\002\000\n\000";
    handle_connect (mqtts, 0, 12, 2, 1);
}

static void should_handle_smoke()
{
    mqtts_t* mqtts;
    mqtts = malloc(sizeof (mqtts_t));
    memset (mqtts->conns, 0, sizeof (mqtts->conns));
    mqtts->available_conns_index = NULL;
    mqtts->conns[0].package_in.buf = malloc(512);
    mqtts->conns[0].package_in.buf = "\020\f\000\006MQIsdp\003\002\000\n\000";
    handle (mqtts, 0);
}


static void should_get_handler_returns_right()
{

    assert_string_equal(get_handler (0x10), handle_connect);
    assert_string_equal(get_handler (0x20), handle_unknown);
}


int main(void) {
    const UnitTest test_clear_connection[] = {
        unit_test(should_handle_unknown_return_0),
        unit_test(should_handle_connect_return_0),
        unit_test(should_handle_connect_smoke),
        unit_test(should_handle_smoke),
        unit_test(should_get_handler_returns_right)
    };
    return run_tests(test_clear_connection);
}
