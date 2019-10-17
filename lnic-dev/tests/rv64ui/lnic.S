# See LICENSE for license details.

#*****************************************************************************
# add.S
#-----------------------------------------------------------------------------
#
# Test add instruction.
#

#include "riscv_test.h"


RVTEST_RV64U
RVTEST_CODE_BEGIN
  lw    x2, testdata;
  addi  x2, x2, 1;
  la    x3, result;
  sw    x2, 0(x3);
  li    x3, 42;
  bne   x2, x3, fail;
  RVTEST_PASS
fail:
  RVTEST_FAIL
RVTEST_CODE_END

.data
  .align 3
testdata:
  .dword 41

result:
  .dword -1

RVTEST_DATA_BEGIN
RVTEST_DATA_END
