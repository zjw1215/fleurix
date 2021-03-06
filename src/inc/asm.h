/* all inlined. */

struct pde;

/* 
 * iodelay. On older machines its necessary to give the PIC some time to react to 
 * commands as they might not be processed quickly.
 * */
static inline void io_delay(){
    asm volatile (
        "jmp 1f; 1: jmp 1f; 1:"
        );
}

/* inb & outb */
static inline uchar inb(ushort port){
    uchar ret;
    asm volatile( "inb %1, %0" : "=a" (ret) : "dN" (port));
    io_delay();
    return ret;
}

static inline void outb(ushort port, uchar data){
    asm volatile( "outb %1, %0" :: "dN" (port), "a" (data));
    io_delay();
}

/* inw & outw */
static inline uchar inw(ushort port){
    ushort ret;
    asm volatile( "inw %1, %0" : "=a" (ret) : "dN" (port));
    io_delay();
    return ret;
}

static inline void outw(ushort port, ushort data){
    asm volatile( "outw %1, %0" :: "dN" (port), "a" (data));
    io_delay();
}

/* insb & outsb */
static inline void insb(uint port, void *addr, int cnt) {
	asm volatile(
            "cld;" "repne;" "insb"
			 :"=D" (addr), "=c" (cnt)
			 :"d" (port), "0" (addr), "1" (cnt)
			 :"memory", "cc");
    io_delay();
}

static inline void outsb(uint port, void *addr, int cnt) {
	asm volatile("cld;" "repne;" "outsb"
			 :"=S" (addr), "=c" (cnt)
			 :"d" (port), "0" (addr), "1" (cnt)
			 :"cc");
    io_delay();
}

/* insl & outsl */
static inline void insl(uint port, void *addr, int cnt) {
	asm volatile(
            "cld;" "repne;" "insl"
			 :"=D" (addr), "=c" (cnt)
			 :"d" (port), "0" (addr), "1" (cnt)
			 :"memory", "cc");
    io_delay();
}

static inline void outsl(uint port, void *addr, int cnt) {
	asm volatile("cld;" "repne;" "outsl"
			 :"=S" (addr), "=c" (cnt)
			 :"d" (port), "0" (addr), "1" (cnt)
			 :"cc");
    io_delay();
}

/* -------------------------------------------- */

/* load TSS into tr */
static inline void ltr(uint n){
    asm volatile("ltr %%ax"::"a"(n));
}

static inline void lldt(uint n){
    asm volatile("lldt %%ax"::"a"(n));
}

/* flush the page directory */
static inline void lpgd(struct pde *pgdir){
    asm volatile("mov %%eax, %%cr3":: "a"((uint)pgdir));
}

/* set the cr0 bit and enable mmu */
static inline void mmu_enable(){
    uint cr0, cr4;
    asm volatile("mov %%cr4, %0": "=r"(cr4));
    cr4 |= 0x10; // set PSE, enable 4mb page
    asm volatile("mov %0, %%cr4":: "r"(cr4));
    //
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // set the paging bit.
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

/* ------------------------------------------------- */


/* load TSS into tr */
static inline void cli(){
    asm volatile("cli");
}

static inline void sti(){
    asm volatile("sti");
}

/* ---------------------------------------------- */


/*
 * ljmp seg, offset...Lovely little instruction.
 * But seg seems only availible for immediate value at compile time.
 * Tricks needed, *Sucks*.
 * */
static inline void ljmp(ushort seg, uint offset){
    struct{ uint offset, seg; } _tmp;
    _tmp.offset = offset;
    _tmp.seg    = seg;
    asm volatile(
        "ljmp %0"
        ::"m"(_tmp)
        );
}

static inline void lcall(ushort seg, uint offset){
    struct{ uint offset, seg; } _tmp;
    _tmp.offset = offset;
    _tmp.seg    = seg;
    asm volatile(
        "lcall %0"
        ::"m"(_tmp)
        );
}
