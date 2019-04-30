
#include "poe_db.h"

static poe_port_status_t m_poe_ports[POE_MAX_PORT + 1] = {{0}};

poe_port_status_t *poe_db_get_port(int lport)
{
    return &m_poe_ports[lport];
}

int  poe_db_init(void)
{
    return 0;
}