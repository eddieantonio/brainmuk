#include "greatest.h"


TEST placeholder_test(void) {
    FAILm("(no tests defined...)");
}

TEST skipped_test(void) {
    SKIP();
}

/* Add all the definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */

    RUN_TEST(skipped_test);
    RUN_TEST(placeholder_test);

    GREATEST_MAIN_END();        /* display results */
}
