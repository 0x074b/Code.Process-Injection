#pragma section(".text")

__declspec(allocate(".text")) char goodcode[] =
"shellcode";

int main()
{
    (*(void(*)())(&goodcode))();
}
