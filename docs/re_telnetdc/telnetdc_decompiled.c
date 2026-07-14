// ===== real_entry @ 00000052 =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 * real_entry(void)

{
  undefined4 in_D0;
  undefined1 *puVar1;
  int iVar2;
  undefined2 unaff_D3w;
  undefined4 unaff_D6;
  int unaff_A3;
  int unaff_A4;
  int unaff_A6;
  undefined1 auStack_100 [236];
  undefined4 uStack_14;
  undefined1 *puStack_10;
  undefined1 *puStack_c;
  
  *(undefined4 *)(unaff_A6 + -0x7fec) = unaff_D6;
  *(int *)(unaff_A6 + -0x7fce) = unaff_A6;
  *(undefined4 *)(unaff_A6 + -0x7fe8) = unaff_D6;
  *(undefined2 *)(unaff_A6 + -0x7fe4) = unaff_D3w;
  *(int *)(unaff_A6 + -0x7fde) = unaff_A3;
  *(undefined4 *)(unaff_A6 + -0x7fd6) = in_D0;
  *(int *)(unaff_A6 + -0x7fda) = *(int *)(unaff_A3 + 0xc) + unaff_A3;
  if (((*(byte *)(unaff_A3 + 0x14) & 0x20) != 0) &&
     (*(int *)(unaff_A6 + -0x7fe2) = unaff_A4, unaff_A4 == 0)) {
    *(undefined4 *)(unaff_A6 + -0x7fe2) = _DAT_00000000;
  }
  FUN_000000d8();
  puStack_c = (undefined1 *)0x9e;
  FUN_00002bf6();
  puStack_10 = &stack0xfffffff8;
  puStack_c = &stack0xfffffffc;
  uStack_14 = 0xb8;
  (*(code *)&LAB_00002b32_2)();
  FUN_000006da();
  FUN_00002f50();
  *(int *)(unaff_A6 + -0x7ff8) = unaff_A6 + -0x6766;
  *(undefined1 **)(unaff_A6 + -0x8000) = &stack0xfffffffc;
  *(undefined1 **)(unaff_A6 + -0x7ff4) = &stack0xfffffffc;
  puVar1 = auStack_100;
  *(undefined4 *)(unaff_A6 + -0x6be4) = 0;
  if (*(undefined1 **)(unaff_A6 + -0x7ff4) <= auStack_100) {
    return puVar1;
  }
  if (*(undefined1 **)(unaff_A6 + -0x7ff8) <= auStack_100) {
    *(undefined1 **)(unaff_A6 + -0x7ff4) = auStack_100;
    return puVar1;
  }
  FUN_00002efa();
  iVar2 = FUN_00002ff2();
  __m68k_trap(0);
  return (undefined1 *)(iVar2 - *(int *)(unaff_A6 + -0x7ff4));
}



// ===== FUN_000000d8 @ 000000d8 =====

undefined1 * FUN_000000d8(void)

{
  undefined1 *puVar1;
  int iVar2;
  int unaff_A6;
  undefined1 auStack_fc [248];
  undefined4 uStack_4;
  
  *(int *)(unaff_A6 + -0x7ff8) = unaff_A6 + -0x6766;
  *(BADSPACEBASE **)(unaff_A6 + -0x8000) = register0x0000003c;
  *(BADSPACEBASE **)(unaff_A6 + -0x7ff4) = register0x0000003c;
  puVar1 = auStack_fc;
  *(undefined4 *)(unaff_A6 + -0x6be4) = 0;
  if (*(undefined1 **)(unaff_A6 + -0x7ff4) <= auStack_fc) {
    return puVar1;
  }
  if (*(undefined1 **)(unaff_A6 + -0x7ff8) <= auStack_fc) {
    *(undefined1 **)(unaff_A6 + -0x7ff4) = auStack_fc;
    return puVar1;
  }
  uStack_4 = 0x11a;
  FUN_00002efa();
  uStack_4 = 0x126;
  iVar2 = FUN_00002ff2();
  __m68k_trap(0);
  return (undefined1 *)(iVar2 - *(int *)(unaff_A6 + -0x7ff4));
}



// ===== FUN_0000013e @ 0000013e =====

/* WARNING: Instruction at (ram,0x0000017a) overlaps instruction at (ram,0x00000178)
    */

undefined8 FUN_0000013e(short param_1)

{
  ushort uVar1;
  undefined4 in_D0;
  undefined4 in_D1;
  int in_A1;
  bool bVar2;
  undefined4 local_18;
  undefined4 uStack_14;
  
  uVar1 = (short)(param_1 + -0x80) >> 2;
  if (uVar1 == 0xf) {
    __m68k_trap(0);
    *(byte *)(in_A1 + -1) = *(byte *)(in_A1 + -1) | 0x1c;
  }
  bVar2 = uVar1 < 0xf;
  local_18 = in_D0;
  uStack_14 = in_D1;
  FUN_00002f4a();
  if (bVar2) {
    local_18 = FUN_00000184();
    uStack_14 = 0xe3;
    __m68k_trap(0);
    register0x0000003c = (BADSPACEBASE *)&local_18;
  }
  *(int *)((int)register0x0000003c + 8) = *(int *)((int)register0x0000003c + 8) + -4;
  return CONCAT44(local_18,uStack_14);
}



// ===== FUN_00000184 @ 00000184 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00000184(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_000006da @ 000006da =====

void FUN_000006da(void)

{
  undefined4 uVar1;
  int iVar2;
  int extraout_A0;
  int extraout_A0_00;
  int extraout_A0_01;
  undefined4 extraout_A1;
  int unaff_A6;
  
  FUN_00002f56();
  *(undefined4 *)(unaff_A6 + -0x679e) = 0;
  *(undefined4 *)(unaff_A6 + -0x677a) = 0;
  FUN_00000878();
  iVar2 = extraout_A0;
  if (*(int *)(unaff_A6 + -0x677a) != 0) {
    uVar1 = FUN_00002fda(*(undefined4 *)(unaff_A6 + -0x7fd6));
    func_0x00002b2e(uVar1);
    iVar2 = extraout_A0_00;
  }
  if (*(int *)(iVar2 + 4) != 0) {
    FUN_00001ba6();
    FUN_00001ba6();
    FUN_00001bc2();
    FUN_00001bc2();
    FUN_00001ba6();
    if (*(int *)(extraout_A0_01 + 8) == 0) {
      FUN_000008e8(3);
      FUN_00002f50();
    }
  }
  *(undefined4 *)(unaff_A6 + -0x67a6) = 0;
  FUN_00002766();
  FUN_000009e4(extraout_A1);
  FUN_00000c9c();
  if (*(int *)(unaff_A6 + -0x679e) != 0) {
    FUN_00000d08();
  }
  FUN_00001ba6();
  FUN_00001ba6();
  FUN_00001ba6();
  FUN_00001bc2();
  FUN_00001bc2();
  if (*(int *)(unaff_A6 + -0x677a) != 0) {
    func_0x00002b2e(0x50);
  }
  iVar2 = FUN_000008e8(3);
  if (iVar2 == -1) {
    if (*(int *)(unaff_A6 + -0x677a) != 0) {
      FUN_00002fda(*(undefined4 *)(unaff_A6 + -0x67aa));
      func_0x00002b2e(0x50);
    }
    FUN_00000dd6();
  }
  FUN_00001ba6();
  FUN_00001ba6();
  *(undefined4 *)(unaff_A6 + -0x67d6) = 0;
  FUN_00000e5c();
  if (*(int *)(unaff_A6 + -0x677a) != 0) {
    FUN_00002fda(*(undefined4 *)(unaff_A6 + -0x7ff0));
    func_0x00002b2e(0x50);
  }
  if (*(int *)(unaff_A6 + -0x679e) != 0) {
    FUN_00000fac();
  }
  FUN_00000dd6();
  return;
}



// ===== FUN_00000878 @ 00000878 =====

int FUN_00000878(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *extraout_A0;
  undefined4 *extraout_A0_00;
  undefined4 *puVar4;
  undefined4 *extraout_A1;
  undefined4 *extraout_A1_00;
  char *pcVar5;
  undefined8 uVar6;
  
  uVar6 = FUN_00002f56();
  puVar3 = (undefined4 *)((int)uVar6 + 4);
  puVar4 = (undefined4 *)((int)uVar6 + 4);
  pcVar5 = (char *)*puVar4;
  iVar1 = 1;
  iVar2 = 1;
  while ((pcVar5 != (char *)0x0 && (iVar2 < (int)((ulonglong)uVar6 >> 0x20)))) {
    if (*pcVar5 == '-') {
      if (pcVar5[1] == '\0') {
        FUN_00001400();
        FUN_00002b28(0,0,0);
        FUN_00000dd6();
        puVar3 = extraout_A0;
        puVar4 = extraout_A1;
      }
      else {
        FUN_00001428();
        puVar3 = extraout_A0_00;
        puVar4 = extraout_A1_00;
      }
    }
    else {
      iVar1 = iVar1 + 1;
      *puVar3 = *puVar4;
      puVar3 = puVar3 + 1;
    }
    puVar4 = puVar4 + 1;
    pcVar5 = (char *)*puVar4;
    iVar2 = iVar2 + 1;
  }
  *puVar3 = 0;
  return iVar1;
}



// ===== FUN_000008e8 @ 000008e8 =====

void FUN_000008e8(undefined4 param_1)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int unaff_A6;
  undefined8 uVar4;
  
  uVar4 = FUN_00002f56();
  uVar1 = (undefined4)((ulonglong)uVar4 >> 0x20);
  if ((int)uVar4 == 0) {
    *(undefined4 *)(unaff_A6 + -0x6c2c) = uVar1;
    *(undefined4 *)(unaff_A6 + -0x6c28) = 0;
  }
  else {
    *(undefined4 *)(unaff_A6 + -0x6c2c) = uVar1;
    iVar2 = 1;
    if (*(int *)(unaff_A6 + -0x677a) != 0) {
      iVar2 = 2;
      *(undefined4 *)(unaff_A6 + -0x6c28) = 0x20f;
    }
    iVar3 = iVar2;
    if (*(int *)(unaff_A6 + -0x679e) != 0) {
      iVar3 = iVar2 + 1;
      *(undefined4 *)(unaff_A6 + -0x6c2c + iVar2 * 4) = 0x20c;
    }
    *(undefined1 *)(unaff_A6 + -0x6c4c) = 0x2d;
    *(undefined1 *)(unaff_A6 + -0x6c4b) = 0x74;
    *(undefined1 *)(unaff_A6 + -0x6c4a) = 0x3d;
    FUN_00002fda(*(undefined4 *)(unaff_A6 + -0x6792));
    *(int *)(unaff_A6 + -0x6c2c + iVar3 * 4) = unaff_A6 + -0x6c4c;
    iVar2 = iVar3 + 1;
    if (*(int *)(unaff_A6 + -0x67aa) != 0) {
      *(undefined1 *)(unaff_A6 + -0x6e40) = 0x2d;
      *(undefined1 *)(unaff_A6 + -0x6e3f) = 0x66;
      *(undefined1 *)(unaff_A6 + -0x6e3e) = 0x3d;
      FUN_00002fe6();
      iVar2 = iVar3 + 2;
      *(int *)(unaff_A6 + -0x6c2c + (iVar3 + 1) * 4) = unaff_A6 + -0x6e40;
    }
    *(undefined4 *)(unaff_A6 + -0x6c2c + iVar2 * 4) = 0;
  }
  FUN_00001c32(unaff_A6 + -0x6c2c,*(undefined4 *)(unaff_A6 + -0x6be8),0,0,param_1);
  return;
}



// ===== FUN_000009e4 @ 000009e4 =====

void FUN_000009e4(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  undefined1 local_8 [4];
  
  FUN_00002f56();
  *(undefined4 *)(unaff_A6 + -0x67be) = 0;
  *(undefined4 *)(unaff_A6 + -0x67d6) = 1;
  *(undefined4 *)(unaff_A6 + -0x67ba) = 0;
  FUN_00001bc2();
  iVar1 = FUN_00001f6e();
  if (iVar1 < 0) {
    FUN_00002b28(0,0,0);
    FUN_00002908();
    FUN_00000dd6();
  }
  iVar1 = FUN_000029d8(local_8);
  if (iVar1 < 0) {
    FUN_00002b28(0,0,0);
    FUN_00002908();
    FUN_00001ba6();
    FUN_00000dd6();
  }
  FUN_00001ba6();
  FUN_00001bc2();
  FUN_00001ba6();
  FUN_00001ba6();
  FUN_00001bc2();
  FUN_00001ba6();
  FUN_00001ba6();
  iVar1 = FUN_0000288a();
  if (iVar1 != -1) {
    iVar1 = FUN_0000288a();
    if (iVar1 != -1) goto LAB_00000b04;
  }
  FUN_00002b28(0,0,0);
  FUN_00002908();
  FUN_00000dd6();
LAB_00000b04:
  if ((*(char *)(unaff_A6 + -0x69e0) != '\0') || (*(char *)(unaff_A6 + -0x6960) != '\b')) {
    FUN_00002b28(0,0,0);
    FUN_00002908();
    FUN_00000dd6();
  }
  FUN_00002fda(*(undefined4 *)(unaff_A6 + -0x7fd6));
  iVar1 = FUN_00002924();
  *(int *)(unaff_A6 + -0x6786) = iVar1;
  if (iVar1 != -1) {
    FUN_00002954();
    FUN_00002954();
    FUN_00002954();
    FUN_00002970();
  }
  iVar1 = FUN_00002992(0xffffffff,unaff_A6 + -0x6c6c);
  *(int *)(unaff_A6 + -0x6786) = iVar1;
  if (iVar1 == -1) {
    FUN_00002b28(0,0,0);
    FUN_00002908();
    FUN_00000dd6();
  }
  uVar2 = FUN_00001bc2();
  *(undefined4 *)(unaff_A6 + -0x67a2) = uVar2;
  uVar2 = FUN_00001bc2();
  *(undefined4 *)(unaff_A6 + -0x677e) = uVar2;
  uVar2 = FUN_00001bc2();
  *(undefined4 *)(unaff_A6 + -0x678a) = uVar2;
  iVar1 = FUN_00002010();
  if (iVar1 == -1) {
    FUN_00002b28(0,0,0);
    FUN_00002908();
    FUN_000014f8();
    FUN_00000dd6();
  }
  iVar1 = FUN_00001520();
  if (iVar1 == -1) {
    FUN_00002b28(0,0,0);
    FUN_00002908();
    FUN_000014f8();
    FUN_00000dd6();
  }
  FUN_0000153e();
  FUN_0000153e();
  FUN_00001a66();
  FUN_000010ae();
  FUN_0000158a();
  return;
}



// ===== FUN_00000c9c @ 00000c9c =====

undefined8 FUN_00000c9c(void)

{
  undefined4 in_D0;
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  int unaff_A6;
  
  FUN_00002f56();
  *(undefined4 *)(unaff_A6 + -0x67d2) = 0x10;
  iVar1 = FUN_00002faa();
  *(int *)(unaff_A6 + -0x679a) = iVar1;
  if (iVar1 != 0) {
    iVar1 = FUN_00002076(unaff_A6 + -0x67d2);
    if (iVar1 == -1) {
      func_0x00002fbc();
      *(undefined4 *)(unaff_A6 + -0x679a) = 0;
    }
    else if (*(int *)(unaff_A6 + -0x679e) != 0) {
      uVar2 = FUN_00002204(*(undefined1 *)(*(int *)(unaff_A6 + -0x679a) + 1));
      *(undefined4 *)(unaff_A6 + -0x67ba) = uVar2;
    }
  }
  return CONCAT44(in_D0,in_D1);
}



// ===== FUN_00000d08 @ 00000d08 =====

void FUN_00000d08(void)

{
  undefined4 uVar1;
  int iVar2;
  int unaff_A6;
  
  FUN_00002f56();
  uVar1 = FUN_00002fda();
  func_0x00002b2e(uVar1);
  if (*(undefined4 **)(unaff_A6 + -0x67ba) != (undefined4 *)0x0) {
    uVar1 = FUN_00002fda(**(undefined4 **)(unaff_A6 + -0x67ba));
    func_0x00002b2e(uVar1);
  }
  if (*(int *)(unaff_A6 + -0x679a) != 0) {
    uVar1 = FUN_00002fda(*(undefined2 *)(*(int *)(unaff_A6 + -0x679a) + 2));
    func_0x00002b2e(uVar1);
  }
  iVar2 = FUN_00002fc2();
  if (iVar2 != -1) {
    func_0x00002fc8();
    uVar1 = FUN_00002f9e();
    uVar1 = FUN_00002fda(uVar1);
    func_0x00002b2e(uVar1);
    FUN_00001582();
    return;
  }
  func_0x00002b2e(1);
  FUN_00001582();
  return;
}



// ===== FUN_00000dd6 @ 00000dd6 =====

void FUN_00000dd6(void)

{
  int iVar1;
  int unaff_A6;
  
  FUN_00002f56();
  if (*(int *)(unaff_A6 + -0x67a2) != -1) {
    FUN_00001520();
    FUN_00001ba6();
  }
  if (*(int *)(unaff_A6 + -0x678a) != -1) {
    FUN_00002010();
    FUN_00001ba6();
  }
  FUN_000014f8();
  if (*(int *)(unaff_A6 + -0x67be) != 0) {
    func_0x00002fbc();
    *(undefined4 *)(unaff_A6 + -0x6be8) = *(undefined4 *)(unaff_A6 + -0x67be);
  }
  if (*(int *)(unaff_A6 + -0x679a) != 0) {
    func_0x00002fbc();
    *(undefined4 *)(unaff_A6 + -0x679a) = 0;
    iVar1 = FUN_000021a2();
    if (-1 < iVar1) {
      FUN_00002a50();
    }
  }
  FUN_00002f50();
  return;
}



// ===== FUN_00000e5c @ 00000e5c =====

undefined8 FUN_00000e5c(void)

{
  undefined4 in_D0;
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  undefined4 in_D1;
  int unaff_A6;
  
  FUN_00002f56();
  if (*(int *)(unaff_A6 + -0x6792) != 0) {
    if (*(int *)(unaff_A6 + -0x677a) != 0) {
      uVar1 = FUN_00002fda();
      func_0x00002b2e(uVar1);
    }
    uVar2 = FUN_00003002();
    iVar3 = FUN_000030b6(uVar2 >> 1 | 0x80000000);
    *(int *)(unaff_A6 + -0x7ff0) = iVar3;
    if (iVar3 != 0) {
      if (*(int *)(unaff_A6 + -0x677a) != 0) {
        FUN_00002fb0();
      }
      goto LAB_00000fa6;
    }
  }
  *(undefined4 *)(unaff_A6 + -0x6782) = 0;
  while( true ) {
    *(undefined4 *)(unaff_A6 + -0x6796) = 0;
    *(undefined4 *)(unaff_A6 + -0x678e) = 0;
    if (*(int *)(unaff_A6 + -0x67a6) != 0) break;
    iVar3 = FUN_000010ae();
    if (iVar3 == 0) goto LAB_00000f92;
    if (iVar3 == 1) {
      *(undefined4 *)(unaff_A6 + -0x6782) = 0;
    }
    iVar3 = FUN_0000118c();
    if (iVar3 == 0) goto LAB_00000f92;
    if (iVar3 == 1) {
      *(undefined4 *)(unaff_A6 + -0x6782) = 0;
    }
    if (*(int *)(unaff_A6 + -0x678e) == 0) {
      FUN_00002812(0x7fff);
      if (1 < *(uint *)(unaff_A6 + -0x6782)) {
        FUN_0000126c();
        goto LAB_00000f92;
      }
      if (*(int *)(unaff_A6 + -0x6796) != 0) {
        FUN_0000126c();
      }
    }
  }
  if (*(int *)(unaff_A6 + -0x677a) != 0) {
    uVar1 = FUN_00002fda(*(int *)(unaff_A6 + -0x67a6));
    func_0x00002b2e(uVar1);
  }
LAB_00000f92:
  if (*(int *)(unaff_A6 + -0x67c6) != 0) {
    FUN_00003072();
  }
  if (*(int *)(unaff_A6 + -0x67ca) != 0) {
    FUN_00003072();
  }
LAB_00000fa6:
  return CONCAT44(in_D0,in_D1);
}



// ===== FUN_00000fac @ 00000fac =====

void FUN_00000fac(void)

{
  undefined4 uVar1;
  int iVar2;
  int unaff_A6;
  
  FUN_00002f56();
  uVar1 = FUN_00002fda();
  func_0x00002b2e(uVar1);
  if (*(undefined4 **)(unaff_A6 + -0x67ba) != (undefined4 *)0x0) {
    uVar1 = FUN_00002fda(**(undefined4 **)(unaff_A6 + -0x67ba));
    func_0x00002b2e(uVar1);
  }
  if (*(int *)(unaff_A6 + -0x679a) != 0) {
    uVar1 = FUN_00002fda(*(undefined2 *)(*(int *)(unaff_A6 + -0x679a) + 2));
    func_0x00002b2e(uVar1);
  }
  uVar1 = FUN_00002fda();
  func_0x00002b2e(uVar1);
  iVar2 = FUN_00002fc2();
  if (iVar2 != -1) {
    func_0x00002fc8();
    uVar1 = FUN_00002f9e();
    uVar1 = FUN_00002fda(uVar1);
    func_0x00002b2e(uVar1);
    FUN_00001582();
    return;
  }
  func_0x00002b2e(1);
  FUN_00001582();
  return;
}



// ===== FUN_000010ae @ 000010ae =====

undefined8 FUN_000010ae(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  if (*(int *)(unaff_A6 + -0x67b6) == 0) {
    iVar1 = FUN_0000283c();
    *(int *)(unaff_A6 + -0x6bfc) = iVar1;
    if (0 < iVar1) {
      if (0x3ff < iVar1) {
        iVar1 = 0x400;
      }
      iVar1 = FUN_00001eea(iVar1);
      *(int *)(unaff_A6 + -0x6bfc) = iVar1;
      if (iVar1 < 1) {
        uVar2 = 0;
        goto LAB_00001264;
      }
      FUN_0000163e(unaff_A6 + -0x7640);
      if (*(int *)(unaff_A6 + -0x6bfc) != 0) {
        *(undefined4 *)(unaff_A6 + -0x67b6) = 1;
        *(int *)(unaff_A6 + -0x6c10) = unaff_A6 + -0x7640;
      }
      goto LAB_00001132;
    }
    if (*(int *)(unaff_A6 + -0x7ff0) != 0xf6) {
LAB_0000117e:
      uVar2 = 0;
      goto LAB_00001264;
    }
LAB_00001178:
    uVar2 = 2;
  }
  else {
LAB_00001132:
    if (*(int *)(unaff_A6 + -0x67b6) != 0) {
      *(undefined4 *)(unaff_A6 + -0x678e) = 1;
      local_8 = *(undefined4 *)(unaff_A6 + -0x6bfc);
      iVar1 = FUN_000034d2(&local_8);
      if (iVar1 != 0) {
        *(int *)(unaff_A6 + -0x7ff0) = iVar1;
        if (iVar1 != 0x701) goto LAB_0000117e;
        FUN_00003050();
        goto LAB_00001178;
      }
      *(undefined4 *)(unaff_A6 + -0x67b6) = 0;
    }
    uVar2 = 1;
  }
LAB_00001264:
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_0000118c @ 0000118c =====

undefined8 FUN_0000118c(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  int unaff_A6;
  
  FUN_00002f56();
  if (*(int *)(unaff_A6 + -0x67b2) == 0) {
    *(undefined4 *)(unaff_A6 + -0x67ae) = 0;
    iVar1 = FUN_0000283c();
    *(int *)(unaff_A6 + -0x6bf8) = iVar1;
    if (0 < iVar1) {
      if (0x3ff < iVar1) {
        iVar1 = 0x400;
      }
      iVar1 = FUN_00001eea(iVar1);
      *(int *)(unaff_A6 + -0x6bf8) = iVar1;
      if (iVar1 < 1) {
        uVar2 = 0;
        goto LAB_00001264;
      }
      *(undefined4 *)(unaff_A6 + -0x67b2) = 1;
      goto LAB_000011f0;
    }
    if (*(int *)(unaff_A6 + -0x7ff0) != 0xf6) {
LAB_00001246:
      uVar2 = 0;
      goto LAB_00001264;
    }
LAB_00001242:
    uVar2 = 2;
  }
  else {
LAB_000011f0:
    if (*(int *)(unaff_A6 + -0x67b2) != 0) {
      *(undefined4 *)(unaff_A6 + -0x678e) = 1;
      iVar1 = FUN_00001f42(*(undefined4 *)(unaff_A6 + -0x6bf8));
      if (iVar1 == -1) {
        if ((*(int *)(unaff_A6 + -0x7ff0) != 0x701) && (*(int *)(unaff_A6 + -0x7ff0) != 0x714))
        goto LAB_00001246;
        FUN_00003050();
        goto LAB_00001242;
      }
      if (iVar1 == *(int *)(unaff_A6 + -0x6bf8)) {
        *(undefined4 *)(unaff_A6 + -0x67b2) = 0;
        *(undefined4 *)(unaff_A6 + -0x67ae) = 0;
      }
      else {
        *(int *)(unaff_A6 + -0x6bf8) = *(int *)(unaff_A6 + -0x6bf8) - iVar1;
        *(int *)(unaff_A6 + -0x67ae) = iVar1 + *(int *)(unaff_A6 + -0x67ae);
      }
    }
    uVar2 = 1;
  }
LAB_00001264:
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_0000126c @ 0000126c =====

void FUN_0000126c(void)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  undefined5 uVar4;
  undefined1 local_a [6];
  
  uVar4 = FUN_00002f56();
  iVar1 = (int)uVar4;
  iVar2 = 0;
  puVar3 = local_a;
  if ((((iVar1 != 0xf1) && (iVar1 != 0xfc)) && (iVar1 != 0xfb)) && (iVar1 != 0xfe)) {
    if (iVar1 == 0xff) {
      puVar3 = local_a + 1;
      local_a[0] = 0xff;
      iVar2 = 1;
      goto LAB_000012c4;
    }
    if (iVar1 != 0xfd) goto LAB_000012c4;
  }
  local_a[0] = 0xff;
  local_a[1] = (undefined1)uVar4;
  puVar3 = local_a + 2;
  iVar2 = 2;
LAB_000012c4:
  *puVar3 = (char)((uint5)uVar4 >> 0x20);
  iVar1 = FUN_00001f42(iVar2 + 1);
  if (iVar2 + 1 != iVar1) {
    func_0x00002b2e(0x13);
  }
  return;
}



// ===== FUN_000012fa @ 000012fa =====

void FUN_000012fa(int param_1,int param_2)

{
  undefined4 uVar1;
  int extraout_A1;
  int unaff_A6;
  
  uVar1 = FUN_00002f56();
  if (*(int *)(unaff_A6 + -0x677a) != 0) {
    uVar1 = FUN_00002fda(uVar1);
    func_0x00002b2e(uVar1);
    if (param_1 < 0x15) {
      param_1 = *(int *)(unaff_A6 + -0x68e0 + param_1 * 4);
    }
    uVar1 = FUN_00002fda(extraout_A1 + 5,param_1);
    func_0x00002b2e(uVar1);
    if (param_2 == 0) {
      uVar1 = 0x2db;
    }
    else {
      uVar1 = 0x2ca;
    }
    uVar1 = FUN_00002fda(uVar1,10);
    func_0x00002b2e(uVar1);
  }
  return;
}



// ===== FUN_000013ce @ 000013ce =====

bool FUN_000013ce(void)

{
  undefined1 extraout_D1b;
  int unaff_A6;
  bool bVar1;
  
  FUN_00002f56();
  bVar1 = *(int *)(unaff_A6 + -0x67d6) == 0;
  if (!bVar1) {
    *(undefined1 *)(unaff_A6 + -0x69dc) = extraout_D1b;
    FUN_000028c2();
  }
  return bVar1;
}



// ===== FUN_00001400 @ 00001400 =====

undefined8 FUN_00001400(void)

{
  undefined4 in_D0;
  undefined4 in_D1;
  uint extraout_A0;
  uint uVar1;
  uint extraout_A1;
  uint uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  uVar1 = unaff_A6 - 0x6830;
  uVar2 = unaff_A6 - 0x680c;
  while (uVar1 < uVar2) {
    FUN_00002fb6();
    uVar1 = extraout_A0;
    uVar2 = extraout_A1;
  }
  return CONCAT44(in_D0,in_D1);
}



// ===== FUN_00001428 @ 00001428 =====

void FUN_00001428(void)

{
  byte *pbVar1;
  byte *pbVar2;
  byte bVar4;
  undefined4 uVar3;
  byte *extraout_A0;
  byte *extraout_A0_00;
  byte *extraout_A0_01;
  int unaff_A6;
  
  pbVar2 = (byte *)FUN_00002f56();
  do {
    bVar4 = *pbVar2 | 0x20;
    if (bVar4 == 100) {
      *(int *)(unaff_A6 + -0x677a) = *(int *)(unaff_A6 + -0x677a) + 1;
    }
    else if (bVar4 == 0x66) {
      pbVar1 = pbVar2 + 1;
      if (pbVar2[1] == 0x3d) {
        pbVar1 = pbVar2 + 2;
      }
      pbVar2 = pbVar1;
      *(byte **)(unaff_A6 + -0x67aa) = pbVar2;
      for (; pbVar2[1] != 0; pbVar2 = pbVar2 + 1) {
      }
    }
    else if (bVar4 == 0x74) {
      uVar3 = FUN_00002fa4();
      *(undefined4 *)(unaff_A6 + -0x6792) = uVar3;
      pbVar2 = extraout_A0;
      if (*(int *)(unaff_A6 + -0x677a) != 0) {
        uVar3 = FUN_00002fda(uVar3);
        func_0x00002b2e(uVar3);
        pbVar2 = extraout_A0_00;
      }
      for (; pbVar2[1] != 0; pbVar2 = pbVar2 + 1) {
      }
    }
    else if (bVar4 == 0x6c) {
      *(int *)(unaff_A6 + -0x679e) = *(int *)(unaff_A6 + -0x679e) + 1;
    }
    else {
      FUN_00001400();
      FUN_00000dd6();
      pbVar2 = extraout_A0_01;
    }
    pbVar2 = pbVar2 + 1;
  } while (*pbVar2 != 0);
  return;
}



// ===== FUN_000014e4 @ 000014e4 =====

void FUN_000014e4(void)

{
  FUN_00002f56();
  FUN_0000288a();
  return;
}



// ===== FUN_000014f8 @ 000014f8 =====

void FUN_000014f8(void)

{
  int iVar1;
  undefined4 extraout_D1;
  undefined4 uVar2;
  int unaff_A6;
  
  iVar1 = FUN_00002f56();
  uVar2 = *(undefined4 *)(unaff_A6 + -0x7ff0);
  if (iVar1 != -1) {
    FUN_00002954();
    FUN_00002970();
    uVar2 = extraout_D1;
  }
  *(undefined4 *)(unaff_A6 + -0x7ff0) = uVar2;
  return;
}



// ===== FUN_00001520 @ 00001520 =====

void FUN_00001520(void)

{
  undefined4 extraout_D1;
  
  FUN_00002f56();
  FUN_00002a02(0,0,extraout_D1,0,0);
  return;
}



// ===== FUN_0000153e @ 0000153e =====

void FUN_0000153e(void)

{
  FUN_00002f56();
  FUN_0000288a();
  FUN_000028c2();
  return;
}



// ===== FUN_00001582 @ 00001582 =====

void FUN_00001582(void)

{
  return;
}



// ===== FUN_0000158a @ 0000158a =====

undefined8 FUN_0000158a(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  int *piVar4;
  undefined4 in_D1;
  char *pcVar5;
  int *piVar6;
  int *extraout_A1;
  int *extraout_A1_00;
  char *pcVar7;
  char *pcVar8;
  int unaff_A6;
  
  FUN_00002f56();
  FUN_00002fe6();
  for (piVar4 = extraout_A1; *piVar4 != 0; piVar4 = piVar4 + 1) {
    pcVar2 = (char *)*piVar4;
    pcVar8 = (char *)(unaff_A6 + -0x6858);
    do {
      pcVar5 = pcVar2;
      pcVar7 = pcVar8;
      if (*pcVar5 != *pcVar8) break;
      pcVar7 = pcVar8 + 1;
      cVar1 = *pcVar8;
      pcVar2 = pcVar5 + 1;
      pcVar8 = pcVar7;
    } while (cVar1 != '=');
    if ((*pcVar7 == '\0') && (*pcVar5 == '=')) break;
  }
  iVar3 = FUN_00002fec();
  *(undefined1 *)(iVar3 + unaff_A6 + -0x6858) = 0x2f;
  iVar3 = FUN_00001f9c();
  if (iVar3 != -1) {
    if (*extraout_A1_00 == 0) {
      piVar4 = (int *)FUN_00002faa();
      iVar3 = 0;
      if (piVar4 != (int *)0x0) {
        *(undefined4 *)(unaff_A6 + -0x67be) = *(undefined4 *)(unaff_A6 + -0x6be8);
        piVar6 = piVar4;
        while (**(int **)(unaff_A6 + -0x6be8) != 0) {
          *piVar6 = **(int **)(unaff_A6 + -0x6be8);
          *(int *)(unaff_A6 + -0x6be8) = *(int *)(unaff_A6 + -0x6be8) + 4;
          piVar6 = piVar6 + 1;
        }
        *piVar6 = unaff_A6 + -0x6858;
        piVar6[1] = 0;
        *(int **)(unaff_A6 + -0x6be8) = piVar4;
        iVar3 = 0;
      }
    }
    else {
      *extraout_A1_00 = unaff_A6 + -0x6858;
    }
  }
  return CONCAT44(iVar3,in_D1);
}



// ===== FUN_0000163e @ 0000163e =====

int FUN_0000163e(byte *param_1)

{
  bool bVar1;
  byte bVar4;
  int iVar2;
  undefined4 uVar3;
  int iVar5;
  int iVar6;
  uint uVar7;
  int *extraout_A0;
  int *piVar8;
  undefined4 extraout_A0_00;
  undefined4 extraout_A0_01;
  undefined4 extraout_A0_02;
  undefined4 extraout_A0_03;
  undefined4 extraout_A0_04;
  undefined4 extraout_A0_05;
  undefined4 extraout_A0_06;
  byte *pbVar9;
  byte *extraout_A1;
  undefined4 extraout_A1_00;
  undefined4 extraout_A1_01;
  undefined4 extraout_A1_02;
  undefined4 extraout_A1_03;
  undefined4 extraout_A1_04;
  undefined4 uVar10;
  undefined4 extraout_A1_05;
  undefined4 extraout_A1_06;
  byte *pbVar11;
  int unaff_A6;
  undefined8 uVar12;
  
  uVar12 = FUN_00002f56();
  iVar5 = *(int *)uVar12;
  iVar6 = 0;
LAB_00001966:
  pbVar11 = (byte *)((ulonglong)uVar12 >> 0x20);
  piVar8 = (int *)uVar12;
  if (iVar5 < 1) {
    *piVar8 = iVar6;
    return iVar5;
  }
  pbVar9 = pbVar11 + 1;
  uVar12 = CONCAT44(pbVar9,piVar8);
  bVar4 = *pbVar11;
  uVar7 = (uint)bVar4;
  iVar5 = iVar5 + -1;
  switch(*(undefined4 *)(unaff_A6 + -0x6776)) {
  case 0:
    if (uVar7 == 0xff) {
      *(undefined4 *)(unaff_A6 + -0x6776) = 1;
      uVar12 = CONCAT44(pbVar9,piVar8);
    }
    else {
      if ((uVar7 == 10) && (*(char *)(unaff_A6 + -0x6ae0) == '\0')) {
        *param_1 = 0xd;
      }
      else {
        *param_1 = bVar4;
      }
      param_1 = param_1 + 1;
      iVar6 = iVar6 + 1;
      uVar12 = CONCAT44(pbVar9,piVar8);
      if ((uVar7 == 0xd) &&
         (uVar12 = CONCAT44(pbVar9,piVar8), *(char *)(unaff_A6 + -0x6ae0) == '\0')) {
        *(undefined4 *)(unaff_A6 + -0x6776) = 2;
        uVar12 = CONCAT44(pbVar9,piVar8);
      }
    }
    goto LAB_00001966;
  case 1:
    switch(uVar7) {
    case 0xf0:
      goto switchD_0000170a_caseD_f0;
    case 0xf6:
      FUN_00001f42(7);
      piVar8 = extraout_A0;
      pbVar9 = extraout_A1;
      break;
    case 0xf7:
    case 0xf8:
      if (uVar7 == 0xf7) {
        bVar4 = *(byte *)(unaff_A6 + -0x69d7);
      }
      else {
        bVar4 = *(byte *)(unaff_A6 + -0x69d6);
      }
      *param_1 = bVar4;
      iVar6 = iVar6 + 1;
      param_1 = param_1 + 1;
      break;
    case 0xfa:
      *(undefined4 *)(unaff_A6 + -0x6776) = 3;
      goto LAB_00001966;
    case 0xfb:
    case 0xfc:
    case 0xfd:
    case 0xfe:
      *(uint *)(unaff_A6 + -0x6776) = uVar7 - 0xf6;
      uVar12 = CONCAT44(pbVar9,piVar8);
      goto LAB_00001966;
    case 0xff:
switchD_0000170a_caseD_ff:
      *param_1 = bVar4;
      iVar6 = iVar6 + 1;
      param_1 = param_1 + 1;
    }
    break;
  case 2:
    if ((uVar7 != 0) && (uVar7 != 10)) goto switchD_0000170a_caseD_ff;
    break;
  case 3:
    uVar12 = CONCAT44(pbVar9,piVar8);
    if (uVar7 == 0xff) {
switchD_0000170a_caseD_f0:
      *(undefined4 *)(unaff_A6 + -0x6776) = 4;
      uVar12 = CONCAT44(pbVar9,piVar8);
    }
    goto LAB_00001966;
  case 4:
    if (uVar7 == 0xf0) {
      uVar3 = 0;
    }
    else {
      uVar3 = 3;
    }
    *(undefined4 *)(unaff_A6 + -0x6776) = uVar3;
    uVar12 = CONCAT44(pbVar9,piVar8);
    goto LAB_00001966;
  case 5:
    FUN_000012fa(uVar7,1);
    uVar3 = extraout_A0_00;
    uVar10 = extraout_A1_00;
    if (uVar7 == 0) {
      if (*(char *)(unaff_A6 + -0x6be0) == '\0') {
LAB_000017f2:
        FUN_00001976();
        uVar3 = extraout_A0_01;
        uVar10 = extraout_A1_01;
      }
    }
    else {
      if ((*(char *)(unaff_A6 + -0x6ae0 + uVar7) == '\0') &&
         (*(char *)(unaff_A6 + -0x6be0 + uVar7) == '\0')) {
        bVar1 = true;
      }
      else {
        bVar1 = false;
      }
      if (bVar1) goto LAB_000017f2;
LAB_000018c8:
      *(undefined1 *)(unaff_A6 + -0x6be0 + uVar7) = 1;
    }
    goto LAB_000018d2;
  case 6:
    FUN_000012fa(uVar7,1);
    if (uVar7 == 0) {
      iVar2 = (int)(short)*(char *)(unaff_A6 + -0x6be0);
    }
    else if ((*(char *)(unaff_A6 + -0x6ae0 + uVar7) == '\0') ||
            (*(char *)(unaff_A6 + -0x6be0 + uVar7) == '\0')) {
      iVar2 = 0;
    }
    else {
      iVar2 = 1;
    }
    *(undefined1 *)(unaff_A6 + -0x6be0 + uVar7) = 0;
    FUN_000019da(iVar2);
    uVar3 = extraout_A0_02;
    uVar10 = extraout_A1_02;
    goto LAB_00001934;
  case 7:
    FUN_000012fa(uVar7,1);
    uVar3 = extraout_A0_03;
    uVar10 = extraout_A1_03;
    if (uVar7 == 0) {
      if (*(char *)(unaff_A6 + -0x6ae0) != '\0') goto LAB_000018d2;
    }
    else {
      if ((*(char *)(unaff_A6 + -0x6ae0 + uVar7) == '\0') &&
         (*(char *)(unaff_A6 + -0x6be0 + uVar7) == '\0')) {
        bVar1 = true;
      }
      else {
        bVar1 = false;
      }
      if (!bVar1) goto LAB_000018c8;
    }
    FUN_00001a66();
    uVar3 = extraout_A0_04;
    uVar10 = extraout_A1_04;
LAB_000018d2:
    *(undefined4 *)(unaff_A6 + -0x6776) = 0;
    uVar12 = CONCAT44(uVar10,uVar3);
    goto LAB_00001966;
  case 8:
    FUN_000012fa(uVar7,1);
    if (uVar7 == 0) {
      iVar2 = (int)(short)*(char *)(unaff_A6 + -0x6ae0);
    }
    else {
      if ((*(char *)(unaff_A6 + -0x6ae0 + uVar7) == '\0') ||
         (*(char *)(unaff_A6 + -0x6be0 + uVar7) == '\0')) {
        iVar2 = 0;
      }
      else {
        iVar2 = 1;
      }
      *(undefined1 *)(unaff_A6 + -0x6be0 + uVar7) = 0;
    }
    FUN_00001af2(iVar2);
    uVar3 = extraout_A0_05;
    uVar10 = extraout_A1_05;
LAB_00001934:
    *(undefined4 *)(unaff_A6 + -0x6776) = 0;
    uVar12 = CONCAT44(uVar10,uVar3);
    goto LAB_00001966;
  default:
    goto switchD_00001676_default;
  }
  *(undefined4 *)(unaff_A6 + -0x6776) = 0;
  uVar12 = CONCAT44(pbVar9,piVar8);
  goto LAB_00001966;
switchD_00001676_default:
  uVar3 = FUN_00002fda(*(undefined4 *)(unaff_A6 + -0x6776));
  func_0x00002b2e(uVar3);
  FUN_00002f50();
  uVar12 = CONCAT44(extraout_A1_06,extraout_A0_06);
  goto LAB_00001966;
}



// ===== FUN_00001976 @ 00001976 =====

void FUN_00001976(void)

{
  int iVar1;
  int iVar2;
  int unaff_A6;
  undefined8 uVar3;
  
  uVar3 = FUN_00002f56();
  iVar1 = (int)((ulonglong)uVar3 >> 0x20);
  if (iVar1 != 0) {
    if ((iVar1 != 3) && ((iVar1 != 1 || (iVar2 = FUN_000013ce(), iVar2 != 0)))) goto LAB_00001ad2;
    *(undefined1 *)(unaff_A6 + -0x6ae0 + iVar1) = 1;
  }
  if ((int)uVar3 == 0) {
    *(undefined1 *)(unaff_A6 + -0x6be0 + iVar1) = 1;
  }
LAB_00001ad2:
  FUN_0000126c();
  FUN_000012fa(uVar3);
  return;
}



// ===== FUN_000019da @ 000019da =====

void FUN_000019da(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int unaff_A6;
  undefined8 uVar4;
  
  uVar4 = FUN_00002f56();
  iVar1 = (int)((ulonglong)uVar4 >> 0x20);
  if ((iVar1 != 0) && ((iVar1 == 3 || ((iVar1 == 1 && (iVar2 = FUN_000013ce(), iVar2 == 0)))))) {
    *(undefined1 *)(unaff_A6 + -0x6ae0 + iVar1) = 0;
  }
  if (param_1 == 0) {
    if (*(int *)(unaff_A6 + -0x677a) != 0) {
      uVar3 = FUN_00002fda();
      func_0x00002b2e(uVar3);
    }
  }
  else {
    FUN_0000126c();
    FUN_000012fa(uVar4);
  }
  return;
}



// ===== FUN_00001a66 @ 00001a66 =====

void FUN_00001a66(void)

{
  int iVar1;
  int iVar2;
  int unaff_A6;
  undefined8 uVar3;
  
  uVar3 = FUN_00002f56();
  iVar1 = (int)((ulonglong)uVar3 >> 0x20);
  if (iVar1 == 0) {
    *(undefined1 *)(unaff_A6 + -0x6ae0) = 1;
  }
  else if (((iVar1 == 3) || ((iVar1 == 1 && (iVar2 = FUN_000013ce(), iVar2 == 0)))) &&
          (*(undefined1 *)(unaff_A6 + -0x6ae0 + iVar1) = 1, (int)uVar3 == 0)) {
    *(undefined1 *)(unaff_A6 + -0x6be0 + iVar1) = 1;
  }
  FUN_0000126c();
  FUN_000012fa(uVar3);
  return;
}



// ===== FUN_00001af2 @ 00001af2 =====

void FUN_00001af2(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int unaff_A6;
  undefined8 uVar4;
  
  uVar4 = FUN_00002f56();
  iVar1 = (int)((ulonglong)uVar4 >> 0x20);
  if (((iVar1 == 0) || (iVar1 == 3)) || ((iVar1 == 1 && (iVar2 = FUN_000013ce(), iVar2 == 0)))) {
    *(undefined1 *)(unaff_A6 + -0x6ae0 + iVar1) = 0;
  }
  if (param_1 == 0) {
    if (*(int *)(unaff_A6 + -0x677a) != 0) {
      uVar3 = FUN_00002fda();
      func_0x00002b2e(uVar3);
    }
  }
  else {
    FUN_0000126c();
    FUN_000012fa(uVar4);
  }
  return;
}



// ===== FUN_00001ba6 @ 00001ba6 =====

undefined4 FUN_00001ba6(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_000030ee();
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00001bc2 @ 00001bc2 =====

undefined8 FUN_00001bc2(void)

{
  int iVar1;
  undefined4 in_D1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  iVar1 = FUN_00003106();
  if (iVar1 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    local_8 = 0xffffffff;
  }
  return CONCAT44(local_8,in_D1);
}



// ===== FUN_00001bf0 @ 00001bf0 =====

char * FUN_00001bf0(void)

{
  char cVar1;
  char *pcVar2;
  undefined8 uVar3;
  
  uVar3 = FUN_00002f56();
  do {
    pcVar2 = (char *)((ulonglong)uVar3 >> 0x20);
    cVar1 = *(char *)uVar3;
    uVar3 = CONCAT44(pcVar2 + 1,(char *)uVar3 + 1);
    *pcVar2 = cVar1;
  } while (cVar1 != '\0');
  *pcVar2 = ' ';
  return pcVar2 + 1;
}



// ===== FUN_00001c10 @ 00001c10 =====

void FUN_00001c10(short param_1)

{
  undefined1 *puVar1;
  undefined1 *puVar2;
  undefined8 uVar3;
  
  uVar3 = FUN_00002f56();
  while (puVar1 = (undefined1 *)((ulonglong)uVar3 >> 0x20), puVar2 = (undefined1 *)uVar3,
        0 < param_1) {
    uVar3 = CONCAT44(puVar1 + 1,puVar2 + 1);
    *puVar1 = *puVar2;
    param_1 = param_1 + -1;
  }
  FUN_00002574();
  return;
}



// ===== FUN_00001c32 @ 00001c32 =====

int FUN_00001c32(int *param_1,int *param_2,undefined4 param_3,short param_4,short param_5)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  ushort uVar5;
  short extraout_D1w;
  uint uVar4;
  code *extraout_A0;
  code *pcVar6;
  code *extraout_A0_00;
  int *piVar7;
  int *piVar8;
  int iVar9;
  undefined1 *puVar10;
  undefined1 *puVar11;
  int unaff_A6;
  undefined8 uVar12;
  undefined8 uVar13;
  undefined1 *local_1a;
  
  FUN_00002f56();
  uVar5 = 0;
  iVar3 = 0;
  uVar12 = FUN_00002fec();
  uVar4 = (uint)uVar12;
  iVar9 = iVar3;
  piVar8 = param_1;
  while (piVar8 = piVar8 + 1, piVar7 = param_2, *piVar8 != 0) {
    uVar13 = FUN_00002fec();
    uVar4 = (uint)uVar13;
    iVar9 = (int)((ulonglong)uVar13 >> 0x20) + iVar9;
    uVar5 = uVar5 + 1;
  }
  for (; *piVar7 != 0; piVar7 = piVar7 + 1) {
    iVar2 = FUN_00002fec();
    iVar3 = iVar2 + iVar3;
    uVar4 = (uint)(ushort)(extraout_D1w + 1);
  }
  iVar2 = FUN_000033ee(0);
  puVar11 = local_1a;
  piVar8 = param_1;
  if (iVar2 == 0) {
    while (piVar8[1] != 0) {
      puVar11 = (undefined1 *)FUN_00001bf0();
      piVar8 = piVar8 + 1;
    }
    if (1 < (ushort)(uVar5 + 1)) {
      puVar11[-1] = 0;
    }
    *puVar11 = 0xd;
    piVar8 = param_2;
    puVar10 = puVar11 + 1;
    if (((uVar4 & 0xffff) + iVar3 + uVar5 + 1 + iVar9 & 1) != 0) {
      puVar10 = puVar11 + 2;
      puVar11[1] = 0;
    }
    for (; *piVar8 != 0; piVar8 = piVar8 + 1) {
      puVar10 = (undefined1 *)FUN_00001bf0();
    }
    puVar10[-1] = 0;
    FUN_00001c10(2);
    FUN_00001c10(4);
    puVar10 = (undefined1 *)FUN_00001bf0();
    puVar10[-1] = 0;
    puVar11 = puVar10;
    if (((int)((ulonglong)uVar12 >> 0x20) + 1U & 1) != 0) {
      puVar11 = puVar10 + 1;
      *puVar10 = 0;
    }
    *puVar11 = 0;
    puVar11[1] = 0xd;
    FUN_00001c10(4);
    while (param_1 = param_1 + 1, *param_1 != 0) {
      FUN_00001c10(4);
      FUN_00002fec();
    }
    FUN_00001c10(4);
    for (; *param_2 != 0; param_2 = param_2 + 1) {
      FUN_00001c10(4);
      FUN_00002fec();
    }
    FUN_00001c10(4);
    iVar9 = 0;
    bVar1 = false;
    pcVar6 = extraout_A0;
    do {
      iVar3 = (*pcVar6)(local_1a,0,0,param_3,(int)param_4,(int)param_5);
      if (((iVar3 != -1) || (*(int *)(unaff_A6 + -0x7ff0) != 0xd8)) || (bVar1)) goto LAB_00001ec8;
      iVar9 = FUN_00001fb6(0);
      bVar1 = true;
      pcVar6 = extraout_A0_00;
    } while (iVar9 != -1);
    iVar9 = 0;
LAB_00001ec8:
    if (iVar9 != 0) {
      FUN_00001fee();
    }
    FUN_00003418();
  }
  else {
    iVar3 = -1;
  }
  return iVar3;
}



// ===== FUN_00001eea @ 00001eea =====

undefined4 FUN_00001eea(undefined4 param_1)

{
  int iVar1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  local_8 = param_1;
  iVar1 = FUN_0000333a(&local_8);
  if (iVar1 != 0) {
    if (iVar1 == 0xd3) {
      local_8 = 0;
    }
    else {
      *(int *)(unaff_A6 + -0x7ff0) = iVar1;
      local_8 = 0xffffffff;
    }
  }
  return local_8;
}



// ===== FUN_00001f42 @ 00001f42 =====

undefined4 FUN_00001f42(undefined4 param_1)

{
  int iVar1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  local_8 = param_1;
  iVar1 = FUN_000034d2(&local_8);
  if (iVar1 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    local_8 = 0xffffffff;
  }
  return local_8;
}



// ===== FUN_00001f6e @ 00001f6e =====

undefined4 FUN_00001f6e(void)

{
  int iVar1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  iVar1 = func_0x000032fe(&local_8);
  if (iVar1 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    local_8 = 0xffffffff;
  }
  return local_8;
}



// ===== FUN_00001f9c @ 00001f9c =====

undefined4 FUN_00001f9c(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_000031f6();
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00001fb6 @ 00001fb6 =====

void FUN_00001fb6(undefined4 param_1)

{
  int iVar1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  iVar1 = FUN_000032f8(param_1,&local_8);
  if (iVar1 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    local_8 = 0xffffffff;
  }
  FUN_00002574();
  return;
}



// ===== FUN_00001fee @ 00001fee =====

undefined4 FUN_00001fee(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_000034b4();
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002010 @ 00002010 =====

undefined4 FUN_00002010(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_D1;
  int unaff_A6;
  undefined4 local_44 [3];
  undefined1 local_38;
  undefined4 *local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined1 local_20;
  undefined1 local_1d;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_c;
  undefined4 local_8;
  
  FUN_00002f56();
  local_20 = 0x82;
  local_18 = 0;
  local_8 = 1;
  local_24 = 0;
  local_1d = 0;
  local_1c = 0;
  local_30 = &local_24;
  local_44[0] = 0x1004;
  local_38 = 0;
  local_2c = 0;
  local_28 = 0;
  local_c = extraout_D1;
  iVar1 = FUN_00002b08(local_44);
  *(int *)(unaff_A6 + -0x7ff0) = iVar1;
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002076 @ 00002076 =====

undefined4 FUN_00002076(uint *param_1)

{
  short sVar3;
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_D1;
  uint **ppuVar4;
  undefined4 *puVar5;
  int unaff_A6;
  uint *local_88 [3];
  undefined1 local_7c;
  short local_74;
  char local_48;
  byte local_45;
  
  FUN_00002f56();
  sVar3 = 3;
  ppuVar4 = local_88;
  puVar5 = (undefined4 *)(unaff_A6 + -0x680c);
  do {
    *ppuVar4 = (uint *)*puVar5;
    sVar3 = sVar3 + -1;
    ppuVar4 = ppuVar4 + 1;
    puVar5 = puVar5 + 1;
  } while (sVar3 != -1);
  local_88[0] = (uint *)0xff0007;
  local_88[1] = param_1;
  local_7c = 0;
  local_88[2] = (uint *)extraout_D1;
  iVar1 = FUN_00002ae8(local_88);
  *(int *)(unaff_A6 + -0x7ff0) = iVar1;
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else if (iVar1 == 0xd0) {
    iVar1 = FUN_00002164();
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    if (iVar1 == 0) {
      if ((local_74 == 8) || (local_74 == 0x20)) {
        if (local_48 == '\x03') {
          if ((int)*param_1 < (int)(uint)local_45) {
            *(undefined4 *)(unaff_A6 + -0x7ff0) = 0x71b;
            uVar2 = 0xffffffff;
          }
          else {
            *param_1 = (uint)local_45;
            FUN_00002fd4(*param_1);
            uVar2 = 0;
          }
        }
        else {
          *(undefined4 *)(unaff_A6 + -0x7ff0) = 0x706;
          uVar2 = 0xffffffff;
        }
      }
      else {
        *(undefined4 *)(unaff_A6 + -0x7ff0) = 0x716;
        uVar2 = 0xffffffff;
      }
    }
    else {
      uVar2 = 0xffffffff;
    }
  }
  else {
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002164 @ 00002164 =====

undefined4 FUN_00002164(void)

{
  undefined4 uVar1;
  int extraout_D1;
  undefined4 local_14 [2];
  int local_c;
  undefined1 local_8;
  
  FUN_00002f56();
  if (extraout_D1 == 0) {
    uVar1 = 0x600;
  }
  else {
    local_14[0] = 0x922;
    local_8 = 0;
    local_c = extraout_D1;
    uVar1 = FUN_00002ae8(local_14);
  }
  return uVar1;
}



// ===== FUN_000021a2 @ 000021a2 =====

undefined8 FUN_000021a2(void)

{
  undefined4 uVar1;
  undefined4 in_D1;
  int unaff_A6;
  
  FUN_00002f56();
  if (*(int *)(unaff_A6 + -0x67c2) < 0) {
    *(undefined4 *)(unaff_A6 + -0x6c9c) = 1;
    *(undefined1 **)(unaff_A6 + -0x6c7c) = &LAB_000026da;
    *(undefined4 *)(unaff_A6 + -0x6c94) = 0;
    *(int *)(unaff_A6 + -0x6c98) = unaff_A6 + -0x7ff0;
    *(int *)(unaff_A6 + -0x6c80) = unaff_A6 + -0x6be8;
    uVar1 = FUN_00002a28();
    *(undefined4 *)(unaff_A6 + -0x67c2) = uVar1;
  }
  return CONCAT44(*(undefined4 *)(unaff_A6 + -0x67c2),in_D1);
}



// ===== FUN_00002204 @ 00002204 =====

undefined4 FUN_00002204(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = func_0x00002a54(param_1);
  }
  return uVar2;
}



// ===== FUN_00002232 @ 00002232 =====

undefined4 FUN_00002232(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = FUN_00002a58();
  }
  return uVar2;
}



// ===== FUN_0000226e @ 0000226e =====

undefined8 FUN_0000226e(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0xffffffff;
  }
  else {
    uVar2 = FUN_00002a60();
  }
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_000022aa @ 000022aa =====

undefined4 FUN_000022aa(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = FUN_00002a68();
  }
  return uVar2;
}



// ===== FUN_000022cc @ 000022cc =====

undefined4 FUN_000022cc(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = FUN_00002a6c();
  }
  return uVar2;
}



// ===== FUN_00002308 @ 00002308 =====

undefined8 FUN_00002308(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0xffffffff;
  }
  else {
    uVar2 = FUN_00002a74();
  }
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_00002344 @ 00002344 =====

undefined4 FUN_00002344(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = func_0x00002a7c();
  }
  return uVar2;
}



// ===== FUN_00002366 @ 00002366 =====

undefined8 FUN_00002366(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = func_0x00002a80();
  }
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_000023a2 @ 000023a2 =====

undefined8 FUN_000023a2(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0xffffffff;
  }
  else {
    uVar2 = func_0x00002a88();
  }
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_000023de @ 000023de =====

undefined4 FUN_000023de(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = func_0x00002a90();
  }
  return uVar2;
}



// ===== FUN_00002404 @ 00002404 =====

undefined4 FUN_00002404(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = func_0x00002a94();
  }
  return uVar2;
}



// ===== FUN_00002444 @ 00002444 =====

undefined8 FUN_00002444(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0xffffffff;
  }
  else {
    uVar2 = func_0x00002a9c();
  }
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_00002480 @ 00002480 =====

undefined4 FUN_00002480(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = func_0x00002aa4();
  }
  return uVar2;
}



// ===== FUN_000024bc @ 000024bc =====

undefined8 FUN_000024bc(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 in_D1;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0xffffffff;
  }
  else {
    uVar2 = func_0x00002aac();
  }
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_000024de @ 000024de =====

undefined4 FUN_000024de(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0x85;
  }
  else {
    uVar2 = func_0x00002ab0();
  }
  return uVar2;
}



// ===== FUN_00002504 @ 00002504 =====

undefined4 FUN_00002504(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0x85;
  }
  else {
    uVar2 = func_0x00002ab4();
  }
  return uVar2;
}



// ===== FUN_00002574 @ 00002574 =====

void FUN_00002574(void)

{
  return;
}



// ===== FUN_0000257c @ 0000257c =====

undefined4 FUN_0000257c(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0x85;
  }
  else {
    uVar2 = func_0x00002ac0();
  }
  return uVar2;
}



// ===== FUN_000025a2 @ 000025a2 =====

undefined4 FUN_000025a2(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0x85;
  }
  else {
    uVar2 = func_0x00002ac4();
  }
  return uVar2;
}



// ===== FUN_0000263c @ 0000263c =====

undefined4 FUN_0000263c(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0x85;
  }
  else {
    uVar2 = FUN_00002ad8();
  }
  return uVar2;
}



// ===== FUN_0000267c @ 0000267c =====

undefined4 FUN_0000267c(void)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00002f56();
  iVar1 = FUN_000021a2();
  if (iVar1 < 0) {
    uVar2 = 0x85;
  }
  else {
    uVar2 = FUN_00002ae0();
  }
  return uVar2;
}



// ===== FUN_00002720 @ 00002720 =====

void FUN_00002720(void)

{
  int iVar1;
  
  FUN_00002f56();
  do {
    iVar1 = FUN_00003370();
  } while (iVar1 != 0);
  return;
}



// ===== FUN_0000273a @ 0000273a =====

void FUN_0000273a(void)

{
  FUN_00002f56();
  FUN_000033a8();
  return;
}



// ===== FUN_0000274e @ 0000274e =====

void FUN_0000274e(void)

{
  int unaff_A6;
  
  FUN_00002f56();
  (**(code **)(unaff_A6 + -0x6c0c))();
  FUN_00003360();
  return;
}



// ===== FUN_00002766 @ 00002766 =====

undefined8 FUN_00002766(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 in_D1;
  int unaff_A6;
  
  uVar1 = FUN_00002f56();
  *(undefined4 *)(unaff_A6 + -0x6c0c) = uVar1;
  uVar1 = 0;
  if ((*(int *)(unaff_A6 + -0x6c08) == 0) && (iVar2 = FUN_000032c6(), iVar2 != 0)) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar2;
    uVar1 = 0xffffffff;
  }
  return CONCAT44(uVar1,in_D1);
}



// ===== FUN_00002812 @ 00002812 =====

undefined4 FUN_00002812(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_D1;
  undefined4 uStack_8;
  
  FUN_00002f56();
  iVar1 = FUN_00003026(extraout_D1,param_1);
  if (iVar1 != 0) {
    uVar2 = FUN_000029c4();
    return uVar2;
  }
  return uStack_8;
}



// ===== FUN_0000283c @ 0000283c =====

undefined8 FUN_0000283c(void)

{
  int iVar1;
  undefined4 in_D1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  iVar1 = FUN_00003296();
  if (iVar1 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    local_8 = 0xffffffff;
  }
  return CONCAT44(local_8,in_D1);
}



// ===== FUN_0000288a @ 0000288a =====

undefined4 FUN_0000288a(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_D1;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_0000323a(extraout_D1);
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_000028c2 @ 000028c2 =====

undefined4 FUN_000028c2(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_D1;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_00003450(extraout_D1);
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002908 @ 00002908 =====

undefined4 FUN_00002908(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_00003326();
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002924 @ 00002924 =====

undefined8 FUN_00002924(void)

{
  int iVar1;
  undefined4 in_D1;
  int unaff_A6;
  undefined4 local_8;
  
  FUN_00002f56();
  iVar1 = FUN_00003172();
  if (iVar1 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    local_8 = 0xffffffff;
  }
  return CONCAT44(local_8,in_D1);
}



// ===== FUN_00002954 @ 00002954 =====

undefined4 FUN_00002954(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_00003196();
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002970 @ 00002970 =====

undefined4 FUN_00002970(void)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_A6;
  
  FUN_00002f56();
  iVar1 = FUN_00003152();
  if (iVar1 == 0) {
    uVar2 = 0;
  }
  else {
    *(int *)(unaff_A6 + -0x7ff0) = iVar1;
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



// ===== FUN_00002992 @ 00002992 =====

undefined4 FUN_00002992(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  int iVar2;
  int unaff_A6;
  undefined4 local_8;
  
  uVar1 = FUN_00002f56();
  iVar2 = FUN_00003122(0x333,&local_8,param_2,uVar1,0);
  if (iVar2 != 0) {
    *(int *)(unaff_A6 + -0x7ff0) = iVar2;
    local_8 = 0xffffffff;
  }
  return local_8;
}



// ===== FUN_000029c4 @ 000029c4 =====

undefined4 FUN_000029c4(void)

{
  undefined4 in_D0;
  int unaff_A6;
  
  *(undefined4 *)(unaff_A6 + -0x7ff0) = in_D0;
  return 0xffffffff;
}



// ===== FUN_000029d8 @ 000029d8 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_000029d8(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a02 @ 00002a02 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a02(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a28 @ 00002a28 =====

undefined8 FUN_00002a28(void)

{
  undefined4 in_D1;
  int in_A1;
  int unaff_A6;
  
  __m68k_trap(0);
  *(byte *)(in_A1 + -1) = *(byte *)(in_A1 + -1) | 6;
  *(undefined4 *)(unaff_A6 + -0x7ff0) = 0;
  return CONCAT44(0xffffffff,in_D1);
}



// ===== FUN_00002a50 @ 00002a50 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a50(void)

{
  __m68k_trap(9);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a58 @ 00002a58 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a58(void)

{
  __m68k_trap(9);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a60 @ 00002a60 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a60(void)

{
  __m68k_trap(9);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a68 @ 00002a68 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a68(void)

{
  __m68k_trap(9);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a6c @ 00002a6c =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a6c(void)

{
  __m68k_trap(9);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002a74 @ 00002a74 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002a74(void)

{
  byte *in_A0;
  byte *in_A1;
  byte *unaff_A2;
  byte *unaff_A3;
  byte *unaff_A4;
  byte *unaff_A5;
  byte *unaff_A6;
  
  __m68k_trap(9);
  *in_A0 = *in_A0 | 0x49;
  *in_A1 = *in_A1 | 0x49;
  *unaff_A2 = *unaff_A2 | 0x49;
  *unaff_A3 = *unaff_A3 | 0x49;
  *unaff_A4 = *unaff_A4 | 0x49;
  *unaff_A5 = *unaff_A5 | 0x49;
  *unaff_A6 = *unaff_A6 | 0x49;
  *in_A0 = *in_A0 | 0x49;
  *in_A1 = *in_A1 | 0x49;
  *unaff_A2 = *unaff_A2 | 0x49;
  *unaff_A3 = *unaff_A3 | 0x49;
  *unaff_A4 = *unaff_A4 | 0x49;
  *unaff_A5 = *unaff_A5 | 0x49;
  *unaff_A6 = *unaff_A6 | 0x49;
  *in_A0 = *in_A0 | 0x49;
  *in_A1 = *in_A1 | 0x49;
  *unaff_A2 = *unaff_A2 | 0x49;
  *unaff_A3 = *unaff_A3 | 0x49;
  *unaff_A4 = *unaff_A4 | 0x49;
  *unaff_A5 = *unaff_A5 | 0x49;
  *unaff_A6 = *unaff_A6 | 0x49;
  in_A0[0x1f] = in_A0[0x1f] | 0x49;
  __m68k_trap(9);
  in_A1[0x2a] = in_A1[0x2a] | 0x49;
  __m68k_trap(9);
  unaff_A3[0x2c] = unaff_A3[0x2c] | 0x49;
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002ad8 @ 00002ad8 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002ad8(void)

{
  int in_A1;
  int unaff_A3;
  
  __m68k_trap(9);
  *(byte *)(in_A1 + 0x2a) = *(byte *)(in_A1 + 0x2a) | 0x49;
  __m68k_trap(9);
  *(byte *)(unaff_A3 + 0x2c) = *(byte *)(unaff_A3 + 0x2c) | 0x49;
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002ae0 @ 00002ae0 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002ae0(void)

{
  int unaff_A3;
  
  __m68k_trap(9);
  *(byte *)(unaff_A3 + 0x2c) = *(byte *)(unaff_A3 + 0x2c) | 0x49;
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002ae8 @ 00002ae8 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002ae8(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002b08 @ 00002b08 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002b08(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002b28 @ 00002b28 =====

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00002b28(void)

{
  ushort *extraout_A1;
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *extraout_A1 = *extraout_A1 | 0x4eae;
  extraout_A1[-0x33b8] = extraout_A1[-0x33b8] | 0x4eae;
  _DAT_00007421 = _DAT_00007421 | 0x4e75;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002bf6 @ 00002bf6 =====

undefined4 FUN_00002bf6(uint param_1,int param_2)

{
  int iVar1;
  byte *pbVar2;
  uint in_D0;
  int in_D1;
  undefined2 uVar3;
  undefined2 uVar4;
  undefined2 uVar5;
  int unaff_A6;
  
  uVar4 = 0;
  uVar3 = 1;
  uVar5 = 2;
  if ((*(byte *)(*(int *)(unaff_A6 + -0x7fde) + 0x14) & 0x20) != 0) {
    iVar1 = *(int *)(*(int *)(unaff_A6 + -0x7fe2) + 0x4c);
    uVar4 = *(undefined2 *)(iVar1 + 0x168);
    uVar3 = *(undefined2 *)(iVar1 + 0x16a);
    uVar5 = *(undefined2 *)(iVar1 + 0x16c);
  }
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 100) = 2;
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 0x66) = uVar5;
  if (((in_D1 == param_2) && (in_D0 == param_1)) &&
     ((in_D0 == 0 || (((in_D0 & 1) != 0 || (*(short *)(in_D0 + in_D1 + -2) != 0)))))) {
    FUN_00002d6c();
  }
  (*(code *)&LAB_00002b38_4)();
  FUN_00002ed4();
  FUN_00002ed4();
  FUN_00002ed4();
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 0xc) = 1;
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 0xe) = uVar4;
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 0x38) = 2;
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 0x3a) = uVar3;
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 100) = 2;
  *(undefined2 *)(*(int *)(unaff_A6 + -0x6bf0) + 0x66) = uVar5;
  pbVar2 = (byte *)(*(int *)(unaff_A6 + -0x6bf0) + 0x65);
  *pbVar2 = *pbVar2 | 0x40;
  (*(code *)&LAB_00002ed8_2)();
  (*(code *)&LAB_00002ed8_2)();
  (*(code *)&LAB_00002ed8_2)();
  return 0;
}



// ===== FUN_00002d6c @ 00002d6c =====

int FUN_00002d6c(void)

{
  char cVar1;
  char *in_D0;
  uint in_D1;
  char *pcVar2;
  char *pcVar3;
  
  pcVar3 = in_D0;
  do {
    pcVar2 = pcVar3 + 1;
    cVar1 = *pcVar3;
    pcVar3 = pcVar2;
  } while (cVar1 != '\0');
  return ((in_D1 >> 1) + 1) * 4 + ((uint)(pcVar2 + (3 - (int)in_D0)) & 0xfffffffc) + 8;
}



// ===== FUN_00002ed4 @ 00002ed4 =====

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Removing unreachable block (ram,0x00002ef2) */
/* WARNING: Removing unreachable block (ram,0x00002f55) */

void FUN_00002ed4(void)

{
  int unaff_A5;
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *(ushort *)(unaff_A6 + -0x6772) = *(ushort *)(unaff_A6 + -0x6772) | 0x4eae;
  *(ushort *)(FUN_00002a28 + unaff_A5 + 2) = *(ushort *)(FUN_00002a28 + unaff_A5 + 2) | 0x2a2a;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002efa @ 00002efa =====

undefined8 FUN_00002efa(void)

{
  undefined4 in_D0;
  undefined4 in_D1;
  int unaff_A6;
  undefined4 local_8;
  
  local_8 = 0x1a;
  if ((*(ushort *)(unaff_A6 + -0x7f5c) & 2) != 0) {
    FUN_000034f8(&local_8);
  }
  FUN_00002ff2();
  return CONCAT44(in_D0,in_D1);
}



// ===== FUN_00002f4a @ 00002f4a =====

void FUN_00002f4a(void)

{
  return;
}



// ===== FUN_00002f50 @ 00002f50 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002f50(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002f56 @ 00002f56 =====

undefined4 FUN_00002f56(void)

{
  int iVar1;
  undefined1 *extraout_A0;
  undefined1 *puVar2;
  int unaff_A6;
  undefined4 *puVar3;
  undefined1 local_110 [256];
  undefined1 auStack_10 [4];
  
  puVar2 = auStack_10;
  iVar1 = (int)puVar2 - *(int *)(unaff_A6 + -0x7ff4);
  *(int *)(unaff_A6 + -0x6be4) = iVar1;
  puVar3 = (undefined4 *)puVar2;
  if (iVar1 < 0) {
    *(undefined1 **)(unaff_A6 + -0x7ff4) = puVar2;
    *(undefined4 *)(unaff_A6 + -0x6be4) = 0;
    if (puVar2 <= *(undefined1 **)(unaff_A6 + -0x7ff8)) {
      puVar3 = (undefined4 *)local_110;
      *(undefined4 *)(unaff_A6 + -0x6be4) = 0x100;
      FUN_00002efa();
      puVar2 = extraout_A0;
    }
    *(int *)(unaff_A6 + -0x7fd2) = *(int *)(unaff_A6 + -0x8000) - (int)puVar2;
  }
  return *puVar3;
}



// ===== FUN_00002f9e @ 00002f9e =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002f9e(void)

{
  byte *unaff_A5;
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *unaff_A5 = *unaff_A5 | 0xae;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fa4 @ 00002fa4 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fa4(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002faa @ 00002faa =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002faa(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fb0 @ 00002fb0 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fb0(void)

{
  int unaff_A3;
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *(byte *)(unaff_A3 + -0x6772) = *(byte *)(unaff_A3 + -0x6772) | 0xae;
  *(byte *)(unaff_A6 + -1) = *(byte *)(unaff_A6 + -1) | 0xae;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fb6 @ 00002fb6 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fb6(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *(byte *)(unaff_A6 + -1) = *(byte *)(unaff_A6 + -1) | 0xae;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fc2 @ 00002fc2 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fc2(void)

{
  byte *extraout_A0;
  byte *extraout_A1;
  byte *unaff_A2;
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *unaff_A2 = *unaff_A2 | 0xae;
  *extraout_A0 = *extraout_A0 | 0xae;
  *extraout_A1 = *extraout_A1 | 0xae;
  *(byte *)(unaff_A6 + -0x6772) = *(byte *)(unaff_A6 + -0x6772) | 0xae;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fd4 @ 00002fd4 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fd4(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *(byte *)(unaff_A6 + -0x6772) = *(byte *)(unaff_A6 + -0x6772) | 0xae;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fda @ 00002fda =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fda(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
  *(byte *)(unaff_A6 + -0x6772) = *(byte *)(unaff_A6 + -0x6772) | 0xae;
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fe6 @ 00002fe6 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fe6(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002fec @ 00002fec =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00002fec(void)

{
  int unaff_A6;
  
  (**(code **)(unaff_A6 + -0x6772))();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00002ff2 @ 00002ff2 =====

void FUN_00002ff2(void)

{
  __m68k_trap(0);
  return;
}



// ===== FUN_00003002 @ 00003002 =====

uint FUN_00003002(void)

{
  ushort uVar1;
  uint in_D0;
  int iVar2;
  uint in_D1;
  
  iVar2 = (in_D0 & 0xffff) * (in_D1 & 0xffff);
  uVar1 = (ushort)((uint)(iVar2 * 0x10000) >> 0x10);
  return CONCAT22(uVar1,(short)in_D0 * (short)(in_D1 >> 0x10) +
                        (short)in_D1 * (short)(in_D0 >> 0x10) + (short)((uint)iVar2 >> 0x10)) <<
         0x10 | (uint)uVar1;
}



// ===== FUN_00003026 @ 00003026 =====

undefined4 FUN_00003026(void)

{
  undefined4 *in_D1;
  ushort *unaff_A3;
  
  __m68k_trap(0);
  *unaff_A3 = *unaff_A3 | 0x650c;
  *in_D1 = 4;
  return 0;
}



// ===== FUN_00003050 @ 00003050 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00003050(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00003072 @ 00003072 =====

undefined4 FUN_00003072(void)

{
  undefined4 in_D1;
  ushort *unaff_A6;
  
  __m68k_trap(0);
  *unaff_A6 = *unaff_A6 | 0x6508;
  return in_D1;
}



// ===== FUN_0000309a @ 0000309a =====

undefined4 FUN_0000309a(void)

{
  undefined4 *in_A0;
  ushort *unaff_A6;
  undefined4 in_stack_00000000;
  
  __m68k_trap(0);
  *unaff_A6 = *unaff_A6 | 0x650c;
  *in_A0 = 0;
  return in_stack_00000000;
}



// ===== FUN_000030b6 @ 000030b6 =====

void FUN_000030b6(void)

{
  FUN_0000309a();
  return;
}



// ===== FUN_000030ee @ 000030ee =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_000030ee(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00003106 @ 00003106 =====

undefined4 FUN_00003106(void)

{
  __m68k_trap(0);
  return 0;
}



// ===== FUN_00003122 @ 00003122 =====

undefined8
FUN_00003122(undefined4 param_1,undefined4 *param_2,undefined4 param_3,undefined4 param_4)

{
  ushort *unaff_A3;
  
  __m68k_trap(0);
  *unaff_A3 = *unaff_A3 | 0x6510;
  *param_2 = param_4;
  return 2;
}



// ===== FUN_00003152 @ 00003152 =====

undefined4 FUN_00003152(void)

{
  undefined4 in_D1;
  ushort *unaff_A3;
  
  __m68k_trap(0);
  *unaff_A3 = *unaff_A3 | 0x650a;
  return in_D1;
}



// ===== FUN_00003172 @ 00003172 =====

undefined8 FUN_00003172(void)

{
  undefined4 in_D0;
  undefined4 *in_D1;
  ushort *unaff_A3;
  
  __m68k_trap(0);
  *unaff_A3 = *unaff_A3 | 0x650c;
  *in_D1 = in_D0;
  return 0;
}



// ===== FUN_00003196 @ 00003196 =====

undefined4 FUN_00003196(void)

{
  undefined4 in_D1;
  ushort *unaff_A3;
  
  __m68k_trap(0);
  *unaff_A3 = *unaff_A3 | 0x6508;
  return in_D1;
}



// ===== FUN_000031b0 @ 000031b0 =====

undefined8 FUN_000031b0(void)

{
  undefined4 in_D0;
  undefined4 uVar1;
  undefined4 in_stack_00000010;
  undefined4 *in_stack_00000014;
  undefined2 uStack00000018;
  int in_stack_0000001c;
  
  if (in_stack_0000001c == 0) {
    __m68k_trap(0);
    *in_stack_00000014 = CONCAT22((short)((uint)in_D0 >> 0x10),uStack00000018);
    uVar1 = 0;
  }
  else {
    in_stack_00000010 = 0xe1;
    uVar1 = 0xe1;
  }
  return CONCAT44(uVar1,in_stack_00000010);
}



// ===== FUN_000031f6 @ 000031f6 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_000031f6(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_0000323a @ 0000323a =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_0000323a(void)

{
  int *in_D1;
  
  if (*in_D1 < 0x80) {
    __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00003296 @ 00003296 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00003296(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_000032c6 @ 000032c6 =====

/* WARNING: Control flow encountered bad instruction data */

undefined4 FUN_000032c6(void)

{
  undefined4 in_D0;
  int in_D1;
  int unaff_A6;
  
  if (unaff_A6 != in_D1) {
    return 0xe1;
  }
  *(undefined4 *)(unaff_A6 + -0x6bf4) = in_D0;
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_000032f8 @ 000032f8 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_000032f8(void)

{
  short sVar1;
  int unaff_A3;
  int unaff_A6;
  
  sVar1 = (**(code **)(unaff_A6 + -0x6772))();
  *(ushort *)(unaff_A3 + sVar1) = *(ushort *)(unaff_A3 + sVar1) | 0x4e55;
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00003326 @ 00003326 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00003326(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_0000333a @ 0000333a =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_0000333a(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_00003360 @ 00003360 =====

void FUN_00003360(void)

{
  byte *unaff_A6;
  
  *(undefined4 *)(unaff_A6 + -0x6c00) = **(undefined4 **)(unaff_A6 + -0x6c00);
  __m68k_trap(0);
  *unaff_A6 = *unaff_A6 | 1;
  return;
}



// ===== FUN_00003370 @ 00003370 =====

undefined4 FUN_00003370(void)

{
  byte bVar1;
  byte *in_D0;
  undefined4 in_D1;
  ushort *unaff_A2;
  
  *(int *)(in_D0 + 4) = *(int *)(in_D0 + 4) + 1;
  bVar1 = *in_D0;
  *in_D0 = *in_D0 | 0x80;
  if (bVar1 != 0) {
    do {
      __m68k_trap(0);
      unaff_A2 = unaff_A2 + -1;
      *unaff_A2 = *unaff_A2 | 0x650e;
      bVar1 = *in_D0;
      *in_D0 = *in_D0 | 0x80;
    } while (bVar1 != 0);
    return in_D1;
  }
  return in_D1;
}



// ===== FUN_00003382 @ 00003382 =====

undefined4 FUN_00003382(void)

{
  byte bVar1;
  byte *in_D0;
  undefined4 in_D1;
  ushort *unaff_A2;
  
  do {
    __m68k_trap(0);
    unaff_A2 = unaff_A2 + -1;
    *unaff_A2 = *unaff_A2 | 0x650e;
    bVar1 = *in_D0;
    *in_D0 = *in_D0 | 0x80;
  } while (bVar1 != 0);
  return in_D1;
}



// ===== FUN_000033a8 @ 000033a8 =====

undefined8 FUN_000033a8(void)

{
  int iVar1;
  char *in_D0;
  undefined4 uVar2;
  undefined4 in_D1;
  int unaff_A2;
  
  if ((in_D0[0x1f] & 1U) == 0) {
    in_D0[0] = '\0';
    in_D0[1] = '\0';
    in_D0[2] = '\0';
    in_D0[3] = '\0';
  }
  else if (*in_D0 == -0x80) {
    *in_D0 = '\0';
  }
  iVar1 = *(int *)(in_D0 + 4) + -1;
  *(int *)(in_D0 + 4) = iVar1;
  if (iVar1 != 0) {
    if (-1 < iVar1) {
      __m68k_trap(0);
      *(ushort *)(unaff_A2 + -2) = *(ushort *)(unaff_A2 + -2) | 0x64ec;
      uVar2 = 2;
      goto LAB_000033ca;
    }
    *(int *)(in_D0 + 4) = *(int *)(in_D0 + 4) + 1;
  }
  uVar2 = 0;
LAB_000033ca:
  return CONCAT44(uVar2,in_D1);
}



// ===== FUN_000033ee @ 000033ee =====

undefined4 FUN_000033ee(undefined4 param_1)

{
  undefined4 uVar1;
  undefined4 *in_D0;
  undefined4 *in_D1;
  undefined4 unaff_A2;
  ushort *unaff_A4;
  
  uVar1 = *in_D0;
  __m68k_trap(0);
  *unaff_A4 = *unaff_A4 | 0x650e;
  *in_D0 = uVar1;
  *in_D1 = unaff_A2;
  return param_1;
}



// ===== FUN_00003418 @ 00003418 =====

void FUN_00003418(void)

{
  int in_A1;
  
  __m68k_trap(0);
  *(byte *)(in_A1 + 0x7000) = *(byte *)(in_A1 + 0x7000) | 8;
  return;
}



// ===== FUN_00003450 @ 00003450 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_00003450(void)

{
  short in_D1w;
  
  if (in_D1w < 0x80) {
    __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_000034b4 @ 000034b4 =====

undefined4 FUN_000034b4(void)

{
  undefined4 in_D1;
  
  __m68k_trap(0);
  return in_D1;
}



// ===== FUN_000034d2 @ 000034d2 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_000034d2(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



// ===== FUN_000034f8 @ 000034f8 =====

/* WARNING: Control flow encountered bad instruction data */

void FUN_000034f8(void)

{
  __m68k_trap(0);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



