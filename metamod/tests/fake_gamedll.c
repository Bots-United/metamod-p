static volatile char padding[64];
void GiveFnptrsToDll(void *pFuncs, void *pGlobals) { (void)pFuncs; (void)pGlobals; (void)padding; }
int GetEntityAPI2(void *pTable, int *version) { (void)pTable; (void)version; return 1; }
