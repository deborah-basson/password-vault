#include <stdlib.h>
#include <unistd.h>

__attribute__((destructor))
static void cleanup_server(void) {
    system("pkill -f password_vault 2>/dev/null");
}
