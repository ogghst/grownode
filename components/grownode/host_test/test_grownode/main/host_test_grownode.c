/* test_mean.c: Implementation of a testable component.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <limits.h>
#include "unity.h"
#include "grownode.h"


void test_init() {
    TEST_ASSERT_EQUAL(GN_CONFIG_STATUS_OK, gn_init()->status);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(test_init);

    int failures = UNITY_END();
    return failures;

}

