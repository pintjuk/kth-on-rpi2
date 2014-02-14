#include "hw.h"
#include "hyper.h"
#include "hyp_cache.h"

extern virtual_machine *curr_vm;


extern uint32_t *flpt_va;
extern uint32_t *slpt_va;

/* This is a container which keeps track of each individual page's type and reference counter
 * main memory has been divided to 2^20 pages
 * 20 must significant of each address is the index of page which has that address inside    */
extern struct page_info ph_block_state[3*256];


/* val unmap_L1_pageTable_entry_def = Define ` */
/* unmap_L1_pageTable_entry (c1, c2, c3, mem, (pgtype:bool[20]->bool[2]), (pgrefs:bool[20]->bool[30])) (va:word32) = */
/*   let l1Base_add = mmu_tbl_base_addr c2       in */
/*   let l1_idx = va >>> 20                 in */
/*   let l1_desc_add = l1Base_add !! (l1_idx ≪ 2)in */
/*   let l1_desc  = read_mem32(l1_desc_add, mem)     in */
/*   let l1_type  = mmu_l1_decode_type(l1_desc)   in */
/*   if(l1_type = 0b01w) */
/*   then */
/*       let l1_pt_desc = rec'l1PTT l1_desc                   in */
/*       let old_pointed_pt_page = l1_pt_desc.addr            in */
/*       let unmap_l1_desc = l1_desc && 0xfffffffcw           in */
/*       let mem' = write_mem32(l1_desc_add, mem, unmap_l1_desc) in */
/*       let old_refs:bool[30] = pgrefs (w2w(old_pointed_pt_page >>> 2):bool[20]) in */
/*       let pgrefs' = ((w2w(old_pointed_pt_page >>> 2):bool[20]) =+ (old_refs - 1w)) pgrefs in */
/*           (c1, c2, c3, mem', pgtype, pgrefs')  */
/*   else  */
/*       let l1_sec_desc = rec'l1SecT l1_desc in */
/*       let old_pointed_sec_page = w2w(l1_sec_desc.addr):word20 << 8 in */
/*       let unmap_l1_desc = l1_desc && 0xfffffffcw in */
/*       let mem' = write_mem32(l1_desc_add, mem, unmap_l1_desc) in */
/*       if((l1_type = 0b10w) /\ (l1_sec_desc.typ = 0b01w)) */
/*       then */
/*       	  let pgrefs' = */
/* 	      (if (l1_sec_desc.ap = 0b011w) */
/*       	      then */
/* 		  let old_sec_refs = */
/*       		     AYMAP (\n. (pgrefs (w2w(old_pointed_sec_page !! n):bool[20]), n)) */
/*       		    	   (GENARRAY (\x :bool[8].(w2w (x)):bool[20])) in */
/*       	  	          AYFOLDLI(\inPgref.\refs. */
/*       		      	    ((w2w (old_pointed_sec_page !! (SND refs)):bool[20]) =+ ((FST refs) - 1w)) inPgref */
/*       		    	  ) pgrefs old_sec_refs   */
/* 	      else  */
/* 		  pgrefs) in */
/*       	      (c1, c2, c3, mem', pgtype, pgrefs') */
/*       else */
/*       	  (c1, c2, c3, mem, pgtype, pgrefs) */
/* `; */

#define DESC_TYPE_MASK 0b11
#define UNMAPPED_ENTRY 0


#define L1_SEC_DESC_MASK 0xFFF00000
#define L1_SEC_DESC_ATTR_MASK 0x000BFFFC

#define VA_TO_L1_IDX(va) (va >> 20)
#define L1_IDX_TO_PA(l1_base, idx) (l1_base | (idx << 2))
#define PA_PT_ADD_VA(pa) (pa - (0x01000000 + HAL_PHYS_START))

#define L1_TYPE(l1_desc) (l1_desc & DESC_TYPE_MASK)

#define UNMAP_L1_ENTRY(l1_desc) (l1_desc && 0b00)
#define CREATE_L1_SEC_DESC(x, y) (L1_SEC_DESC_MASK & x) || (L1_SEC_DESC_ATTR_MASK & y) || (0b10)
#define GET_L1_AP(sec) (((sec->ap_3b) << 2) | (sec->ap_0_1bs))


#define START_PA_OF_SECTION(sec) ((sec->addr) << 20)
#define PA_OF_POINTED_PT(pt) ((pt->addr) << 10)


#define MAX_30BIT 0x3fffffff

/*register l1PTT :: bits(32)
{
  31-10:        addr
  8-5:          dom
  4:            sbz
  3:            ns
  2:            pxn
  1-0:          typ
}*/
typedef struct l1PTT_
{
  uint32_t typ          : 2;
  uint32_t pxn          : 1;
  uint32_t ns           : 1;
  uint32_t sbz          : 1;
  uint32_t dom          : 4;
  uint32_t dummy        : 1;
  uint32_t addr         : 22;
} l1PTT;

/*register l1SecT :: bits(32)
{
  31-20:        addr
  19:           ns
  18,1:         typ
  17:           ng
  16:           s
  15,11-10:     ap
  14-12:        tex
  8-5:          dom
  4:            xn
  3:            c
  2:            b
  0:            pxn
}*/
typedef struct l1SecT_
{
  uint32_t pxn          : 1;
  uint32_t typ          : 1;
  uint32_t b            : 1;
  uint32_t c            : 1;
  uint32_t xn           : 1;
  uint32_t dom          : 4;
  uint32_t dummy        : 1;
  uint32_t ap_0_1bs     : 2;
  uint32_t tex          : 3;
  uint32_t ap_3b        : 1;
  uint32_t s            : 1;
  uint32_t ng           : 1;
  uint32_t secIndic     : 1;
  uint32_t ns           : 1;
  uint32_t addr         : 12;
}l1SecT;


uint32_t hypercall_unmap_L1_pageTable_entry (addr_t  va)
{
        uint32_t l1_base_add;
        uint32_t l1_idx;
	uint32_t l1_desc_pa_add;
	uint32_t l1_desc_va_add;
	uint32_t l1_desc;
	uint32_t l1_type;

        /*Check that the guest does not override the virtual addresses used by the hypervisor */
	// HAL_VIRT_START is usually 0xf0000000, where the hypervisor code/data structures reside
        if( va >= HAL_VIRT_START)
	  return ERR_HYP_RESERVED_VA;

        if( va >= INITIAL_PT_FIXED_MAP_VA && va <= END_PT_FIXED_MAP_VA)
	  return ERR_HYP_RESERVED_VA;
	

        COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, (uint32_t)l1_base_add);
        l1_idx = VA_TO_L1_IDX(va);
	l1_desc_pa_add = L1_IDX_TO_PA(l1_base_add, l1_idx);
	l1_desc_va_add = PA_PT_ADD_VA(l1_desc_pa_add);
        l1_desc = *((uint32_t *) l1_desc_va_add);
	l1_type = L1_TYPE(l1_desc);
	// We are unmapping a PT
	if (l1_type == 1) {
	  l1PTT *l1_pt_desc = (l1PTT *) (&l1_desc);
	  *((uint32_t *) l1_desc_va_add) = UNMAP_L1_ENTRY(l1_desc);
	  uint32_t ph_block = PA_TO_PH_BLOCK(PA_OF_POINTED_PT(l1_pt_desc));
	  ph_block_state[ph_block].refs -= 1;
	}
	// We are unmapping a section
        if ((l1_type == 2) && (((l1SecT *) (&l1_desc))->secIndic == 0)) {
	  l1SecT *l1_sec_desc = (l1SecT *) (&l1_desc);
	  *((uint32_t *) l1_desc_va_add) = UNMAP_L1_ENTRY(l1_desc);
	  uint32_t ap = GET_L1_AP(l1_sec_desc);
	  int sec_idx;
	  if(ap == 3)
	    for(sec_idx = 0; sec_idx < 256; sec_idx++)  {
	      // TODO: fix also for negative numbers
	      uint32_t ph_block = PA_TO_PH_BLOCK(START_PA_OF_SECTION(l1_sec_desc)) | (sec_idx);
	      if (ph_block < 3*256)
		ph_block_state[ph_block].refs -= 1;
	    }
	}
	// nothing, since the entry was already unmapped
	else {
	  return ERR_HYP_ENTRY_UNMAPPED;
	}

	isb();
	mem_mmu_tlb_invalidate_all(TRUE, TRUE);
	mem_cache_invalidate(TRUE,TRUE,TRUE); //instr, data, writeback
	mem_cache_set_enable(TRUE);

        return 0;
}


uint32_t hypercall_map_l1_section(addr_t va, addr_t sec_base_add, uint32_t attrs)
{
       uint32_t l1_base_add;
       uint32_t l1_idx;
       uint32_t l1_desc;
       uint32_t l1_desc_va_add;
       uint32_t l1_desc_pa_add;
       uint32_t ap;
    
      /*Check that the guest does not override the virtual addresses used by the hypervisor */
      // HAL_VIRT_START is usually 0xf0000000, where the hypervisor code/data structures reside
      if( va >= HAL_VIRT_START)
         return ERR_HYP_RESERVED_VA;

      if( va >= INITIAL_PT_FIXED_MAP_VA && va <= END_PT_FIXED_MAP_VA)
         return ERR_HYP_RESERVED_VA;
      /*Check that the guest does not override the physical addresses outside its range*/

      uint32_t guest_start_pa = curr_vm->guest_info.phys_offset;
      uint32_t guest_size = curr_vm->guest_info.guest_size;
      printf("gadds %x %x\n", guest_start_pa, guest_size);
      if(!(sec_base_add >= (guest_start_pa) && sec_base_add < (guest_start_pa + guest_size )))
          return ERR_HYP_OUT_OF_RANGE_PA;             

      COP_READ(COP_SYSTEM, COP_SYSTEM_TRANSLATION_TABLE0, (uint32_t)l1_base_add);
      l1_idx = VA_TO_L1_IDX(va);
      l1_desc_pa_add = L1_IDX_TO_PA(l1_base_add, l1_idx);
      l1_desc_va_add = PA_PT_ADD_VA(l1_desc_pa_add);
      l1_desc = *((uint32_t *) l1_desc_va_add);
      if(L1_TYPE(l1_desc) != UNMAPPED_ENTRY)
          return ERR_HYP_SECTION_NOT_UNMAPPED;

      // Access permission from the give attribute
      l1_desc = CREATE_L1_SEC_DESC(sec_base_add, attrs);
      l1SecT *l1_sec_desc = (l1SecT *) (&l1_desc);
      ap = GET_L1_AP(l1_sec_desc); 

      if((ap != 2) && (ap != 3))
           return ERR_HYP_AP_UNSUPPORTED;
      if(ap == 2)
      {
        // Updating memory with the new descriptor
         *((uint32_t *) l1_desc_va_add) = l1_sec_desc;
      }
      else if(ap == 3)
       {
        int sec_idx;
       BOOL sanity_check = TRUE;
        for(sec_idx = 0; sec_idx < 256; sec_idx++)
        {
          // TODO: fix also for negative numbers
	  uint32_t ph_block = PA_TO_PH_BLOCK(START_PA_OF_SECTION(l1_sec_desc)) | (sec_idx);
          if (ph_block < 3*256)
	  {
          if((ph_block_state[ph_block].refs == MAX_30BIT) || (ph_block_state[ph_block].type != PAGE_INFO_TYPE_DATA))
           {
             sanity_check = FALSE;
           }
	  }
	  else {
	     sanity_check = FALSE;
	  }
        }
       if(!sanity_check)
          return ERR_HYP_PH_BLOCK_NOT_WRITABLE;
       for(sec_idx = 0; sec_idx < 256; sec_idx++)
        {
   	  uint32_t ph_block = PA_TO_PH_BLOCK(START_PA_OF_SECTION(l1_sec_desc)) | (sec_idx);
          ph_block_state[ph_block].refs += 1;
        }
	*((uint32_t *) l1_desc_va_add) = l1_sec_desc;
       }
       return 0;     	
}
/*Create a MB Section page
 *Guest can only map to its own domain and to its own physical addresses
 */
void hypercall_create_section(addr_t va, addr_t pa, uint32_t page_attr)
{
	uint32_t PHYS_OFFSET = curr_vm->guest_info.phys_offset;
	uint32_t guest_size = curr_vm->guest_info.guest_size;

	/*Set domain in Section page for user */
	page_attr &= ~MMU_L1_DOMAIN_MASK;
	page_attr |= (HC_DOM_KERNEL << MMU_L1_DOMAIN_SHIFT);


	/*Check virtual address*/
	if( va >= HAL_VIRT_START && va < (uint32_t)(HAL_VIRT_START+MAX_TRUSTED_SIZE))
		hyper_panic("Guest trying to map sensitive section virtual address", 1);

	/*Check physical address*/
	if(!(pa >= (PHYS_OFFSET) && pa < (PHYS_OFFSET + guest_size ))){
		hyper_panic("Guest does not own section physical address", 1);
	}

	uint32_t offset = ((va >> MMU_L1_SECTION_SHIFT)*4);
	uint32_t *pmd = (uint32_t *)((uint32_t)flpt_va + offset);

	/*We have created own mapping of first MB in hypervisor with lvl 2
	 *Do nothing if this is the page to be mapped, this is to guarantee read only access to
	 *the master page table(even though its not used )*/
	if(va == 0xc0000000){
		return;
	}

	*pmd = (pa | page_attr);
	COP_WRITE(COP_SYSTEM,COP_DCACHE_INVALIDATE_MVA, pmd);

}

#define LINUX_VA(x) ((((x - curr_vm->guest_info.phys_offset) << 4) >> 24) + 0xc00)

/*Switch page table
 * TODO Add list of allowed page tables*/
void hypercall_switch_mm(addr_t table_base, uint32_t context_id)
{
	uint32_t *l2_pt, lvl2_idx;
	uint32_t pgd_va;
	uint32_t pgd_size = 0x4000;
	uint32_t PAGE_OFFSET = curr_vm->guest_info.page_offset;

	/*First translate the physical address to linux virtual*/
	pgd_va = LINUX_VA(table_base);

	/*Check if va is inside allowed regions*/
	uint32_t va = pgd_va << 20;

	/*Check page address*/
	if( va < PAGE_OFFSET || va > (uint32_t)(HAL_VIRT_START - pgd_size) )
		hyper_panic("Table base va does not reside in kernel address space\n", 1);

	l2_pt = (uint32_t *)GET_VIRT((MMU_L1_PT_ADDR(flpt_va[pgd_va])));
	lvl2_idx = ((table_base << 12) >> 24); /*This is the index+4 to make read only*/

	/*Look if the lvl2 entry is set to read only*/
	if(l2_pt[lvl2_idx] & (1 << 4))
		hyper_panic("Guest tried to set a table base address that is not read only", 1);

	/*Switch the TTB and set context ID*/
	COP_WRITE(COP_SYSTEM,COP_CONTEXT_ID_REGISTER, 0); //Set reserved context ID
	isb();
	COP_WRITE(COP_SYSTEM,COP_SYSTEM_TRANSLATION_TABLE0, table_base); // Set TTB0
	isb();
	COP_WRITE(COP_SYSTEM,COP_CONTEXT_ID_REGISTER, context_id); //Set context ID
	isb();

}

/* Free Page table, Make it RW again
 * TODO Security critical, need to add list of used page tables
 * and have a counter for each reference so that we dont free a page table that is still
 * used by another task (Linux kernel knows not to do this but can be used in an attack)*/
void hypercall_free_pgd(addr_t *pgd)
{
//	printf("\n\tLinux kernel Free PGD: %x\n", pgd);
	uint32_t pgd_size = 0x4000;
	uint32_t PAGE_OFFSET = curr_vm->guest_info.page_offset;

	/*Check page address*/
	if( (uint32_t)pgd < PAGE_OFFSET || (uint32_t)pgd > (uint32_t)(HAL_VIRT_START - pgd_size) )
		hyper_panic("Page address not reside in kernel address space\n", 1);


	/*Make the page tables RW*/
	uint32_t lvl2_idx,  i, clean_va;
	uint32_t *l2_pt;


	/*Get level 2 page table address*/

	l2_pt = (uint32_t *)GET_VIRT(((MMU_L1_PT_ADDR(flpt_va[(uint32_t)pgd >> MMU_L1_SECTION_SHIFT]))));


	lvl2_idx = (((uint32_t)pgd << 12) >> 24); /*This is the index+4 to make read only*/


	/*First get the physical address of the lvl 2 page by
	 * looking at the index of the pgd location. Then set
	 * 4 lvl 2 pages to read only*/

    for(i=lvl2_idx; i < lvl2_idx + 4; i++){
		 l2_pt[i] |= (1 << 4 | 1 << 5); /*RW */
		 clean_va = (MMU_L2_SMALL_ADDR(l2_pt[i])) + curr_vm->guest_info.page_offset
				 	 	 	 	 	 	 	 -curr_vm->guest_info.phys_offset;

		 COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, &l2_pt[i]);
		 dsb();

		 COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA,clean_va);
		 COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, clean_va); /*Update cache with new values*/
		 dsb();
		 isb();
    }
    hypercall_dcache_clean_area((uint32_t)pgd, 0x4000);
}

/*New pages for processes, copys kernel space from master pages table
 *and cleans the cache, set these pages read only for user
 *TODO Add list of used page tables and keep count */
void hypercall_new_pgd(addr_t *pgd)
{
	uint32_t slpt_pa, lvl2_idx, i;
	uint32_t *l2_pt, clean;
	uint32_t PAGE_OFFSET = curr_vm->guest_info.page_offset;
	uint32_t PHYS_OFFSET = curr_vm->guest_info.phys_offset;
	uint32_t pgd_size = 0x4000;

	/*Check page address*/
	if( (uint32_t)pgd < PAGE_OFFSET || (uint32_t)pgd > (uint32_t)(HAL_VIRT_START - pgd_size) )
		hyper_panic("New page global directory does not reside in kernel address space\n", 1);

//	printf("\n\tLinux kernel NEW PGD: %x\n", pgd);
	/*If the requested page is in a section page, we need to modify it to lvl 2 pages
	 *so we can modify the access control granularity */
	slpt_pa = (flpt_va[(uint32_t)pgd >> MMU_L1_SECTION_SHIFT]);
	uint32_t linux_va, pgd_va;

	if(slpt_pa & MMU_L1_TYPE_SECTION) { /*Section page, replace it with lvl 2 pages*/
		pgd_va = MMU_L1_SECTION_ADDR((uint32_t)pgd) ; //Mask out everything except address
		linux_va = MMU_L1_PT_ADDR(flpt_va[(uint32_t)pgd >> MMU_L1_SECTION_SHIFT]) - PHYS_OFFSET + PAGE_OFFSET;
    	COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
		flpt_va[(uint32_t)pgd >> MMU_L1_SECTION_SHIFT] = 0; /*Clear it and replace mapping with small pages*/
		pt_create_coarse (flpt_va,pgd_va, MMU_L1_SECTION_ADDR(slpt_pa), MMU_L1_SECTION_SIZE, MLT_USER_RAM);
		l2_pt = (uint32_t *)(GET_VIRT(MMU_L1_PT_ADDR(flpt_va[(uint32_t)pgd >> MMU_L1_SECTION_SHIFT])));
		clean = (((uint32_t)pgd >> MMU_L1_SECTION_SHIFT) * 4) + (uint32_t)flpt_va;
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA,clean);
		dsb();
	}
	/*Here we already got a second lvl page*/
	else {
		l2_pt = (uint32_t *)GET_VIRT(MMU_L1_PT_ADDR(slpt_pa));
	}

	/*Get the index of the page entry to make read only*/
	lvl2_idx = (((uint32_t)pgd << 12) >> 24);

	/*First get the physical address of the lvl 2 page by
	 * looking at the index of the pgd location. Then set
	 * 4 lvl 2 pages to read only (16k page table)*/

    for(i=lvl2_idx; i < lvl2_idx + 4; i++){
    	linux_va = (MMU_L2_SMALL_ADDR(l2_pt[i])) - PHYS_OFFSET + PAGE_OFFSET;
    	COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
    	l2_pt[i] &= ~(MMU_L2_SMALL_AP_MASK << MMU_L2_SMALL_AP_SHIFT);
		l2_pt[i] |= (MMU_AP_USER_RO << MMU_L2_SMALL_AP_SHIFT); //AP = 2 READ ONLY
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA,(uint32_t)&l2_pt[i]);
		dsb();

    }

	/* Page table 0x0 - 0x4000
	 * Reset user space 0-0x2fc0
	 * 0x2fc0 = 0xBF000000 END OF USER SPACE
	 * Copy kernel, IO and hypervisor mappings
	 * 0x2fc0 - 0x4000
	 * */
	memset((void *)pgd, 0, 0x2fc0);
	memcpy((void *)((uint32_t)pgd + 0x2fC0), (uint32_t *)((uint32_t)(flpt_va) + 0x2fc0), 0x1040);

	/*Clean dcache on whole table*/
	hypercall_dcache_clean_area((uint32_t)pgd, 0x4000);
}

/*TODO Add some security check on the val
 * Sets a entry in the current L1 page table
 * If val is 0, it means clean the page so we need to check the current
 * mapped page and remove the write protection.
 * */
void hypercall_set_pmd(addr_t *pmd, uint32_t val)
{
//	printf("\n\tHYP: Set page pgd: %x val: %x \n", pgd, val );
	uint32_t offset, *l1_pt, slpt_pa, sect_idx;
	uint32_t PAGE_OFFSET = curr_vm->guest_info.page_offset;
	uint32_t PHYS_OFFSET = curr_vm->guest_info.phys_offset;
	uint32_t guest_size = curr_vm->guest_info.guest_size;

	/*Security Checks*/
	uint32_t pa = MMU_L1_SECTION_ADDR(val);

	/*Check virtual address*/
	if((uint32_t)pmd < PAGE_OFFSET || (uint32_t)pmd > (uint32_t)(HAL_VIRT_START - sizeof(uint32_t)))
		hyper_panic("Page middle directory reside outside of allowed address space !\n", 1);
	if(val != 0){
		/*Check physical address*/
		if(!(pa >= (PHYS_OFFSET) && pa < (PHYS_OFFSET + guest_size ))){
			printf("Address: %x\n", pa);
			hyper_panic("Guest does not own pte physical address", 1);
		}
	}

	/***********************************************/

	/*Swapper Page*/
	if((uint32_t)pmd >= PAGE_OFFSET + 0x4000 && (uint32_t)pmd < PAGE_OFFSET + 0x8000) {
		offset = (uint32_t)pmd - PAGE_OFFSET - 0x4000; //Pages located 0x4000 below kernel
		l1_pt = (uint32_t *)((uint32_t)flpt_va + offset);
	}
	else{ /* Here the pages are at some physical address, user process page*/
		l1_pt = pmd;
	}
	uint32_t *l1_pt_va, pte;

	/*Contains the page table entry, if the val is 0 we have to
	 *get it from the master page table */
	if(val != 0)
		pte = val;
	else
		pte = l1_pt[0];
	sect_idx = (((pte - PHYS_OFFSET) >> MMU_L1_SECTION_SHIFT)*4) + 0x3000;
	l1_pt_va = (uint32_t *)(sect_idx + (uint32_t)flpt_va);

	slpt_pa = *l1_pt_va;

	if(slpt_pa & MMU_L1_TYPE_SECTION) { /*Section page, replace it with lvl 2 pages*/
		uint32_t linux_va = MMU_L2_SMALL_ADDR(pte) - PHYS_OFFSET + PAGE_OFFSET;
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
		*l1_pt_va = 0; /*Clear it and replace mapping with small pages*/
		pt_create_coarse (flpt_va, (sect_idx * 4) << 16 , MMU_L2_SMALL_ADDR(slpt_pa), MMU_L1_SECTION_SIZE, MLT_USER_RAM);
		uint32_t clean;
		if((uint32_t)pmd > 0xc0008000)
			clean = (((uint32_t)pmd >> MMU_L1_SECTION_SHIFT) * 4) + (uint32_t)flpt_va;
		else
			clean = (((uint32_t)pmd - 0xc0004000) + (uint32_t)flpt_va);
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA,clean);
		dsb();
	}

	if(val != 0){
		uint32_t linux_va = val - PHYS_OFFSET + PAGE_OFFSET;
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();
		l1_pt[0] = val;
		l1_pt[1] = (val + 256 * 4); //(4 is the size of each entry)
		/*Flush entry*/
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, (uint32_t)l1_pt);
		dsb();

	}
	else{
		uint32_t linux_va = l1_pt[0] - PHYS_OFFSET + PAGE_OFFSET;
		COP_WRITE(COP_SYSTEM, COP_TLB_INVALIDATE_MVA, linux_va);
		COP_WRITE(COP_SYSTEM, COP_BRANCH_PRED_INVAL_ALL, linux_va);
		dsb();
		isb();

		l1_pt[0] = 0;
		l1_pt[1] = 0; //(4 is the size of each entry)
		/*Flush entry*/
		COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, (uint32_t)l1_pt);
		dsb();
	}
}

/*Sets an entry in lvl 2 page table*/
void hypercall_set_pte(addr_t *va, uint32_t linux_pte, uint32_t phys_pte)
{
	uint32_t *phys_va = (uint32_t *)((uint32_t)va - 0x800);
	uint32_t PAGE_OFFSET = curr_vm->guest_info.page_offset;
	uint32_t PHYS_OFFSET = curr_vm->guest_info.phys_offset;
	uint32_t guest_size = curr_vm->guest_info.guest_size;

	/*Security Checks*/
	uint32_t pa = MMU_L2_SMALL_ADDR(phys_pte);

	/*Check virtual address*/
	if((uint32_t)phys_va < PAGE_OFFSET || (uint32_t)va > (uint32_t)(HAL_VIRT_START - sizeof(uint32_t)))
		hyper_panic("Page table entry reside outside of allowed address space !\n", 1);

	if(phys_pte != 0) { /*If 0, then its a remove mapping*/
		/*Check physical address*/
		if(!(pa >= (PHYS_OFFSET) && pa < (PHYS_OFFSET + guest_size) )){
			printf("Address: %x\n", pa);
			hyper_panic("Guest trying does not own pte physical address", 1);
		}
	}

	/***********************************************/
//	printf("\n\tHYP: Set pte va: %x phys_pte: %x \n", va, phys_pte );
	*phys_va = phys_pte;
	*va = linux_pte;
	COP_WRITE(COP_SYSTEM, COP_DCACHE_INVALIDATE_MVA, (uint32_t)phys_va );
	dsb();
}
