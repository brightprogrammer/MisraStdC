/// file      : Tests/Std/SymbolResolver.BiasLib.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Fixture shared object for the `p_vaddr != p_offset` load-bias path. Built
/// with a 64 KiB max-page-size so the later PT_LOAD segments carry a p_vaddr
/// ahead of their p_offset (the AArch64 `max-page-size` layout, reproduced on
/// x86-64). `symres_bias_data` lives in that gapped RW segment; the accessor
/// hands back its real in-.so address so no copy relocation drags it into the
/// main image, keeping the resolved address inside the gapped mapping.

long symres_bias_data[32] = {1};

void *symres_bias_data_addr(void) {
    return (void *)&symres_bias_data[0];
}
