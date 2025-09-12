#include "ble.h"
#include <stdio.h>


int scan_ble(void)
{
	size_t adapter_count = simpleble_adapter_get_count();
    if (adapter_count == 0) {
        printf("No adapter was found.\n");
        return 1;
    }
	else
	{
		printf("Found adapter\n");
	}
	// TODO: Allow the user to pick an adapter.
    simpleble_adapter_t adapter = simpleble_adapter_get_handle(0);
    if (adapter == NULL) {
        printf("No adapter was found.\n");
        return 1;
    }

}