void* NewLayout(HWND hWnd, HINSTANCE hInst, LPCTSTR xml);
void DeleteLayout(void* ptr);
void SizeLayout(void* ptr, const SIZE* sz);
void VisibaleLayout(void* ptr, LPCTSTR name, BOOL bVisible);