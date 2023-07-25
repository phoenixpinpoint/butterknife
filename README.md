# butterknife
A simple HTML Templating Engine for C. 

Butterknife is blade template (https://laravel.com/docs/10.x/blade) like system for templating and rendering HTML. 

### Technology
- [clibs/clib] - fantastic C Package Manager(ish).
- [jwerkle/fs] - node like fs tools to ease the fs burden.
- [kokke/tiny-regex-c] - simple, small regular expression tool.
- [likle/cwalk] - a useful os file path tool.
- [rxi/vec] - great vector type for C (Archived but publicly available). 

## use
Install with CLib or copy butterknife.c and butterknife.h and the deps folder into your project.

```bash
clib install
```


After which you can create a basic layout:
```html
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
<body>
    @yields content;
</body>
    @yields footer;
</html>
```
And a basic page:
```html
@layout layout.bk.html;
@head
    <title>page</title>
@headclose
@section content 
<div>
    <h1>Butterknife</h1>
    <p>Here is a butterknife section called content.</p>
</div><!--Content Div-->
@sectionclose
@section footer
<footer>
<p>Copyright 2023.</p>
</footer>
@sectionclose
```
If our example code in in the ./example directory (hint hint) we could use the following program to render the page and output it the console.

```C
#include "butterknife.c"

int main()
{
    buffer_t* page = bk_generate_webpage("./example/page.bk.html");
    printf("Page: \n%s\n", page->data);
}
```
To build and run:
```bash
gcc app.c -o app
./app
```

## Built-in debugging [WIP]
In butterknife.h uncomment:
```C
#define BK_DEBUG
```

[//]: # 
[clibs/clib]: https://github.com/clibs/clib
[jwerkle/fs]: https://github.com/jwerle/fs.c
[kokke/tiny-regex-c]: https://github.com/kokke/tiny-regex-c
[likle/cwalk]: https://github.com/likle/cwalk
[rxi/vec]: https://github.com/rxi/vec
