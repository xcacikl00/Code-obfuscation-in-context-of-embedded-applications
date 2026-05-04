
void do_something()
{
   volatile  int x = 0;
    for (int i = 0; i < 10; i++)
    {
        x += i;
    }
}

void do_something_else()
{
    volatile int y = 1;
    for (int j = 1; j < 5; j++)
    {
        y *= j;
    }

    if (y > 10)
    {
        y = y - 3;
    }
    else
    {
        y = y + 3;
    }

    if (y > 10)
    {
        y = y - 3;
    }
    else
    {
        y = y + 3;
    }

    if (y > 10)
    {
        y = y - 3;
    }
    else
    {
        y = y + 3;
    }
}

void _start()
{
    volatile int a = 5;
    volatile int b = 10;
    volatile int c = a + b;

    if (c > 10)
    {
        c = c * 2;
        c = c - 5;
    }
    else
    {
        c = c / 2;
        do_something();
    }

    do_something_else();

    while (1)        ;
}
