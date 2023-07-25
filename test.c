#include "butterknife.c"

int main()
{
    buffer_t* page = bk_generate_webpage("./example/page.bk.html");
    printf("Page: \n%s\n", page->data);
}