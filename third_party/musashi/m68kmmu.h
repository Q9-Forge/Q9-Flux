/*
    m68kmmu.h - PMMU implementation for 68851/68030/68040

    By R. Belmont

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.
*/

/*
	pmmu_translate_addr: perform 68851/68030-style PMMU address translation
*/
uint pmmu_translate_addr(uint addr_in)
{
	uint32 addr_out, tbl_entry = 0, tbl_entry2, tamode = 0, tbmode = 0, tcmode = 0;
	uint root_aptr, root_limit, tofs, is, abits, bbits, cbits;
	uint resolved, tptr, shift;

	resolved = 0;
	addr_out = addr_in;

	// if SRP is enabled and we're in supervisor mode, use it
	if ((m68ki_cpu.mmu_tc & 0x02000000) && (m68ki_get_sr() & 0x2000))
	{
		root_aptr = m68ki_cpu.mmu_srp_aptr;
		root_limit = m68ki_cpu.mmu_srp_limit;
	}
	else	// else use the CRP
	{
		root_aptr = m68ki_cpu.mmu_crp_aptr;
		root_limit = m68ki_cpu.mmu_crp_limit;
	}

	// get initial shift (# of top bits to ignore)
	is = (m68ki_cpu.mmu_tc>>16) & 0xf;
	abits = (m68ki_cpu.mmu_tc>>12)&0xf;
	bbits = (m68ki_cpu.mmu_tc>>8)&0xf;
	cbits = (m68ki_cpu.mmu_tc>>4)&0xf;

//	fprintf(stderr,"PMMU: tcr %08x limit %08x aptr %08x is %x abits %d bbits %d cbits %d\n", m68ki_cpu.mmu_tc, root_limit, root_aptr, is, abits, bbits, cbits);

	// get table A offset
	tofs = (addr_in<<is)>>(32-abits);

	// find out what format table A is
	switch (root_limit & 3)
	{
		case 0:	// invalid, should cause MMU exception
			// Q9: diagnostic detail added (see Q9_VENDOR.md)
			fatalerror("680x0 PMMU: Unhandled root mode (addr %08x tc %08x srp %08x/%08x crp %08x/%08x sr %04x pc %08x)\n",
				addr_in, m68ki_cpu.mmu_tc,
				m68ki_cpu.mmu_srp_limit, m68ki_cpu.mmu_srp_aptr,
				m68ki_cpu.mmu_crp_limit, m68ki_cpu.mmu_crp_aptr,
				m68ki_get_sr(), REG_PC);
			break;

		case 1:	// page descriptor -> direct mapping
			// Q9 addition (see Q9_VENDOR.md): DT=1 in the root pointer means
			// the root pointer itself is the page descriptor — the whole address
			// space maps flat, offset by the descriptor's address field (OS-9/68K
			// boots with such a 1:1 supervisor map before the SSM takes over).
			addr_out = addr_in + (root_aptr & 0xffffff00);
			resolved = 1;
			break;

		case 2:	// valid 4 byte descriptors
			tofs *= 4;
//			fprintf(stderr,"PMMU: reading table A entry at %08x\n", tofs + (root_aptr & 0xfffffffc));
			tbl_entry = m68k_read_memory_32( tofs + (root_aptr & 0xfffffffc));
			tamode = tbl_entry & 3;
//			fprintf(stderr,"PMMU: addr %08x entry %08x mode %x tofs %x\n", addr_in, tbl_entry, tamode, tofs);
			break;

		case 3: // valid 8 byte descriptors
			tofs *= 8;
//			fprintf(stderr,"PMMU: reading table A entries at %08x\n", tofs + (root_aptr & 0xfffffffc));
			tbl_entry2 = m68k_read_memory_32( tofs + (root_aptr & 0xfffffffc));
			tbl_entry = m68k_read_memory_32( tofs + (root_aptr & 0xfffffffc)+4);
			tamode = tbl_entry2 & 3;
//			fprintf(stderr,"PMMU: addr %08x entry %08x entry2 %08x mode %x tofs %x\n", addr_in, tbl_entry, tbl_entry2, tamode, tofs);
			break;
	}

	// Q9 addition: root was a page descriptor (direct mapping) — done, don't
	// walk the (nonexistent) tables (tamode==0 would hit the Table A fatalerror).
	if (resolved)
	{
		return addr_out;
	}

	// get table B offset and pointer
	tofs = (addr_in<<(is+abits))>>(32-bbits);
	tptr = tbl_entry & 0xfffffff0;

	// find out what format table B is, if any
	switch (tamode)
	{
		case 0: // invalid, should cause MMU exception
			fatalerror("680x0 PMMU: Unhandled Table A mode %d (addr_in %08x)\n", tamode, addr_in);
			break;

		case 2: // 4-byte table B descriptor
			tofs *= 4;
//			fprintf(stderr,"PMMU: reading table B entry at %08x\n", tofs + tptr);
			tbl_entry = m68k_read_memory_32( tofs + tptr);
			tbmode = tbl_entry & 3;
//			fprintf(stderr,"PMMU: addr %08x entry %08x mode %x tofs %x\n", addr_in, tbl_entry, tbmode, tofs);
			break;

		case 3: // 8-byte table B descriptor
			tofs *= 8;
//			fprintf(stderr,"PMMU: reading table B entries at %08x\n", tofs + tptr);
			tbl_entry2 = m68k_read_memory_32( tofs + tptr);
			tbl_entry = m68k_read_memory_32( tofs + tptr + 4);
			tbmode = tbl_entry2 & 3;
//			fprintf(stderr,"PMMU: addr %08x entry %08x entry2 %08x mode %x tofs %x\n", addr_in, tbl_entry, tbl_entry2, tbmode, tofs);
			break;

		case 1:	// early termination descriptor
			tbl_entry &= 0xffffff00;

			shift = is+abits;
			addr_out = ((addr_in<<shift)>>shift) + tbl_entry;
			resolved = 1;
			break;
	}

	// if table A wasn't early-out, continue to process table B
	if (!resolved)
	{
		// get table C offset and pointer
		tofs = (addr_in<<(is+abits+bbits))>>(32-cbits);
		tptr = tbl_entry & 0xfffffff0;

			switch (tbmode)
			{
				case 0:	// invalid, should cause MMU exception
				fprintf(stderr,
					"680x0 PMMU DEBUG: TC %08x SRP %08x/%08x CRP %08x/%08x "
					"root_limit %08x root_aptr %08x tblB %08x tofs %08x tptr %08x\n",
					m68ki_cpu.mmu_tc,
					m68ki_cpu.mmu_srp_limit, m68ki_cpu.mmu_srp_aptr,
					m68ki_cpu.mmu_crp_limit, m68ki_cpu.mmu_crp_aptr,
					root_limit, root_aptr, tbl_entry, tofs, tptr);
				fprintf(stderr, "680x0 PMMU DEBUG: D0-D7 %08x %08x %08x %08x %08x %08x %08x %08x\n",
					REG_D[0], REG_D[1], REG_D[2], REG_D[3], REG_D[4], REG_D[5], REG_D[6], REG_D[7]);
				fprintf(stderr, "680x0 PMMU DEBUG: A0-A7 %08x %08x %08x %08x %08x %08x %08x %08x\n",
					REG_A[0], REG_A[1], REG_A[2], REG_A[3], REG_A[4], REG_A[5], REG_A[6], REG_A[7]);
				fprintf(stderr, "680x0 PMMU DEBUG: USP %08x SR %04x\n", REG_USP, m68ki_get_sr());
				fprintf(stderr, "680x0 PMMU DEBUG: fault instruction PPC=%08x PC=%08x IR=%04x\n",
					REG_PPC, REG_PC, REG_IR);
				fprintf(stderr, "680x0 PMMU DEBUG: access VA=%08x FC=%x %s size=%u instr_mode=%u\n",
					q9_mmu_access_address, q9_mmu_access_fc,
					(q9_mmu_access_rw == MODE_READ) ? "read" : "write",
					q9_mmu_access_size, CPU_INSTR_MODE);
				{
					int dbgi;
					fprintf(stderr, "680x0 PMMU DEBUG: bytes at PC-8..PC+15:");
					for (dbgi = -8; dbgi < 16; dbgi++)
					{
						fprintf(stderr, "%s%02x", (dbgi == 0) ? " |" : " ", m68k_read_memory_8(REG_PC + dbgi));
					}
					fprintf(stderr, "\n");
				}
				/*
				 * A mode-0 descriptor is a translation fault.  The old Q9
				 * diagnostic deliberately stopped the whole emulator here, but
				 * on a real 68030 this is delivered to the guest as a fault.
				 * Use Musashi's existing exception path so OS-9 gets a chance to
				 * handle it (or report it) instead of taking down Q9.
				 */
				/* Keep the fault contained in the CPU exception path.  Passing
				 * the untranslated address through only creates cascading, bogus
				 * page-table faults and corrupts the diagnostic run. */
				m68ki_exception_bus_error();
				return addr_in;

			case 2: // 4-byte table C descriptor
				tofs *= 4;
//				fprintf(stderr,"PMMU: reading table C entry at %08x\n", tofs + tptr);
				tbl_entry = m68k_read_memory_32(tofs + tptr);
				tcmode = tbl_entry & 3;
//				fprintf(stderr,"PMMU: addr %08x entry %08x mode %x tofs %x\n", addr_in, tbl_entry, tbmode, tofs);
				break;

			case 3: // 8-byte table C descriptor
				tofs *= 8;
//				fprintf(stderr,"PMMU: reading table C entries at %08x\n", tofs + tptr);
				tbl_entry2 = m68k_read_memory_32(tofs + tptr);
				tbl_entry = m68k_read_memory_32(tofs + tptr + 4);
				tcmode = tbl_entry2 & 3;
//				fprintf(stderr,"PMMU: addr %08x entry %08x entry2 %08x mode %x tofs %x\n", addr_in, tbl_entry, tbl_entry2, tbmode, tofs);
				break;

			case 1: // termination descriptor
				tbl_entry &= 0xffffff00;

				shift = is+abits+bbits;
				addr_out = ((addr_in<<shift)>>shift) + tbl_entry;
				resolved = 1;
				break;
		}
	}

	if (!resolved)
	{
		// Q9 change (see Q9_VENDOR.md): table C entries with DT=2/3 point to a
		// FOURTH table level (TID field of TC) — the 68851/68030 walk has up to four
		// levels A/B/C/D and OS-9/68K's ssm851 actually uses them all. Was fatalerror
		// (with a mislabeled "Table B"/tbmode message) before.
		uint tdmode = 0;
		uint dbits = m68ki_cpu.mmu_tc & 0xf;

		switch (tcmode)
		{
			case 0:	// invalid, should cause MMU exception
				fprintf(stderr, "680x0 PMMU DEBUG: Table C fault PPC=%08x PC=%08x IR=%04x\n",
					REG_PPC, REG_PC, REG_IR);
				fprintf(stderr, "680x0 PMMU DEBUG: access VA=%08x FC=%x %s size=%u instr_mode=%u\n",
					q9_mmu_access_address, q9_mmu_access_fc,
					(q9_mmu_access_rw == MODE_READ) ? "read" : "write",
					q9_mmu_access_size, CPU_INSTR_MODE);
				fatalerror("680x0 PMMU: Unhandled Table C mode %d (entry %08x addr_in %08x PC %x)\n", tcmode, tbl_entry, addr_in, REG_PC);
				break;

			case 1: // termination descriptor
				tbl_entry &= 0xffffff00;

				shift = is+abits+bbits+cbits;
				addr_out = ((addr_in<<shift)>>shift) + tbl_entry;
				resolved = 1;
				break;

			case 2: // 4-byte table D descriptor
			case 3: // 8-byte table D descriptor
				// get table D offset and pointer
				tofs = (addr_in<<(is+abits+bbits+cbits))>>(32-dbits);
				tptr = tbl_entry & 0xfffffff0;

				if (tcmode == 2)
				{
					tofs *= 4;
					tbl_entry = m68k_read_memory_32(tofs + tptr);
					tdmode = tbl_entry & 3;
				}
				else
				{
					tofs *= 8;
					tbl_entry2 = m68k_read_memory_32(tofs + tptr);
					tbl_entry = m68k_read_memory_32(tofs + tptr + 4);
					tdmode = tbl_entry2 & 3;
				}

				switch (tdmode)
				{
					case 1: // termination descriptor (page descriptor — last level)
						tbl_entry &= 0xffffff00;

						shift = is+abits+bbits+cbits+dbits;
						addr_out = ((addr_in<<shift)>>shift) + tbl_entry;
						resolved = 1;
						break;

					default:
						fatalerror("680x0 PMMU: Unhandled Table D mode %d (entry %08x addr_in %08x PC %x)\n", tdmode, tbl_entry, addr_in, REG_PC);
						break;
				}
				break;
		}
	}


//	fprintf(stderr,"PMMU: [%08x] => [%08x]\n", addr_in, addr_out);

	return addr_out;
}

/*

	m68881_mmu_ops: COP 0 MMU opcode handling

*/

void m68881_mmu_ops(void)
{
	uint16 modes;
	uint32 ea = m68ki_cpu.ir & 0x3f;
	uint64 temp64;

	// catch the 2 "weird" encodings up front (PBcc)
	if ((m68ki_cpu.ir & 0xffc0) == 0xf0c0)
	{
		fprintf(stderr,"680x0: unhandled PBcc\r\n");
		return;
	}
	else if ((m68ki_cpu.ir & 0xffc0) == 0xf080)
	{
		fprintf(stderr,"680x0: unhandled PBcc\r\n");
		return;
	}
	else	// the rest are 1111000xxxXXXXXX where xxx is the instruction family
	{
		switch ((m68ki_cpu.ir>>9) & 0x7)
		{
			case 0:
				modes = OPER_I_16();

				if ((modes & 0xfde0) == 0x2000)	// PLOAD
				{
					fprintf(stderr,"680x0: unhandled PLOAD\r\n");
					return;
				}
				else if ((modes & 0xe200) == 0x2000)	// PFLUSH
				{
					// Q9 (see Q9_VENDOR.md): warn once, then stay silent — OS-9
					// flushes on every context switch and the repeated message wrecks
					// the console. Ignoring PFLUSH is harmless here: this emulation
					// has no TLB, every access re-walks the tables.
					static int warned_pflush = 0;
					if (!warned_pflush)
					{
						warned_pflush = 1;
						fprintf(stderr,"680x0: unhandled PFLUSH PC=%x (ok/ignoriert - kein TLB; weitere Meldungen unterdrueckt)\r\n", REG_PC);
					}
					return;
				}
				else if (modes == 0xa000)	// PFLUSHR
				{
					fprintf(stderr,"680x0: unhandled PFLUSHR\r\n");
					return;
				}
				else if (modes == 0x2800)	// PVALID (FORMAT 1)
				{
					fprintf(stderr,"680x0: unhandled PVALID1\r\n");
					return;
				}
				else if ((modes & 0xfff8) == 0x2c00)	// PVALID (FORMAT 2)
				{
					fprintf(stderr,"680x0: unhandled PVALID2\r\n");
					return;
				}
				else if ((modes & 0xe000) == 0x8000)	// PTEST
				{
					// Q9 (see Q9_VENDOR.md): warn once, then stay silent (s. PFLUSH).
					// TODO: PTEST sollte eigentlich das MMU-SR setzen — bisher scheint
					// OS-9 das Ergebnis nicht auszuwerten (Boot + Shell laufen), bei
					// Bedarf nachruesten.
					static int warned_ptest = 0;
					if (!warned_ptest)
					{
						warned_ptest = 1;
						fprintf(stderr,"680x0: unhandled PTEST (weitere Meldungen unterdrueckt)\r\n");
					}
					return;
				}
				else
				{
					switch ((modes>>13) & 0x7)
					{
						case 0:	// MC68030/040 form with FD bit
						case 2:	// MC68881 form, FD never set
							if (modes & 0x200)
							{
							 	switch ((modes>>10) & 7)
								{
									case 0:	// translation control register
										WRITE_EA_32(ea, m68ki_cpu.mmu_tc);
										break;

									case 2: // supervisor root pointer
										WRITE_EA_64(ea, (uint64)m68ki_cpu.mmu_srp_limit<<32 | (uint64)m68ki_cpu.mmu_srp_aptr);
										break;

									case 3: // CPU root pointer
										WRITE_EA_64(ea, (uint64)m68ki_cpu.mmu_crp_limit<<32 | (uint64)m68ki_cpu.mmu_crp_aptr);
										break;

									default:
										fprintf(stderr,"680x0: PMOVE from unknown MMU register %x, PC %x\n", (modes>>10) & 7, REG_PC);
										break;
								}
							}
							else
							{
							 	switch ((modes>>10) & 7)
								{
									case 0:	// translation control register
										m68ki_cpu.mmu_tc = READ_EA_32(ea);

										if (m68ki_cpu.mmu_tc & 0x80000000)
										{
											m68ki_cpu.pmmu_enabled = 1;
										}
										else
										{
											m68ki_cpu.pmmu_enabled = 0;
										}
										break;

									case 2:	// supervisor root pointer
										temp64 = READ_EA_64(ea);
										m68ki_cpu.mmu_srp_limit = (temp64>>32) & 0xffffffff;
										m68ki_cpu.mmu_srp_aptr = temp64 & 0xffffffff;
										break;

									case 3:	// CPU root pointer
										temp64 = READ_EA_64(ea);
										m68ki_cpu.mmu_crp_limit = (temp64>>32) & 0xffffffff;
										m68ki_cpu.mmu_crp_aptr = temp64 & 0xffffffff;
										break;

									default:
										fprintf(stderr,"680x0: PMOVE to unknown MMU register %x, PC %x\n", (modes>>10) & 7, REG_PC);
										break;
								}
							}
							break;

						case 3:	// MC68030 to/from status reg
							if (modes & 0x200)
							{
								WRITE_EA_32(ea, m68ki_cpu.mmu_sr);
							}
							else
							{
								m68ki_cpu.mmu_sr = READ_EA_32(ea);
							}
							break;

						default:
							fprintf(stderr,"680x0: unknown PMOVE mode %x (modes %04x) (PC %x)\n", (modes>>13) & 0x7, modes, REG_PC);
							break;
					}
				}
				break;

			default:
				fprintf(stderr,"680x0: unknown PMMU instruction group %d\n", (m68ki_cpu.ir>>9) & 0x7);
				break;
		}
	}
}
