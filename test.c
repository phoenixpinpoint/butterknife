#include "deps/buffer/buffer.c"
#include "deps/fs/fs.c"
#include "deps/tiny-regex-c/re.c"
#include "deps/cwalk/cwalk.c"
#include "deps/vec/vec.c"

#include "butterknife.c"

int main()
{
    buffer_t* page = bk_generate_webpage("./example/page.bk.html");
    printf("Page: \n%s\n", page->data);
}